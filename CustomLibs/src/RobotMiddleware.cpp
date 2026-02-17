#include "RobotMiddleware.h"
#include "RobotMiddlewareParsers.cpp"
#include <Ice/Ice.h> // solo aquí dentro
#include <IceStorm/IceStorm.h>
#include <cmath>
#include <deque>
#include <future>
#include <include/GenericBase.h>
#include <include/KinovaArm.h>
#include <include/Lidar3D.h>
#include <include/OmniRobot.h>
#include <include/VRController.h>
#include <memory>
#include <mutex>
#include <numeric>
#include <optional>
#include <shared_mutex>


#define ROBOCOMP

#ifdef ROBOCOMP
  #include "RobocompImpl.cpp"
#endif


constexpr double DEG2RAD = M_PI / 180.0;


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

    pImpl->haptics_future =
        pImpl->vrcontroller_proxy->sendDataReceiveHapticsAsync(
            toIcePose(head), retLeft, retRight);
    return true;
  } catch (const Ice::ConnectionRefusedException &ex) {
    std::cout << "\033[31mERROR\033[0m sendData connection refused: "
              << ex.what() << "\n";
    return false;
  } catch (const Ice::ConnectionLostException &ex) {
    std::cout << "\033[1;33mWARNING\033[0m sendData connection lost: "
              << ex.what() << "\n";
    return false;
  } catch (const Ice::TimeoutException &ex) {
    std::cout << "\033[1;33mWARNING\033[0m sendData timeout: " << ex.what()
              << "\n";
    return false;
  } catch (const Ice::ObjectNotExistException &ex) {
    std::cout << "\033[31mERROR\033[0m sendData object not found: " << ex.what()
              << "\n";
    return false;
  } catch (const Ice::InvocationCanceledException &ex) {
    std::cout << "\033[1;33mWARNING\033[0m sendData invocation cancelled: "
              << ex.what() << "\n";
    return false;
  } catch (const Ice::Exception &ex) {
    std::cout << "\033[1;33mWARNING\033[0m sendData Ice exception: "
              << ex.what() << "\n";
    return false;
  } catch (const std::exception &ex) {
    std::cout << "\033[1;33mWARNING\033[0m sendData std::exception: "
              << ex.what() << "\n";
    return false;
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
  } catch (const Ice::ConnectionRefusedException &ex) {
    std::cout << "\033[31mERROR\033[0m receiveHaptics connection refused: "
              << ex.what() << "\n";
  } catch (const Ice::ConnectionLostException &ex) {
    std::cout << "\033[1;33mWARNING\033[0m receiveHaptics connection lost: "
              << ex.what() << "\n";
  } catch (const Ice::TimeoutException &ex) {
    std::cout << "\033[1;33mWARNING\033[0m receiveHaptics timeout: "
              << ex.what() << "\n";
  } catch (const Ice::Exception &ex) {
    std::cout << "\033[1;33mWARNING\033[0m receiveHaptics Ice exception: "
              << ex.what() << "\n";
  } catch (const std::exception &ex) {
    std::cout << "\033[1;33mWARNING\033[0m receiveHaptics std::exception: "
              << ex.what() << "\n";
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
    } catch (const std::system_error &ex) {
      std::cout << "\033[1;33mWARNING\033[0m getColorCloudData mutex error: "
                << ex.what() << "\n";
    } catch (const std::exception &ex) {
      std::cout << "\033[1;33mWARNING\033[0m getColorCloudData std::exception: "
                << ex.what() << "\n";
    } catch (...) {
      std::cout << "\033[1;33mWARNING\033[0m Failed to get cloud data\n";
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
  } catch (const std::system_error &ex) {
    std::cout << "\033[1;33mWARNING\033[0m getRobotState mutex error: "
              << ex.what() << "\n";
    return false;
  } catch (const std::exception &ex) {
    std::cout << "\033[1;33mWARNING\033[0m getRobotState std::exception: "
              << ex.what() << "\n";
    return false;
  } catch (...) {
    std::cout << "\033[1;33mWARNING\033[0m Failed to get robot state\n";
    return false;
  }
}

bool RobotMiddleware::getRobotPose(RobotMiddleware::Pose &robot) {
  if (!pImpl)
    return false;
  try {
    {
      std::scoped_lock lock(pImpl->robot_mutex);
      robot = pImpl->robot_pose;
    }
    return true;
  } catch (const std::system_error &ex) {
    std::cout << "\033[1;33mWARNING\033[0m getRobotPose mutex error: "
              << ex.what() << "\n";
    return false;
  } catch (const std::exception &ex) {
    std::cout << "\033[1;33mWARNING\033[0m getRobotPose std::exception: "
              << ex.what() << "\n";
    return false;
  } catch (...) {
    std::cout << "\033[1;33mWARNING\033[0m Failed to get robot pose\n";
    return false;
  }
}

