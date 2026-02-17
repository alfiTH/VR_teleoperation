#include "RobotMiddleware.h"
#include <include/GenericBase.h>
#include <include/KinovaArm.h>
#include <include/Lidar3D.h>
#include <include/OmniRobot.h>
#include <include/VRController.h>

#include <Ice/Communicator.h>
#include <IceStorm/IceStorm.h>
#include <optional>
#include <mutex>
#include <thread>
#include <atomic>
#include <chrono>
#include <numeric>
#include <deque>
#include <shared_mutex>

#include "RobotMiddleware.tpp"

struct RobotMiddleware::Impl {
  std::atomic<bool> running{true};
  Ice::CommunicatorHolder communicator;

  RoboCompVRController::VRControllerPrxPtr vrcontroller_proxy;
  std::future<RoboCompVRController::Haptics> haptics_future;
  std::mutex haptic_mutex;

  // Lidar
  RoboCompLidar3D::Lidar3DPrxPtr lidar3d_proxy, zed_proxy;
  std::shared_timed_mutex lidar_mutex;
  std::thread lidar_worker;
  RobotMiddleware::ColorCloudData cloudPoints;

  // Arm
  RoboCompKinovaArm::KinovaArmPrxPtr arm_left_proxy, arm_right_proxy;
  std::mutex arm_mutex;
  std::thread arm_worker;
  float left_arm[8]{};
  float right_arm[8]{};

  // Robot
  std::mutex robot_mutex;
  RoboCompOmniRobot::OmniRobotPrxPtr omnirobot_proxy;
  std::mutex omnirobot_mutex;
  std::thread omnirobot_worker;
  RobotMiddleware::Pose robot_pose;

  Impl() : communicator(makeInitData()) {
    try {
      auto ic = communicator.communicator();

      if (auto lidar3d_opt = require<RoboCompLidar3D::Lidar3DPrx,
                                     RoboCompLidar3D::Lidar3DPrxPtr>(
              ic, "lidar3d:tcp -h " + IP_ROBOT + " -p 12001", "Lidar3DProxy"))
        lidar3d_proxy = *lidar3d_opt;
      else {
        running = false;
        return;
      }

      if (auto zed_opt = require<RoboCompLidar3D::Lidar3DPrx,
                                 RoboCompLidar3D::Lidar3DPrxPtr>(
              ic, "lidar3d:tcp -h " + IP_ROBOT + " -p 12000", "Lidar3DProxy"))
        zed_proxy = *zed_opt;
      else {
        running = false;
        return;
      }

      if (auto arm_left_opt = require<RoboCompKinovaArm::KinovaArmPrx,
                                      RoboCompKinovaArm::KinovaArmPrxPtr>(
              ic, "kinovaarm1:tcp -h " + IP_ROBOT + " -p 10006",
              "KinovaArmProxy"))
        arm_left_proxy = *arm_left_opt;
      else {
        running = false;
        return;
      }

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

      if (auto omnirobot_opt = require<RoboCompOmniRobot::OmniRobotPrx,
                                       RoboCompOmniRobot::OmniRobotPrxPtr>(
              ic, "omnirobot:tcp -h " + IP_ROBOT + " -p 10004",
              "OmniRobotProxy"))
        omnirobot_proxy = *omnirobot_opt;
      else {
        running = false;
        return;
      }

    } catch (const Ice::Exception &ex) {
      std::cout << "\033[31mERROR\033[0m Exception: 'rcnode' not running: "
                << ex << std::endl;
      std::cerr << "Ice exception: " << ex.what() << "\n";
      running = false;
    } catch (const std::exception &ex) {
      std::cout << "\033[31mERROR\033[0m Impl failed: " << ex.what()
                << std::endl;
      running = false;
    } catch (...) {
      std::cout << "\033[31mERROR\033[0m Exception: 'rcnode' not running: "
                << std::endl;
      std::cout
          << "\033[31mERROR\033[0m Impl failed: unknown non-std::exception"
          << std::endl;
      running = false;
    }
    if (running) {
      lidar_worker = std::thread(&RobotMiddleware::Impl::updateLidarData, this);
      arm_worker = std::thread(&RobotMiddleware::Impl::updateRobotState, this);
      omnirobot_worker = std::thread(&RobotMiddleware::Impl::updateRobotPose, this);
    }
  }
  ~Impl() {
    running = false;

    vrcontroller_proxy = nullptr;
    lidar3d_proxy = nullptr;
    arm_left_proxy = nullptr;
    arm_right_proxy = nullptr;
    omnirobot_proxy = nullptr;

    auto ic = communicator.communicator();
    if (ic) {
      ic->shutdown();
      ic->waitForShutdown();
    }

    if (lidar_worker.joinable())
      lidar_worker.join();
    if (arm_worker.joinable())
      arm_worker.join();
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
        auto lidar3d_future = lidar3d_proxy->getColorCloudDataAsync();
        RobotMiddleware::ColorCloudData lidar_cloud;
        RobotMiddleware::ColorCloudData zed_cloud;
        try {
          // auto start = std::chrono::high_resolution_clock::now();
          long long int lidar_timestamp;
          if (lidar3d_future.wait_for(timeout) == std::future_status::ready) {
            lidar_cloud = iceToCloudPoints(lidar3d_future.get(), lidar_timestamp);
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch()).count();
            add_time(lidar3d_timestamps, timestamp - lidar_timestamp);
            std::cout << "\033[1;32mINFO\033[0m timestamps diff Lidar3D: "
                      << mean_time(lidar3d_timestamps) << std::endl;
          } else {
            std::cout << "\033[1;33mWARNING\033[0m Lidar3D not ready\n";
          }

          // std::cout << "\033[32mINFO\033[0m " << "lidar getted, points "<<
          // lidar_cloud.X.size() << ", time "<<timestamp - start).count()<<
          // "\n";
        } catch (const Ice::ConnectionRefusedException &ex) {
          std::cout << "\033[31mERROR\033[0m Lidar3D connection refused: "
                    << ex.what() << "\n";
        } catch (const Ice::ConnectionLostException &ex) {
          std::cout << "\033[1;33mWARNING\033[0m Lidar3D connection lost: "
                    << ex.what() << "\n";
        } catch (const Ice::TimeoutException &ex) {
          std::cout << "\033[1;33mWARNING\033[0m Lidar3D timeout: " << ex.what()
                    << "\n";
        } catch (const Ice::ObjectNotExistException &ex) {
          std::cout << "\033[31mERROR\033[0m Lidar3D object not found: "
                    << ex.what() << "\n";
        } catch (const Ice::InvocationCanceledException &ex) {
          std::cout << "\033[1;33mWARNING\033[0m Lidar3D invocation cancelled: "
                    << ex.what() << "\n";
        } catch (const Ice::Exception &ex) {
          std::cout << "\033[1;33mWARNING\033[0m Lidar3D Ice exception: "
                    << ex.what() << "\n";
        } catch (const std::exception &ex) {
          std::cout << "\033[1;33mWARNING\033[0m Lidar3D std::exception: "
                    << ex.what() << "\n";
        } catch (...) {
          std::cout << "\033[1;33mWARNING\033[0m Failed getting Lidar data\n";
        }

        try {
          // auto start = std::chrono::high_resolution_clock::now();
          long long int zed_timestamp;
          if (zed_future.wait_for(timeout) == std::future_status::ready) {
            zed_cloud = iceToCloudPoints(zed_future.get(), zed_timestamp);
            auto timestamp = std::chrono::duration_cast<std::chrono::milliseconds>(
                                std::chrono::system_clock::now().time_since_epoch()).count();
            add_time(zed_timestamps, timestamp - zed_timestamp);
            std::cout << "\033[1;32mINFO\033[0m timestamps diff zed: "
                      << mean_time(zed_timestamps) << std::endl;
          } else {
            std::cout << "\033[1;33mWARNING\033[0m Zed not ready\n";
          }
          // std::cout << "\033[32mINFO\033[0m " << "zed getted, points "<<
          // zed_cloud.X.size() << ", time "<<timestamp - start).count()<< "\n";
        } catch (const Ice::ConnectionRefusedException &ex) {
          std::cout << "\033[31mERROR\033[0m Zed connection refused: "
                    << ex.what() << "\n";
        } catch (const Ice::ConnectionLostException &ex) {
          std::cout << "\033[1;33mWARNING\033[0m Zed connection lost: "
                    << ex.what() << "\n";
        } catch (const Ice::TimeoutException &ex) {
          std::cout << "\033[1;33mWARNING\033[0m Zed timeout: " << ex.what()
                    << "\n";
        } catch (const Ice::ObjectNotExistException &ex) {
          std::cout << "\033[31mERROR\033[0m Zed object not found: "
                    << ex.what() << "\n";
        } catch (const Ice::InvocationCanceledException &ex) {
          std::cout << "\033[1;33mWARNING\033[0m Zed invocation cancelled: "
                    << ex.what() << "\n";
        } catch (const Ice::Exception &ex) {
          std::cout << "\033[1;33mWARNING\033[0m Zed Ice exception: "
                    << ex.what() << "\n";
        } catch (const std::exception &ex) {
          std::cout << "\033[1;33mWARNING\033[0m Zed std::exception: "
                    << ex.what() << "\n";
        } catch (...) {
          std::cout << "\033[1;33mWARNING\033[0m Failed getting Zed data\n";
        }

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
        {
          std::unique_lock<std::shared_timed_mutex> lock(lidar_mutex);
          cloudPoints = std::move(zed_cloud);
        }
      } catch (const Ice::ConnectionRefusedException &ex) {
        std::cout << "\033[31mERROR\033[0m Cloud data connection refused: "
                  << ex.what() << "\n";
      } catch (const Ice::ConnectionLostException &ex) {
        std::cout << "\033[1;33mWARNING\033[0m Cloud data connection lost: "
                  << ex.what() << "\n";
      } catch (const Ice::TimeoutException &ex) {
        std::cout << "\033[1;33mWARNING\033[0m Cloud data timeout: "
                  << ex.what() << "\n";
      } catch (const Ice::Exception &ex) {
        std::cout << "\033[1;33mWARNING\033[0m Cloud data Ice exception: "
                  << ex.what() << "\n";
      } catch (const std::exception &ex) {
        std::cout << "\033[1;33mWARNING\033[0m Cloud data std::exception: "
                  << ex.what() << "\n";
      } catch (...) {
        std::cout << "\033[1;33mWARNING\033[0m Failed getting cloud data\n";
      }
      // Period
      auto elapsed = steady_clock::now() - start;
      if (elapsed < period)
        std::this_thread::sleep_for(period - elapsed);
    }
  }

  void updateRobotState() {
    using namespace std::chrono;
    const auto period = 20ms;
    const auto timeout = period / 2;
    const double rad_to_deg = 180.0 / M_PI;
    RoboCompKinovaArm::TJoints left_joints, right_joints;

    left_joints.joints.resize(8);
    right_joints.joints.resize(8);

    for (int i = 0; i < 8; ++i) {
      left_joints.joints[i].angle = 0.0;
      right_joints.joints[i].angle = 0.0;

      left_joints.timestamp = 0;
      right_joints.timestamp = 0;
    }

    while (running) {
      auto start = steady_clock::now();
      try {
        // Llamadas asíncronas no bloqueantes
        auto left_future = arm_left_proxy->getJointsStateAsync();
        auto right_future = arm_right_proxy->getJointsStateAsync();

        if (left_future.wait_for(timeout) == std::future_status::ready) {
          left_joints = left_future.get();
        }
        if (right_future.wait_for(timeout) == std::future_status::ready) {
          right_joints = right_future.get();
        }

        {
          std::scoped_lock lock(arm_mutex);
          for (int i = 0; i < 8; ++i) {
            left_arm[i] = left_joints.joints[i].angle * rad_to_deg;
            right_arm[i] = right_joints.joints[i].angle * rad_to_deg;
          }
        }
      } catch (const Ice::ConnectionRefusedException &ex) {
        std::cout << "\033[31mERROR\033[0m Robot state connection refused: "
                  << ex.what() << "\n";
      } catch (const Ice::ConnectionLostException &ex) {
        std::cout << "\033[1;33mWARNING\033[0m Robot state connection lost: "
                  << ex.what() << "\n";
      } catch (const Ice::TimeoutException &ex) {
        std::cout << "\033[1;33mWARNING\033[0m Robot state timeout: "
                  << ex.what() << "\n";
      } catch (const Ice::ObjectNotExistException &ex) {
        std::cout << "\033[31mERROR\033[0m Robot arm object not found: "
                  << ex.what() << "\n";
      } catch (const Ice::Exception &ex) {
        std::cout << "\033[1;33mWARNING\033[0m Robot state Ice exception: "
                  << ex.what() << "\n";
      } catch (const std::exception &ex) {
        std::cout << "\033[1;33mWARNING\033[0m Robot state std::exception: "
                  << ex.what() << "\n";
      } catch (...) {
        std::cout << "\033[1;33mWARNING\033[0m Failed to update robot state\n";
      }

      // Period
      auto elapsed = steady_clock::now() - start;
      if (elapsed < period)
        std::this_thread::sleep_for(period - elapsed);
    }
  }
   void updateRobotPose() {
    using namespace std::chrono;
    const auto period = 20ms;
    const auto timeout = period / 2;
    RoboCompGenericBase::TBaseState state;
    while (running) {
      auto start = steady_clock::now();
      try {
        omnirobot_proxy->getBaseState(state); 
        robot_pose = angle2DToQuaternion(state.alpha, state.x, state.z, 0);
      } catch (const Ice::ConnectionRefusedException &ex) {
        std::cout << "\033[31mERROR\033[0m Robot pose connection refused: "
                  << ex.what() << "\n";
      } catch (const Ice::ConnectionLostException &ex) {
        std::cout << "\033[1;33mWARNING\033[0m Robot pose connection lost: "
                  << ex.what() << "\n";
      } catch (const Ice::TimeoutException &ex) {
        std::cout << "\033[1;33mWARNING\033[0m Robot pose timeout: "
                  << ex.what() << "\n";
      } catch (const Ice::ObjectNotExistException &ex) {
        std::cout << "\033[31mERROR\033[0m Robot arm object not found: "
                  << ex.what() << "\n";
      } catch (const Ice::Exception &ex) {
        std::cout << "\033[1;33mWARNING\033[0m Robot pose Ice exception: "
                  << ex.what() << "\n";
      } catch (const std::exception &ex) {
        std::cout << "\033[1;33mWARNING\033[0m Robot pose std::exception: "
                  << ex.what() << "\n";
      } catch (...) {
        std::cout << "\033[1;33mWARNING\033[0m Failed to update robot pose\n";
      }

      // Period
      auto elapsed = steady_clock::now() - start;
      if (elapsed < period)
        std::this_thread::sleep_for(period - elapsed);
    }
  }

};