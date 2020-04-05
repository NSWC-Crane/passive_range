# Camera  Project

This project contains the project files for the Passive Range Project.


## Dependencies

The code in this repository has the following dependecies:

1. [CMake 2.8.12+](https://cmake.org/download/)
2. [FTDI D2XX Drivers](https://www.ftdichip.com/Drivers/D2XX.htm)
3. [OpenCV v4+](https://opencv.org/releases/)
4. [Spinnaker SDK](https://www.flir.com/products/spinnaker-sdk/)
5. [davemers0160 common code repository](https://github.com/davemers0160/Common)

Follow the instruction for each of the dependencies according to your operating system.  For the FTDI drivers on a Linux based system an additional script needs to be run to add a udev rule that allows users without elevated privaledges to access the USB device.  In a terminal window run the following script in the main repository:

```
sudo sh ftdi_config.sh
```

You will also need to copy the following files from the ftdi unziped folder:

```
sudo cp ftdid2xx.h /usr/include/.
sudo cp WinTypes.h /usr/include/.
```

## Build

The project uses CMake as the pmrimary mechanism to build the executables.  There are some modifications that may have to be made to the CMakeLists.txt file in order to get the project to build successfully.

The first thing that must be done is to create an environment variable called "PLATFORM".  The CMakeLists.txt file uses this variable to determine where to look for the other required repositories and/or libraries.  These will be machine specific.

To create an environment variable in Windows (drop the -m if you do not have elevated privileges):
```
setx -m PLATFORM MY_PC
```

In Linux (usually placed in .profile or .bashrc):
```
export PLATFORM=MY_PC
```

In the CMakeLists.txt file make sure to add a check for the platform you've added and point to the right locations for the repositories/libraries.

### Windows

Execute the following commands in a Windows command window:

```
mkdir build
cd build
cmake -G "Visual Studio 15 2017 Win64" -T host=x64 ..
cmake --build . --config Release
```

Or you can use the cmake-gui and set the "source code" location to the location of the CmakeLists.txt file and the set the "build" location to the build folder. 

### Linux

Execute the following commands in a terminal window:

```
mkdir build
cd build
cmake ..
cmake --build . --config Release -- -j4
```

Or you can use the cmake-gui and set the "source code" location to the location of the CmakeLists.txt file and the set the "build" location to the build folder. Then open a terminal window and navigate to the build folder and execute the follokwing command:

```
cmake --build . --config Release -- -j4
```

The -- -j4 tells the make to use 4 cores to build the code.  This number can be set to as many cores as you have on your PC.

## Running

To run the code you have two options.  The first is to supply individual parameters described in the table below.  For parameters that were not supplied at runtime the default values will be used.


parameter | default value | description |
| --- | --- | --- |
help h ?   |  | Display Usage message
cfg_file   |  | Alternate input method to supply all parameters, all parameters must be included in the file
focus_step | 0:216:21384 | voltage step range
x_off      | 8 | X offset for camera
y_off      | 4 | Y offset for camera
width      | 1264 | Width of the captured image
height     | 1020 | Height of the captured image
sharpness  | 3072 | Sharpness setting for the camera
fps        | 10.0 | Frames per second setting for the camera
exp_time   | 15000:-2000:1000 | Exposure time (us) range settings for the camera
gain       | 5.0 | Inital gain setting before letting the camera find one
avg        | 11 | Number of images to capture for an average
source     | 1  | source for the trigger (0 -> Line0, 1 -> Software)
output     | ../results/       | Output directory to save lidar images


To supply the parameters at runtime they can be called in the following manner:

```
executable -x_off=0 -fps=5.1
```

The second method (preferred) is to supply all of the parameters in a single file.  Using this method all of the input parametes must be supplied and they must be in the order outlined in the example file *cam_config.txt*

To use the file enter the following:

```
executable -cfg_file=../cam_config.txt
```

It is important to note that if the output folder does not exist the program will run, but there may not be any indication that the data is not being saved.
