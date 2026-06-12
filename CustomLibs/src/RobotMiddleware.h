#pragma once
#include <memory>
#include <vector>

const std::string IP_ROBOT = "localhost";

class RobotMiddleware {
public:
  // ── Singleton ───────────────────────────────────────────────────
  static RobotMiddleware &getInstance(){
    static RobotMiddleware instance; // Se crea una sola vez, es thread-safe en C++11+
    return instance;
  }

  // No se permite copiar ni mover
  RobotMiddleware(const RobotMiddleware &) = delete;
  RobotMiddleware &operator=(const RobotMiddleware &) = delete;
  RobotMiddleware(RobotMiddleware &&) = delete;
  RobotMiddleware &operator=(RobotMiddleware &&) = delete;

  struct Pose {
    float x;
    float y;
    float z;
    float qrx;
    float qry;
    float qrz;
    float qrw;
  };

#pragma pack(push,1)          
struct Controller {
    float trigger;
    float grab;
    float x;
    float y;
    float thumbstickCapTouch;
    float aButtonCapTouch;      
    float bButtonCapTouch;      
    bool  bButton;              
    bool  aButton;  
    bool  thumbstickButton;           
};
#pragma pack(pop)    

  struct Haptic {
    float intensity;
    float frequency;
  };

  struct ColorCloudData {
    std::vector<short> X;
    std::vector<short> Y;
    std::vector<short> Z;
    std::vector<unsigned char> R;
    std::vector<unsigned char> G;
    std::vector<unsigned char> B;
  };

  using ArmJoint = float[8];

  bool isRunning();
  bool sendData(const RobotMiddleware::Pose &head,
                const RobotMiddleware::Pose &left,
                const RobotMiddleware::Controller &leftController,
                const RobotMiddleware::Pose &right,
                const RobotMiddleware::Controller &rightController);
  bool receiveHaptics(RobotMiddleware::Haptic &left,
                      RobotMiddleware::Haptic &right);
  const ColorCloudData &getColorCloudData();
  void lockUlockGetColorCloudData(bool lock);
  bool getRobotState(float (&left)[8], float (&right)[8]);
  bool getRobotPose(RobotMiddleware::Pose &robot);
  bool setSpeedBase(float x, float y, float yaw);
  bool setBasePose(const RobotMiddleware::Pose &target);
  bool stopBase();

private:
  RobotMiddleware();
  ~RobotMiddleware();

  bool initIce();

  // ICE y detalles internos NO se exponen
  struct Impl;

  std::unique_ptr<Impl> pImpl; // Puntero al "pImpl"
};