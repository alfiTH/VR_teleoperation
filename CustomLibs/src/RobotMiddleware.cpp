#include "RobotMiddleware.h"
#include <memory>
#include <mutex>
#include <shared_mutex>

#ifdef ROBOCOMP
  #include "RobocompMiddleware.cpp"
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
  if (!pImpl)
      return false;

  try {
    return pImpl->sendVRData(head, left, leftController, right, rightController);
  } catch (const std::exception &ex) {
    std::cout << "\033[1;33mWARNING\033[0m getRobotState std::exception: "
              << ex.what() << "\n";
    return false;
  } catch (...) {
    std::cout << "\033[1;33mWARNING\033[0m Failed to get robot state\n";
    return false;
  }
}

bool RobotMiddleware::receiveHaptics(RobotMiddleware::Haptic &left,
                                     RobotMiddleware::Haptic &right) {
  if (!pImpl)
    return false;
  try {
    return pImpl->getHapticData(left, right);
  } catch (const std::exception &ex) {
    std::cout << "\033[1;33mWARNING\033[0m receiveHaptics std::exception: "
              << ex.what() << "\n";
    return false;
  } catch (...) {
    std::cout << "\033[1;33mWARNING\033[0m Failed to receive haptics\n";
    return false;
  }
}

const RobotMiddleware::ColorCloudData &RobotMiddleware::getColorCloudData() {
  static const RobotMiddleware::ColorCloudData empty{};
  if (!pImpl) {
    return empty;
  }
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
      std::cout << "getRobotState: ";
      for (int i = 0; i < 8; ++i) {
        left[i] = pImpl->left_arm[i];
        right[i] = pImpl->right_arm[i];
        std::cout << left[i] << " " << right[i] << " ";
      }
      std::cout << "\n";
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
  if (!pImpl or !pImpl->poseChanged)
    return false;
  try {
    {
      std::scoped_lock lock(pImpl->robot_mutex);
      robot = pImpl->robot_pose;
      pImpl->poseChanged = false;
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

bool RobotMiddleware::setBasePose(const RobotMiddleware::Pose &target){
  if (!pImpl)
    return false;  
  try {
        pImpl->setBasePose(target);
        return true;
    } catch (const std::exception &ex) {
        std::cout << "\033[1;33mWARNING\033[0m setBasePose std::exception: "
                << ex.what() << "\n";
        return false;
    } catch (...) {
        std::cout << "\033[1;33mWARNING\033[0m Failed to set base pose\n";
        return false;
    }
}

bool RobotMiddleware::setSpeedBase(float x, float y, float yaw){
  if (!pImpl)
    return false;  
  try {
    pImpl->setSpeedBase(x, y, yaw);
    return true;
    } catch (const std::exception &ex) {
        std::cout << "\033[1;33mWARNING\033[0m setSpeedBase std::exception: "
                << ex.what() << "\n";
        return false;
    } catch (...) {
        std::cout << "\033[1;33mWARNING\033[0m Failed to set speed base\n";
        return false;
    }
}

bool RobotMiddleware::stopBase(){
  if (!pImpl)
    return false;  
  try {
    pImpl->stopBase();
    return true;
    } catch (const std::exception &ex) {
        std::cout << "\033[1;33mWARNING\033[0m stopBase std::exception: "
                << ex.what() << "\n";
        return false;
    } catch (...) {
        std::cout << "\033[1;33mWARNING\033[0m Failed to stop base\n";
        return false;
    }
}

