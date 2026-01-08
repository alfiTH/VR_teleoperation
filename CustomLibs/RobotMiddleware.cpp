#include "RobotMiddleware.h"
#include <include/CloudCompressor.h>
#include <Ice/Ice.h> // solo aquí dentro
#include <IceStorm/IceStorm.h>
#include <include/KinovaArm.h>
#include <include/Lidar3D.h>
#include <include/VRController.h>
#include <cmath>
#include <deque>
#include <future>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <shared_mutex>

#pragma region Templates

template <typename SubInterfaceType>
bool subscribe(const Ice::CommunicatorPtr &communicator,
               const IceStorm::TopicManagerPrxPtr &topicManager,
               const std::string &endpointConfig, std::string name_topic,
               const std::string &topicBaseName,
               std::shared_ptr<SubInterfaceType> servant,
               std::shared_ptr<IceStorm::TopicPrx> &topic,
               Ice::ObjectPrxPtr &proxy) {
  bool ok = true;
  try {
    if (!name_topic.empty())
      name_topic += "/";
    name_topic += topicBaseName;

    Ice::ObjectAdapterPtr adapter =
        communicator->createObjectAdapterWithEndpoints(name_topic,
                                                       endpointConfig);
    proxy = adapter->addWithUUID(servant)->ice_oneway();

    try {
      std::cout << "\033[32mINFO\033[0m " << "creating topic\n";
      topic = topicManager->create(name_topic);
    } catch (...) {
      try {
        std::cout << "\033[1;33mWARNING\033[0m subscribing the " << name_topic
                  << " topic. It's possible that other component have created, "
                     "retrieving\n";
        topic = topicManager->retrieve(name_topic);
      } catch (...) {
        std::cout << "\033[31mERROR\033[0m subscribing the " << name_topic
                  << "unknown non-std::exception" << std::endl;
        ok = false;
      }
      IceStorm::QoS qos;
      topic->subscribeAndGetPublisher(qos, proxy);
    }
    adapter->activate();
  } catch (...) {
    std::cout << "\033[31mERROR\033[0m subscribing the " << name_topic
              << " to created adapter, endpoint: " <<endpointConfig<< std::endl;
    ok = false;
  }
  return ok;
}

template <typename PubProxyType, typename PubProxyPointer>
std::optional<PubProxyPointer>
publish(const IceStorm::TopicManagerPrxPtr &topicManager,
        std::string name_topic, const std::string &topicBaseName) {
  if (!name_topic.empty())
    name_topic += "/";
  name_topic += topicBaseName;
  std::shared_ptr<IceStorm::TopicPrx> topic;
  try {
    std::cout << "\033[32mINFO\033[0m " << "creating topic\n";
    topic = topicManager->retrieve(name_topic);
  } catch (...) {
    try {
      std::cout << "\033[1;33mWARNING\033[0m publishing the " << name_topic
                << " topic. It's possible that other component have created, "
                   "retrieving\n";
      topic = topicManager->create(name_topic);
    } catch (...) {
      std::cout << "\033[31mERROR\033[0m publishing the " << name_topic
                << "unknown non-std::exception" << std::endl;
      return std::nullopt;
    }
  }

  if (!topic)
    return std::nullopt;

  try {

    auto publisher = topic->getPublisher();
    if (!publisher) {
      std::cout << "\033[31mERROR\033[0m  Publisher is null for topic "
                << name_topic << "\n";
      return std::nullopt;
    }
    auto pubProxy = Ice::uncheckedCast<PubProxyType>(publisher->ice_oneway());
    if (!pubProxy) {
      std::cout << "\033[31mERROR\033[0m  uncheckedCast failed for topic "
                << name_topic << "\n";
      return std::nullopt;
    }
    std::cout << "\033[32mINFO\033[0m " << name_topic << " initialized Ok!\n";
    return pubProxy;

  } catch (const Ice::Exception &ex) {
    std::cout << "\033[31mERROR\033[0m Failed to create publisher: " << ex
              << std::endl;
    return std::nullopt;
  } catch (...) {
    std::cout << "\033[31mERROR\033[0m Failed to create publisher: unknown "
                 "non-std::exception\n";
    return std::nullopt;
  }
};

template <typename ProxyType, typename ProxyPointer>
std::optional<ProxyPointer> require(const Ice::CommunicatorPtr &communicator,
                                    const std::string &proxyConfig,
                                    const std::string &proxyName) {
  try {
    ProxyPointer proxy =
        Ice::uncheckedCast<ProxyType>(communicator->stringToProxy(proxyConfig));
    std::cout << "\033[32mINFO\033[0m " << proxyName << " initialized Ok!\n";
    return proxy;
  } catch (const Ice::Exception &ex) {
    std::cout << "\033[31mERROR\033[0m Exception creating proxy " << proxyName
              << ": " << ex;
    return std::nullopt;
  }
}
#pragma endregion Templates

constexpr double DEG2RAD = M_PI / 180.0;
#pragma region Parsers

inline RoboCompVRController::Pose toIcePose(const RobotMiddleware::Pose &p) {
  return {p.x * 10, p.y * 10, p.z * 10, p.qrx, p.qry, p.qrz, p.qrw};
}
inline RoboCompVRController::Controller
toIceController(const RobotMiddleware::Controller &c) {
  return {RoboCompVRController::Pose(),
          c.trigger,
          c.grab,
          c.x,
          c.y,
          c.thumbstickCapTouch,
          c.aButton,
          c.aButtonCapTouch,
          c.bButton,
          c.bButtonCapTouch};
}

inline RobotMiddleware::ColorCloudData
iceToCloudPoints(RoboCompLidar3D::TColorCloudData &&cloudIn,
                 long long int &timestamp) {
  RobotMiddleware::ColorCloudData cloudOut;
  if (cloudIn.compressed) {
    size_t total_bytes = sizeof(cloudIn);
    total_bytes += cloudIn.cX.size() * sizeof(uint8_t);
    total_bytes += cloudIn.cY.size() * sizeof(uint8_t);
    total_bytes += cloudIn.cZ.size() * sizeof(uint8_t);
    total_bytes += cloudIn.R.size() * sizeof(uint8_t);
    total_bytes += cloudIn.G.size() * sizeof(uint8_t);
    total_bytes += cloudIn.B.size() * sizeof(uint8_t);
    std::cout << "\033[32mINFO\033[0m " << " bytes cloud in: " << total_bytes
              << "\n";

    auto init = std::chrono::high_resolution_clock::now();
    CloudCompressor::decompress(cloudIn);
    std::chrono::duration<double, std::milli> compress =
        std::chrono::high_resolution_clock::now() - init;
    size_t total_bytes_compress = sizeof(cloudIn);
    total_bytes_compress += cloudIn.X.size() * sizeof(short);
    total_bytes_compress += cloudIn.Y.size() * sizeof(short);
    total_bytes_compress += cloudIn.Z.size() * sizeof(short);
    total_bytes_compress += cloudIn.R.size() * sizeof(uint8_t);
    total_bytes_compress += cloudIn.G.size() * sizeof(uint8_t);
    total_bytes_compress += cloudIn.B.size() * sizeof(uint8_t);
    std::cout << "\033[32mINFO\033[0m " << "descompresss, points "
              << cloudIn.X.size() << "decompress size" << total_bytes_compress
              << ", time " << compress.count() << "\n";
  }

  cloudOut.X = std::move(cloudIn.X);
  cloudOut.Y = std::move(cloudIn.Y);
  cloudOut.Z = std::move(cloudIn.Z);
  cloudOut.R = std::move(cloudIn.R);
  cloudOut.G = std::move(cloudIn.G);
  cloudOut.B = std::move(cloudIn.B);

  timestamp = cloudIn.timestamp;
  return cloudOut;
}
inline RobotMiddleware::Haptic
iceToHaptic(const RoboCompVRController::Haptic &haptic) {
  return {haptic.intensity, haptic.frequency};
}

#pragma endregion Parsers

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
  std::thread arm_worker;
  RoboCompKinovaArm::KinovaArmPrxPtr arm_left_proxy, arm_right_proxy;
  std::mutex arm_mutex;
  float left_arm[8]{};
  float right_arm[8]{};

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
              ic, "kinovaarm:tcp -h " + IP_ROBOT + " -p 10006",
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

      if (auto vrcontroller_opt = require<RoboCompVRController::VRControllerPrx,
                                          RoboCompVRController::VRControllerPrxPtr>(
                  ic, "vrcontroller:tcp -h " + IP_ROBOT + " -p 12002",
                  "VRControllerProxy"))
        vrcontroller_proxy = *vrcontroller_opt;
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

    lidar_worker = std::thread(&RobotMiddleware::Impl::updateLidarData, this);
    arm_worker = std::thread(&RobotMiddleware::Impl::updateRobotState, this);
  }
  ~Impl() {
    running = false;

    vrcontroller_proxy = nullptr;
    lidar3d_proxy = nullptr;
    arm_left_proxy = nullptr;
    arm_right_proxy = nullptr;

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
    initData.properties->setProperty("Ice.MessageSizeMax", "20004800");
    return initData;
  }
  void updateLidarData() {
    using namespace std::chrono;
    const auto period = 50ms;
    const auto timeout = period*5;
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
          }
          else {
            std::cout << "\033[1;33mWARNING\033[0m Lidar3D not ready\n";
          }
          
          // std::cout << "\033[32mINFO\033[0m " << "lidar getted, points "<<
          // lidar_cloud.X.size() << ", time "<<timestamp - start).count()<<
          // "\n";
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
          }
          else {
            std::cout << "\033[1;33mWARNING\033[0m Zed not ready\n";
          }
          // std::cout << "\033[32mINFO\033[0m " << "zed getted, points "<<
          // zed_cloud.X.size() << ", time "<<timestamp - start).count()<< "\n";
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
    const auto timeout = period/2;
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
      } catch (...) {
        std::cout << "\033[1;33mWARNING\033[0m Failed to update robot state\n";
      }

      // Period
      auto elapsed = steady_clock::now() - start;
      if (elapsed < period)
        std::this_thread::sleep_for(period - elapsed);
    }
  }
};

RobotMiddleware::RobotMiddleware() { initIce(); }
RobotMiddleware::~RobotMiddleware() {}
bool RobotMiddleware::initIce() {
  this->pImpl = std::make_unique<Impl>();
  return isRunning();
}

bool RobotMiddleware::isRunning() { return this->pImpl->running; }

bool RobotMiddleware::sendData(
    const RobotMiddleware::Pose &head, const RobotMiddleware::Pose &left,
    const RobotMiddleware::Controller &leftController,
    const RobotMiddleware::Pose &right,
    const RobotMiddleware::Controller &rightController) {
  try {
    auto retLeft = toIceController(leftController);
    retLeft.pose = toIcePose(left);
    auto retRight = toIceController(rightController);
    retRight.pose = toIcePose(right);
    
    pImpl->haptics_future = pImpl->vrcontroller_proxy->sendDataReceiveHapticsAsync(
                                    toIcePose(head), retLeft, retRight);
    return true;
  } catch (...) {
    std::cout << "\033[1;33mWARNING\033[0m Failed sending Data controllers\n";
    return false;
  }
}

bool RobotMiddleware::receiveHaptics(RobotMiddleware::Haptic &left,
                                     RobotMiddleware::Haptic &right) {
  if (!pImpl)
    return false;

  bool hapticChanged = false;
  try {
    std::scoped_lock<std::mutex> lock(pImpl->haptic_mutex);
    if (pImpl->haptics_future.valid() &&
        pImpl->haptics_future.wait_for(std::chrono::milliseconds(5)) ==
            std::future_status::ready) {
      auto haptics = pImpl->haptics_future.get();
      left = iceToHaptic(haptics.left);
      right = iceToHaptic(haptics.right);
      hapticChanged = true;
    }
  } catch (...) {
    std::cout << "\033[1;33mWARNING\033[0m Failed to receive haptics\n";
  }
  return hapticChanged;
}

const RobotMiddleware::ColorCloudData &RobotMiddleware::getColorCloudData() {
  if (pImpl) {
    try {
      std::scoped_lock lock(pImpl->lidar_mutex);
      return pImpl->cloudPoints;
    } catch (...) {
      std::cout << "\033[1;33mWARNING\033[0m Failed to get robot state\n";
    }
  }
  static RobotMiddleware::ColorCloudData empty;
  return empty;
}
void RobotMiddleware::lockUlockGetColorCloudData(bool lock) {
  static thread_local std::shared_lock<std::shared_timed_mutex> readLock;
  if (lock) {
    readLock = std::shared_lock<std::shared_timed_mutex>(pImpl->lidar_mutex);
  } else {
    readLock.unlock();
  }
}

bool RobotMiddleware::getRobotState(float (&left)[8], float (&right)[8]) {
  if (!pImpl)
    return false;
  try {
    {
      std::scoped_lock lock(pImpl->arm_mutex);
      for (int i = 0; i < 8; ++i) {
        left[i] = pImpl->left_arm[i];
        right[i] = pImpl->right_arm[i];
      }
    }
    return true;
  } catch (...) {
    std::cout << "\033[1;33mWARNING\033[0m Failed to get robot state\n";
    return false;
  }
}
