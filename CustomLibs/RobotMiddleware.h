#pragma once
#include <array>

struct Pose
{
    float x;
    float y;
    float z;
    float rx;
    float ry;
    float rz;
};

class RobotMiddleware {
public:
    RobotMiddleware();
    ~RobotMiddleware();

    bool initIce();


    bool sendPose(const Pose& head, const Pose& left, const Pose& right);
    bool getRobotState(/* ... */);
private:
    // ICE y detalles internos NO se exponen
    struct Impl;
    Impl* pImpl;  // Puntero al "pImpl"
};