# VR Teleoperation Framework for Dual-Arm Manipulators
A professional Virtual Reality (VR) framework designed for the immersive control of dual-arm robotic manipulators. This system integrates Unreal Engine 5.6.1 and the Webots simulator to bridge the gap between simulated environments and real-world hardware.

## Overview
This project enables real-time teleoperation of robotic arms via VR controllers. By utilizing natural depth perception and motion tracking, users can manipulate tool points seamlessly across both virtual and physical platforms.
- Real-time Visualization: Powered by Unreal Engine 5.6.1.
- Dual-Domain Support: Unified interface for Webots simulator and physical robotic hardware.
- Bidirectional Communication: Low-latency control and feedback loop.
- Immersive Interface: High-fidelity depth perception and motion tracking for precise manipulation.

## Tested Environment
This framework has been developed and validated under the following configuration:
- **Operating System:** Ubuntu 24.04 LTS
- **Robotics Simulator:** Webots
- **Game Engine:** Unreal Engine 5.6.1
- **Communication:** ZeroC ICE / RoboComp
- **Robot Deployment:** [p3botUR](https://github.com/alfiTH/robocomp-p3bot/blob/main/tools/p3botUR.sh) Configuration Script (Used for automated setup of the robotic environment)

## Installation
1. **VR & OpenXR Setup**
The system relies on ALVR and OpenXR for headset communication.
- Install [ALVR](https://github.com/alvr-org/ALVR)
- Install [STEAM](https://store.steampowered.com/about/)(Official .deb package only; avoid Flatpak).
- OpenXR Drivers:
```bash
sudo apt install libopenxr-dev libopenxr-loader1 openxr-layer-corevalidation   
```

2. **Graphics (Vulkan)**
Ensure your drivers are up to date. To test your Vulkan installation:
```bash
sudo apt install vulkan-tools libvulkan1 libvulkan-dev vulkan-validationlayers nvidia-vulkan-common vulkan-utils
```
To test Vulkan:
```bash
sudo apt install vkcube
vkcube
```

3.  **Unreal Engine 5.6.1** 
    1. Download the Linux binaries ([Linux_Unreal_Engine_5.6.1.zip](https://www.unrealengine.com/en-US/linux))
    2. Extract to your preferred software directory (e.g., $HOME/software/).
    3. Run the editor to verify installation:
    ```bash
    cd Linux_Unreal_Engine_5.6.1/Engine/Binaries/Linux
    ./UnrealEditor
    ```

## Development & Building
### Communication Library (ICE/RoboComp)
The system uses a custom middleware to interface with RoboComp components via ZeroC ICE.
1. *Generate C++ classes from Slice files*:
```bash
cd CustomLibs/include
slice2cpp ../ices/KinovaArm.ice ../ices/Lidar3D.ice ../ices/VRController.ice  ../ices/GenericBase.ice ../ices/OmniRobot.ice
```
2. *Build the Shared Library*:
```bash
cd .. && cmake -B build && make  -C build -j12
```
3. *Link to Unreal Binaries*:
```bash
cd ..
ln -s $(pwd)/CustomLibs/build/libRobotMiddleware.so  Binaries/Linux/libRobotMiddleware.so
```

### Compiling the Unreal Project
To build the project from the terminal:
```bash 
~/software/Linux_Unreal_Engine_5.6.1/Engine/Build/BatchFiles/Linux/Build.sh VRTeleoperationEditor Linux Development ~/Code/VR_teleoperation/VRTeleoperation.uproject
```

### Execution
1. Headset: Ensure your VR headset is connected via ALVR/SteamVR.
2. Launch:
```bash
./UnrealEditor ~/Code/VR_teleoperation/VRTeleoperation.uproject
```

## Technical Notes
- Game Mode: Logic is defined in Blueprints/VRGameMode.
- Input Handling: Controls are mapped in Inputs/IMC_Robot.
- Robot Actor: The Robot(Pawn) is wrapped in RobotBP to facilitate Unreal's Enhanced Input System.
- Asset Pipeline (URDF to FBX):
    - Use Blender 3.1.2 with the urdf_importer.
    - Ensure urdf_parser_py and yamlpy are in Blender’s internal Python environment.
    - Import Settings: Right-click in Content Drawer -> Import -> Select FBX -> Set type to Skeletal Mesh.