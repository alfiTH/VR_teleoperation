    #include <functional>
	#include <VRControllerPub.h>


class VRControllerPubI : public RoboCompVRControllerPub::VRControllerPub
{
public:
	using ControllersCallback = std::function<void(RoboCompVRControllerPub::Controller,
                                                   RoboCompVRControllerPub::Controller)>;
    using HapticsCallback     = std::function<void(RoboCompVRControllerPub::Haptic,
                                                   RoboCompVRControllerPub::Haptic)>;
    using PosesCallback       = std::function<void(RoboCompVRControllerPub::Pose,
                                                   RoboCompVRControllerPub::Pose,
                                                   RoboCompVRControllerPub::Pose)>;

    VRControllerPubI(ControllersCallback onControllers,
                     HapticsCallback onHaptics,
                     PosesCallback onPoses)
        : onControllers_(std::move(onControllers)),
          onHaptics_(std::move(onHaptics)),
          onPoses_(std::move(onPoses)) {}

    ~VRControllerPubI() override = default;

    // --- Implementación de la interfaz Ice ---
    void sendControllers(RoboCompVRControllerPub::Controller left,
                         RoboCompVRControllerPub::Controller right,
                         const Ice::Current&)
    {
        if (onControllers_) onControllers_(left, right);
    }

    void sendHaptics(RoboCompVRControllerPub::Haptic left,
                     RoboCompVRControllerPub::Haptic right,
                     const Ice::Current&)
    {
        if (onHaptics_) onHaptics_(left, right);
    }

    void sendPoses(RoboCompVRControllerPub::Pose head,
                   RoboCompVRControllerPub::Pose left,
                   RoboCompVRControllerPub::Pose right,
                   const Ice::Current&)
    {
        if (onPoses_) onPoses_(head, left, right);
    }

private:
    ControllersCallback onControllers_;
    HapticsCallback onHaptics_;
    PosesCallback onPoses_;
};
