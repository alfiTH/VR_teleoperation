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



    RobotMiddleware();
    ~RobotMiddleware();

    bool initIce();


    bool sendPose(const RobotMiddleware::Pose& head, const RobotMiddleware::Pose& left, const RobotMiddleware::Pose& right);
    bool sendControllers(const RobotMiddleware::Controller& left, const RobotMiddleware::Controller& right);
    bool getRobotState(/* ... */);

    std::vector<std::array<float, 3>> getLidarData();
private:
    // ICE y detalles internos NO se exponen
    struct Impl;
    Impl* pImpl;  // Puntero al "pImpl"
};