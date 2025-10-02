#pragma once
#include <array>
#include <vector>

class RobotMiddleware {
public:

    struct Pose
    {
        float x;
        float y;
        float z;
        float rx;
        float ry;
        float rz;
        float qrx;
        float qry;
        float qrz;
        float qrw;

    };

    struct Controller
    {
        float trigger;
        float grab;
        float x;
        float y;
        float thumbstickCapTouch;
        bool aButton;
        float aButtonCapTouch;
        bool bButton;
        float bButtonCapTouch;
    };

    struct Haptic
    {
        float intensity;
        float frequency;
        float duration;
    };
    

    RobotMiddleware();
    ~RobotMiddleware();

    bool initIce();
    bool sendPoses(const RobotMiddleware::Pose& head, const RobotMiddleware::Pose& left, const RobotMiddleware::Pose& right);
    bool sendControllers(const RobotMiddleware::Controller& left, const RobotMiddleware::Controller& right);
    bool getHaptics(RobotMiddleware::Haptic& left, RobotMiddleware::Haptic& right);
    std::vector<std::array<float, 3>> getLidarData();
    bool getRobotState(float (&left)[8], float (&right)[8]);

private:
    // ICE y detalles internos NO se exponen
    struct Impl;
    Impl* pImpl = nullptr;  // Puntero al "pImpl"
};