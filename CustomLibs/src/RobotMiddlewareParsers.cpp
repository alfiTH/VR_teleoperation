#include "RobotMiddleware.h"
#include <include/GenericBase.h>
#include <include/KinovaArm.h>
#include <include/Lidar3D.h>
#include <include/OmniRobot.h>
#include <include/VRController.h>
#include <include/CloudCompressor.h>
#include <cmath>

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

inline RobotMiddleware::Pose angle2DToQuaternion(float angle2D, float x, float y, float z) {
    RobotMiddleware::Pose p{};
    p.x = x;
    p.y = y;
    p.z = z;

    // Conversión de ángulo sobre Z a Cuaternión
    p.qrw = std::cos(angle2D / 2.0f);
    p.qrx = 0.0f;
    p.qry = 0.0f;
    p.qrz = std::sin(angle2D / 2.0f);

    return p;
}