#pragma once
#include <array>
#include <vector>
#include <memory>

const std::string IP_ROBOT  = "192.168.50.78";

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
    };

    struct ColorCloudData 
    {
    std::vector<short> X;
    std::vector<short> Y;
    std::vector<short> Z;
    std::vector<unsigned char> R;
    std::vector<unsigned char> G;
    std::vector<unsigned char> B;   
    };
    

    RobotMiddleware();
    ~RobotMiddleware();
    bool isRunning();

    bool sendPoses(const RobotMiddleware::Pose& head, const RobotMiddleware::Pose& left, const RobotMiddleware::Pose& right);
    bool sendControllers(const RobotMiddleware::Controller& left, const RobotMiddleware::Controller& right);
    bool getHaptics(RobotMiddleware::Haptic& left, RobotMiddleware::Haptic& right);
    const ColorCloudData& getColorCloudData();
    void lockUlockGetColorCloudData(bool lock);
    bool getRobotState(float (&left)[8], float (&right)[8]);

private:
    bool initIce();

    // ICE y detalles internos NO se exponen
    struct Impl;

    std::unique_ptr<Impl> pImpl;  // Puntero al "pImpl"
};