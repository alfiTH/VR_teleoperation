#include "RobotMiddleware.h"
#include <include/GenericBase.h>
#include <include/KinovaArm.h>
#include <include/Lidar3D.h>
#include <include/OmniRobot.h>
#include <include/VRController.h>
#include <include/Navigator.h>

#include <DataRecord.h>


#include <Ice/Communicator.h>
#include <IceStorm/IceStorm.h>
#include <Ice/Ice.h> 

#include <iostream>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <numeric>
#include <deque>
#include <shared_mutex>


#include "RobocompMiddleware.tpp"
#include "RobocompMiddlewareParsers.cpp"


struct RobotMiddleware::Impl {
  std::atomic<bool> running{true};
  Ice::CommunicatorHolder communicator;

  RoboCompVRController::VRControllerPrxPtr vrcontroller_proxy;
  std::future<RoboCompVRController::Haptics> haptics_future;
  std::mutex haptic_mutex;

  // Lidar
  RoboCompLidar3D::Lidar3DPrxPtr lidar3d_proxy, zed_proxy;
  std::mutex lidar_mutex;
  std::thread lidar_worker;
  RobotMiddleware::ColorCloudData cloudPoints;
  uint64_t cloudVersion = 0;

  // Arm
  RoboCompKinovaArm::KinovaArmPrxPtr arm_left_proxy, arm_right_proxy;
  std::mutex arm_mutex;
  std::thread arm_worker;
  float left_arm[8]{};
  float right_arm[8]{};

  // Robot
  RoboCompOmniRobot::OmniRobotPrxPtr omnirobot_proxy;
  std::thread omnirobot_worker;
  std::mutex robot_mutex;
  RobotMiddleware::Pose robot_pose;
  bool poseChanged = false;

  RoboCompNavigator::NavigatorPrxPtr navigator_proxy;
  std::thread navigator_worker;
  std::mutex navigator_mutex;
  

  Impl() : communicator(makeInitData()) {
    try {
      auto ic = communicator.communicator();

      #ifdef P3BOT
        if (auto lidar3d_opt = require<RoboCompLidar3D::Lidar3DPrx,
                                      RoboCompLidar3D::Lidar3DPrxPtr>(
                ic, "lidar3d:tcp -h " + IP_ROBOT + " -p 12001", "Lidar3DProxy"))
          lidar3d_proxy = *lidar3d_opt;
        else {
          running = false;
          return;
        }
      #endif

      if (auto zed_opt = require<RoboCompLidar3D::Lidar3DPrx,
                                 RoboCompLidar3D::Lidar3DPrxPtr>(
              ic, "lidar3d:tcp -h " + IP_ROBOT + " -p 12000", "Lidar3DProxy"))
        zed_proxy = *zed_opt;
      else {
        running = false;
        return;
      }

      #ifdef P3BOT
        if (auto arm_left_opt = require<RoboCompKinovaArm::KinovaArmPrx,
                                      RoboCompKinovaArm::KinovaArmPrxPtr>(
                ic, "kinovaarm1:tcp -h " + IP_ROBOT + " -p 10006",
                "KinovaArmProxy"))
          arm_left_proxy = *arm_left_opt;
        else {
          running = false;
          return;
        }
      #endif

      if (auto arm_right_opt = require<RoboCompKinovaArm::KinovaArmPrx,
                                       RoboCompKinovaArm::KinovaArmPrxPtr>(
              ic, "kinovaarm:tcp -h " + IP_ROBOT + " -p 10005",
              "KinovaArmProxy"))
        arm_right_proxy = *arm_right_opt;
      else {
        running = false;
        return;
      }

      if (auto vrcontroller_opt =
              require<RoboCompVRController::VRControllerPrx,
                      RoboCompVRController::VRControllerPrxPtr>(
                  ic, "vrcontroller:tcp -h " + IP_ROBOT + " -p 12002",
                  "VRControllerProxy"))
        vrcontroller_proxy = *vrcontroller_opt;
      else {
        running = false;
        return;
      }

      #ifdef P3BOT
        if (auto omnirobot_opt = require<RoboCompOmniRobot::OmniRobotPrx,
                                        RoboCompOmniRobot::OmniRobotPrxPtr>(
                ic, "omnirobot:tcp -h " + IP_ROBOT + " -p 10004",
                "OmniRobotProxy"))
          omnirobot_proxy = *omnirobot_opt;
        else {
          running = false;
          return;
        }
      #endif


      #ifdef P3BOT
        if (auto navigator_opt = require<RoboCompNavigator::NavigatorPrx,
                                        RoboCompNavigator::NavigatorPrxPtr>(
                ic, "navigator:tcp -h " + IP_ROBOT + " -p 11000",
                "NavigatorProxy"))
          navigator_proxy = *navigator_opt;
        else {
          running = false;
          return;
        }
      #endif

    } catch (const Ice::Exception &ex) {
      std::cerr << "\033[31mERROR\033[0m Exception: 'rcnode' not running: "
                << ex << std::endl;
      std::cerr << "Ice exception: " << ex.what() << "\n";
      running = false;
    } catch (const std::exception &ex) {
      std::cerr << "\033[31mERROR\033[0m Impl failed: " << ex.what()
                << std::endl;
      running = false;
    } catch (...) {
      std::cerr << "\033[31mERROR\033[0m Exception: 'rcnode' not running: "
                << std::endl;
      std::cerr
          << "\033[31mERROR\033[0m Impl failed: unknown non-std::exception"
          << std::endl;
      running = false;
    }
    if (running) {
      lidar_worker = std::thread(&RobotMiddleware::Impl::updateLidarData, this);
      arm_worker = std::thread(&RobotMiddleware::Impl::updateRobotState, this);
      #ifdef P3BOT
        omnirobot_worker = std::thread(&RobotMiddleware::Impl::updateRobotPose, this);
      #endif
    }
  }
  
  ~Impl() {
    running = false;

    vrcontroller_proxy = nullptr;
    arm_right_proxy = nullptr;
    
    #ifdef P3BOT
      lidar3d_proxy = nullptr;
      arm_left_proxy = nullptr;
      omnirobot_proxy = nullptr;
      navigator_proxy = nullptr;
    #endif

    auto ic = communicator.communicator();
    if (ic) {
      ic->shutdown();
      ic->waitForShutdown();
    }

    if (lidar_worker.joinable())
      lidar_worker.join();
    if (arm_worker.joinable())
      arm_worker.join();
    if (omnirobot_worker.joinable())
      omnirobot_worker.join();
  }

  static Ice::InitializationData makeInitData() {
    Ice::InitializationData initData;
    initData.properties = Ice::createProperties();
    initData.properties->setProperty("Ice.Warn.Connections", "0");
    initData.properties->setProperty("Ice.Trace.Network", "0");
    initData.properties->setProperty("Ice.Trace.Protocol", "0");
    initData.properties->setProperty("Ice.MessageSizeMax", "100000000");
    return initData;
  }


////////////////UPDATE LOOPS////////////////
//ALL CALLS TO ICE MUST BE IN A TRY-CATCH BLOCK
  void updateLidarData() {
    using namespace std::chrono;
    const auto period = 50ms;
    const auto timeout = period * 5;
    std::deque<long long> zed_timestamps, lidar3d_timestamps;
    size_t max_size = 100;
    auto add_time = [max_size](std::deque<long long> &buffer, long long value) {
      if (buffer.size() == max_size)
        buffer.pop_front(); // quitar el más antiguo
      buffer.push_back(value);
    };

    auto mean_time = [](std::deque<long long> &buffer) {
      if (buffer.empty())
        return 0.0;
      long long sum = std::accumulate(buffer.begin(), buffer.end(), 0LL);
      return static_cast<double>(sum) / buffer.size();
    };

    while (running) {
      auto start = steady_clock::now();
      try {
        auto zed_future = zed_proxy->getColorCloudDataAsync();
        #ifdef P3BOT
          RobotMiddleware::ColorCloudData lidar_cloud;
          auto lidar3d_future = lidar3d_proxy->getColorCloudDataAsync();

          try {
            // auto start = std::chrono::high_resolution_clock::now();
            long long int lidar_timestamp;
            if (lidar3d_future.wait_for(timeout) == std::future_status::ready) {
              lidar_cloud = iceToCloudPoints(lidar3d_future.get(), lidar_timestamp);
              // auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
              //                     std::chrono::system_clock::now().time_since_epoch()).count();
              // add_time(lidar3d_timestamps, timestamp - lidar_timestamp);
              // std::cout << "\033[1;32mINFO\033[0m timestamps diff Lidar3D: "
              //           << mean_time(lidar3d_timestamps) << std::endl;
            } else {
              std::cout << "\033[1;33mWARNING\033[0m Lidar3D not ready\n";
            }

            // std::cout << "\033[32mINFO\033[0m " << "lidar getted, points "<<
            // lidar_cloud.X.size() << ", time "<<timestamp - start).count()<<
            // "\n";
          } catch (const Ice::ConnectionRefusedException &ex) {
            std::cerr << "\033[31mERROR\033[0m Lidar3D connection refused: "
                      << ex.what() << "\n";
          } catch (const Ice::ConnectionLostException &ex) {
            std::cerr << "\033[1;33mWARNING\033[0m Lidar3D connection lost: "
                      << ex.what() << "\n";
          } catch (const Ice::TimeoutException &ex) {
            std::cerr << "\033[1;33mWARNING\033[0m Lidar3D timeout: " << ex.what()
                      << "\n";
          } catch (const Ice::ObjectNotExistException &ex) {
            std::cerr << "\033[31mERROR\033[0m Lidar3D object not found: "
                      << ex.what() << "\n";
          } catch (const Ice::InvocationCanceledException &ex) {
            std::cerr << "\033[1;33mWARNING\033[0m Lidar3D invocation cancelled: "
                      << ex.what() << "\n";
          } catch (const Ice::Exception &ex) {
            std::cerr << "\033[1;33mWARNING\033[0m Lidar3D Ice exception: "
                      << ex.what() << "\n";
          } catch (const std::exception &ex) {
            std::cerr << "\033[1;33mWARNING\033[0m Lidar3D std::exception: "
                      << ex.what() << "\n";
          } catch (...) {
            std::cerr << "\033[1;33mWARNING\033[0m Failed getting Lidar data\n";
          }
        #endif

        RobotMiddleware::ColorCloudData zed_cloud;
        try {
          // auto start = std::chrono::high_resolution_clock::now();
          long long int zed_timestamp;
          if (zed_future.wait_for(timeout) == std::future_status::ready) {
            zed_cloud = iceToCloudPoints(zed_future.get(), zed_timestamp);
            // auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
            //                     std::chrono::system_clock::now().time_since_epoch()).count();
            // add_time(zed_timestamps, timestamp - zed_timestamp);
            // std::cout << "\033[1;32mINFO\033[0m timestamps diff zed: "
            //           << mean_time(zed_timestamps) << std::endl;
          } else {
            std::cout << "\033[1;33mWARNING\033[0m Zed not ready\n";
          }
          // std::cout << "\033[32mINFO\033[0m " << "zed getted, points "<<
          // zed_cloud.X.size() << ", time "<<timestamp - start).count()<< "\n";
        } catch (const Ice::ConnectionRefusedException &ex) {
          std::cerr << "\033[31mERROR\033[0m Zed connection refused: "
                    << ex.what() << "\n";
        } catch (const Ice::ConnectionLostException &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Zed connection lost: "
                    << ex.what() << "\n";
        } catch (const Ice::TimeoutException &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Zed timeout: " << ex.what()
                    << "\n";
        } catch (const Ice::ObjectNotExistException &ex) {
          std::cerr << "\033[31mERROR\033[0m Zed object not found: "
                    << ex.what() << "\n";
        } catch (const Ice::InvocationCanceledException &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Zed invocation cancelled: "
                    << ex.what() << "\n";
        } catch (const Ice::Exception &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Zed Ice exception: "
                    << ex.what() << "\n";
        } catch (const std::exception &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Zed std::exception: "
                    << ex.what() << "\n";
        } catch (...) {
          std::cerr << "\033[1;33mWARNING\033[0m Failed getting Zed data\n";
        }
        
        #ifdef P3BOT
        size_t total = zed_cloud.X.size() + lidar_cloud.X.size();
        zed_cloud.X.reserve(total);
        zed_cloud.Y.reserve(total);
        zed_cloud.Z.reserve(total);
        zed_cloud.R.reserve(total);
        zed_cloud.G.reserve(total);
        zed_cloud.B.reserve(total);

        zed_cloud.X.insert(zed_cloud.X.end(),
                           std::make_move_iterator(lidar_cloud.X.begin()),
                           std::make_move_iterator(lidar_cloud.X.end()));

        zed_cloud.Y.insert(zed_cloud.Y.end(),
                           std::make_move_iterator(lidar_cloud.Y.begin()),
                           std::make_move_iterator(lidar_cloud.Y.end()));

        zed_cloud.Z.insert(zed_cloud.Z.end(),
                           std::make_move_iterator(lidar_cloud.Z.begin()),
                           std::make_move_iterator(lidar_cloud.Z.end()));

        zed_cloud.R.insert(zed_cloud.R.end(),
                           std::make_move_iterator(lidar_cloud.R.begin()),
                           std::make_move_iterator(lidar_cloud.R.end()));

        zed_cloud.G.insert(zed_cloud.G.end(),
                           std::make_move_iterator(lidar_cloud.G.begin()),
                           std::make_move_iterator(lidar_cloud.G.end()));

        zed_cloud.B.insert(zed_cloud.B.end(),
                           std::make_move_iterator(lidar_cloud.B.begin()),
                           std::make_move_iterator(lidar_cloud.B.end()));
        #endif

        {
          std::unique_lock lock(lidar_mutex);
          cloudPoints = std::move(zed_cloud);
          ++cloudVersion;
        }
      } catch (const Ice::ConnectionRefusedException &ex) {
        std::cerr << "\033[31mERROR\033[0m Cloud data connection refused: "
                  << ex.what() << "\n";
      } catch (const Ice::ConnectionLostException &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m Cloud data connection lost: "
                  << ex.what() << "\n";
      } catch (const Ice::TimeoutException &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m Cloud data timeout: "
                  << ex.what() << "\n";
      } catch (const Ice::Exception &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m Cloud data Ice exception: "
                  << ex.what() << "\n";
      } catch (const std::exception &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m Cloud data std::exception: "
                  << ex.what() << "\n";
      } catch (...) {
        std::cerr << "\033[1;33mWARNING\033[0m Failed getting cloud data\n";
      }
      // Period
      auto elapsed = steady_clock::now() - start;
      if (elapsed < period)
        std::this_thread::sleep_for(period - elapsed);
    }
  }

  void updateRobotState() {
    using namespace std::chrono;
    const auto period = 50ms;
    const auto timeout = period;
    const double rad_to_deg = 180.0 / M_PI;
    RoboCompKinovaArm::TJoints left_joints, right_joints;

    #ifdef P3BOT
      left_joints.joints.resize(8);
      left_joints.timestamp = 0;
    #endif
    right_joints.joints.resize(8);
    right_joints.timestamp = 0;

    for (int i = 0; i < 8; ++i) {
      #ifdef P3BOT
        left_joints.joints[i].angle = 0.0;
      #endif
      right_joints.joints[i].angle = 0.0;
    }

    while (running) {
      auto start = steady_clock::now();
      try {
        // Llamadas asíncronas no bloqueantes
        #ifdef P3BOT
          auto left_future = arm_left_proxy->getJointsStateAsync();
        #endif
        auto right_future = arm_right_proxy->getJointsStateAsync();

        #ifdef P3BOT
          if (left_future.wait_for(timeout) == std::future_status::ready) {
            left_joints = left_future.get();
          }
        #endif
        if (right_future.wait_for(timeout) == std::future_status::ready) {
          right_joints = right_future.get();
        }
        // std::cout << "arms: ";
        {
          std::scoped_lock lock(arm_mutex);
          for (int i = 0; i < right_joints.joints.size(); ++i) {
            #ifdef P3BOT
              left_arm[i] = left_joints.joints[i].angle * rad_to_deg;
              // std::cout << left_arm[i] << " ";
            #endif
            right_arm[i] = right_joints.joints[i].angle * rad_to_deg;
            // std::cout << right_arm[i] << " ";

          }
          // std::cout << "\n";
        }
      } catch (const Ice::ConnectionRefusedException &ex) {
        std::cerr << "\033[31mERROR\033[0m Robot state connection refused: "
                  << ex.what() << "\n";
      } catch (const Ice::ConnectionLostException &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m Robot state connection lost: "
                  << ex.what() << "\n";
      } catch (const Ice::TimeoutException &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m Robot state timeout: "
                  << ex.what() << "\n";
      } catch (const Ice::ObjectNotExistException &ex) {
        std::cerr << "\033[31mERROR\033[0m Robot arm object not found: "
                  << ex.what() << "\n";
      } catch (const Ice::Exception &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m Robot state Ice exception: "
                  << ex.what() << "\n";
      } catch (const std::exception &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m Robot state std::exception: "
                  << ex.what() << "\n";
      } catch (...) {
        std::cerr << "\033[1;33mWARNING\033[0m Failed to update robot state\n";
      }

      // Period
      auto elapsed = steady_clock::now() - start;
      if (elapsed < period)
        std::this_thread::sleep_for(period - elapsed);
    }
  }

  void updateRobotPose() {
    #ifdef P3BOT
      using namespace std::chrono;
      const auto period = 20ms;
      std::future<RoboCompGenericBase::TBaseState> state_future;

      while (running) {
        auto start = steady_clock::now();

        // Lanzar nueva llamada solo si no hay una pendiente
        if (!state_future.valid()) {
          try {
            state_future = omnirobot_proxy->getBaseStateAsync();
          } catch (const Ice::Exception &ex) {
            std::cerr << "\033[31mERROR\033[0m Robot pose async launch failed: " << ex.what() << "\n";
          } catch (...) {
            std::cerr << "\033[31mERROR\033[0m Robot pose async launch failed\n";
          }
        }

        // Comprobar si la llamada pendiente ya tiene resultado (sin bloquear)
        if (state_future.valid() &&
            state_future.wait_for(0ms) == std::future_status::ready) {
          try {
            auto state = state_future.get();
            {
              std::unique_lock<std::mutex> lock(robot_mutex);
              robot_pose = angle2DToQuaternion(static_cast<float>(state.alpha), static_cast<float>(state.z)*100, static_cast<float>(state.x)*100, 0.0);
              poseChanged = true;
            }
          } catch (const Ice::ConnectionRefusedException &ex) {
            std::cerr << "\033[31mERROR\033[0m Robot pose connection refused: " << ex.what() << "\n";
          } catch (const Ice::ConnectionLostException &ex) {
            std::cerr << "\033[1;33mWARNING\033[0m Robot pose connection lost: " << ex.what() << "\n";
          } catch (const Ice::TimeoutException &ex) {
            std::cerr << "\033[1;33mWARNING\033[0m Robot pose timeout: " << ex.what() << "\n";
          } catch (const Ice::ObjectNotExistException &ex) {
            std::cerr << "\033[31mERROR\033[0m Robot pose object not found: " << ex.what() << "\n";
          } catch (const Ice::Exception &ex) {
            std::cerr << "\033[1;33mWARNING\033[0m Robot pose Ice exception: " << ex.what() << "\n";
          } catch (const std::exception &ex) {
            std::cerr << "\033[1;33mWARNING\033[0m Robot pose std::exception: " << ex.what() << "\n";
          } catch (...) {
            std::cerr << "\033[1;33mWARNING\033[0m Failed to update robot pose\n";
          }
          // Lanzar la siguiente llamada inmediatamente tras consumir el resultado
          try {
            state_future = omnirobot_proxy->getBaseStateAsync();
          } catch (...) {}
        }

        auto elapsed = steady_clock::now() - start;
        if (elapsed < period)
          std::this_thread::sleep_for(period - elapsed);
      }
    #endif
  }


  //////////GETTERS IMPLEMENTATIONS//////////
  //ALL CALLS TO ICE MUST BE IN A TRY-CATCH BLOCK
  
  bool getHapticData(RobotMiddleware::Haptic &left, RobotMiddleware::Haptic &right) {
    try {
      bool hapticChanged = false;
      std::scoped_lock<std::mutex> lock(haptic_mutex);
      if (haptics_future.valid() &&
          haptics_future.wait_for(std::chrono::milliseconds(5)) ==
              std::future_status::ready) {
        RoboCompVRController::Haptics haptics = haptics_future.get();
        left = iceToHaptic(haptics.left);
        right = iceToHaptic(haptics.right);
        hapticChanged = true;
      }
      return hapticChanged;
    } catch (const std::exception &ex) {
      std::cerr << "\033[1;33mWARNING\033[0m receiveHaptics std::exception: "
                << ex.what() << "\n";
      return false;
    } catch (...) {
      std::cerr << "\033[1;33mWARNING\033[0m Failed to receive haptics\n";
      return false;
    }
  }

  ///////////SETTERS IMPLEMENTATIONS///////////
  //ALL CALLS TO ICE MUST BE ASYNC
  //ALL CALLS TO ICE MUST BE IN A TRY-CATCH BLOCK

  bool sendVRData(
    const RobotMiddleware::Pose &head, const RobotMiddleware::Pose &left,
    const RobotMiddleware::Controller &leftController,
    const RobotMiddleware::Pose &right,
    const RobotMiddleware::Controller &rightController) {
    try {
      RoboCompVRController::Controller retLeft = toIceController(leftController);
      retLeft.pose = toIcePose(left);
      RoboCompVRController::Controller retRight = toIceController(rightController);
      retRight.pose = toIcePose(right);

      haptics_future = vrcontroller_proxy->sendDataReceiveHapticsAsync(
                                                toIcePose(head), retLeft, retRight);
      return true;

    } catch (const Ice::ConnectionRefusedException &ex) {
      std::cerr << "\033[31mERROR\033[0m sendData connection refused: "<< ex.what() << "\n";
      return false;
    } catch (const Ice::ConnectionLostException &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m sendData connection lost: "<< ex.what() << "\n";
        return false;
    } catch (const Ice::TimeoutException &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m sendData timeout: " << ex.what()<< "\n";
        return false;
    } catch (const Ice::ObjectNotExistException &ex) {
        std::cerr << "\033[31mERROR\033[0m sendData object not found: " << ex.what()<< "\n";
        return false;
    } catch (const Ice::Exception &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m sendData Ice exception: "<< ex.what() << "\n";
        return false;
    } catch (const std::exception &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m sendData std::exception: "<< ex.what() << "\n";
        return false;
    } catch (...) {
        std::cerr << "\033[1;33mWARNING\033[0m Failed sending Data controllers\n";
        return false;
    }
  }
  
  bool setBasePose(const RobotMiddleware::Pose &target){
    #ifdef P3BOT
       try {
          navigator_proxy->gotoPoseAsync(RoboCompNavigator::TPose(-target.y*10, target.x*10, getYawFromQuaternion(target.qrx, target.qry, target.qrz, target.qrw))); 
          return true;
        } catch (const Ice::ConnectionRefusedException &ex) {
          std::cerr << "\033[31mERROR\033[0m Set robot pose connection refused: "
                    << ex.what() << "\n";
        } catch (const Ice::ConnectionLostException &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Set robot pose connection lost: "
                    << ex.what() << "\n";
        } catch (const Ice::TimeoutException &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Set robot pose timeout: "
                    << ex.what() << "\n";
        } catch (const Ice::ObjectNotExistException &ex) {
          std::cerr << "\033[31mERROR\033[0m Set robot pose object not found: "
                    << ex.what() << "\n";
        } catch (const Ice::Exception &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Set robot pose Ice exception: "
                    << ex.what() << "\n";
        } catch (const std::exception &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Set robot pose std::exception: "
                    << ex.what() << "\n";
        } catch (...) {
          std::cerr << "\033[1;33mWARNING\033[0m Failed to set robot pose\n";
        }
    #endif
    return false;
  }

  bool setSpeedBase(float x, float y, float yaw){
    #ifdef P3BOT
       try {
          // std::cerr << "\033[32mINFO\033[0m " << "Set robot speed: " << x << " " << y << " " << yaw << "\n";
          omnirobot_proxy->setSpeedBaseAsync(x, y, yaw); 
          return true;
        } catch (const Ice::ConnectionRefusedException &ex) {
          std::cerr << "\033[31mERROR\033[0m Set robot speed connection refused: "
                    << ex.what() << "\n";
          return false;
        } catch (const Ice::ConnectionLostException &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Set robot speed connection lost: "
                    << ex.what() << "\n";
        } catch (const Ice::TimeoutException &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Set robot speed timeout: "
                    << ex.what() << "\n";
        } catch (const Ice::ObjectNotExistException &ex) {
          std::cerr << "\033[31mERROR\033[0m Set robot speed object not found: "
                    << ex.what() << "\n";
        } catch (const Ice::Exception &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Set robot speed Ice exception: "
                    << ex.what() << "\n";
        } catch (const std::exception &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Set robot speed std::exception: "
                    << ex.what() << "\n";
        } catch (...) {
          std::cerr << "\033[1;33mWARNING\033[0m Failed to set robot speed\n";
        }
    #endif
    return false;
  }

  bool stopBase(){
    #ifdef P3BOT
       try {
          navigator_proxy->stopAsync();
          omnirobot_proxy->setSpeedBaseAsync(0, 0, 0); 
          return true;
        } catch (const Ice::ConnectionRefusedException &ex) {
          std::cerr << "\033[31mERROR\033[0m Stop robot speed connection refused: "
                    << ex.what() << "\n";
          return false;
        } catch (const Ice::ConnectionLostException &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Stop robot speed connection lost: "
                    << ex.what() << "\n";
        } catch (const Ice::TimeoutException &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Stop robot speed timeout: "
                    << ex.what() << "\n";
        } catch (const Ice::ObjectNotExistException &ex) {
          std::cerr << "\033[31mERROR\033[0m Stop robot speed object not found: "
                    << ex.what() << "\n";
        } catch (const Ice::Exception &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Stop robot speed Ice exception: "
                    << ex.what() << "\n";
        } catch (const std::exception &ex) {
          std::cerr << "\033[1;33mWARNING\033[0m Stop robot speed std::exception: "
                    << ex.what() << "\n";
        } catch (...) {
          std::cerr << "\033[1;33mWARNING\033[0m Failed to stop robot speed\n";
        }
    #endif
    return false;
  }

  bool resetOdometer(){
    #ifdef P3BOT
      try {
        omnirobot_proxy->resetOdometerAsync();
        return true;
      } catch (const Ice::ConnectionRefusedException &ex) {
        std::cerr << "\033[31mERROR\033[0m Reset odometer connection refused: "
                  << ex.what() << "\n";
        return false;
      } catch (const Ice::ConnectionLostException &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m Reset odometer connection lost: "
                  << ex.what() << "\n";
      } catch (const Ice::TimeoutException &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m Reset odometer timeout: "
                  << ex.what() << "\n";
      } catch (const Ice::ObjectNotExistException &ex) {
        std::cerr << "\033[31mERROR\033[0m Reset odometer object not found: "
                  << ex.what() << "\n";
      } catch (const Ice::Exception &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m Reset odometer Ice exception: "
                  << ex.what() << "\n";
      } catch (const std::exception &ex) {
        std::cerr << "\033[1;33mWARNING\033[0m Reset odometer std::exception: "
                  << ex.what() << "\n";
      } catch (...) {
        std::cerr << "\033[1;33mWARNING\033[0m Failed to reset odometer\n";
      }
    #endif
    return false;
  }
};