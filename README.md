# VR_teleoperation



## Installation

1. Install [ALVR](https://github.com/alvr-org/ALVR)
2. Install[STEAM](https://store.steampowered.com/about/)
**Important**: download the official .deb package (do not use Flatpak or other distributions).
```bash
sudo apt install libopenxr-dev libopenxr-loader1 openxr-layer-corevalidation   
```

3.  Install [Unreal Engine](https://www.unrealengine.com/en-US/linux)
Example: `Linux_Unreal_Engine_5.6.1.zip`
```bash
cd Linux_Unreal_Engine_5.6.1/Engine/Binaries/Linux
./UnrealEditor
```

(Optional) You may need Vulkan drivers:
```bash
sudo apt install vulkan-tools libvulkan1 libvulkan-dev
sudo apt install vulkan-tools libvulkan1 libvulkan-dev vulkan-validationlayers
sudo apt install nvidia-vulkan-common vulkan-utils
```
To test Vulkan:
```bash
sudo apt install vkcube
vkcube
```

## Execution
Make sure to start the headset connection before running the project.

Open the project with:
```bash
./UnrealEditor ~/Code/VR_teleoperation/VRTeleoperation.uproject
```


## Development
### Compile Unreal project
```bash 
/home/robolab/software/Linux_Unreal_Engine_5.6.1/Engine/Build/BatchFiles/Linux/Build.sh VRTeleoperationEditor Linux Development /home/robolab/Documents/Unreal\ Projects/VRTeleoperation/VRTeleoperation.uproject
```

### Communication Library
Inside the CustomLibs folder, there is an ICE communication interface for RoboComp components.
To build it:
```bash
cd CustomLibs
slice2cpp VRController.ice
cmake -B build && make  -C build -j12

```
This generates a .so shared library inside build. Link it to the Unreal binaries with:
```bash
cd ..
ln -s $(pwd)/CustomLibs/build/libRobotMiddleware.so  Binaries/Linux/libRobotMiddleware.so
```

 