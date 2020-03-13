# Motor Command Line Interface (CLI) Project

This project contains the project files for a simple command line interface to the motor controller to test the motors.


## Dependencies

The code in this repository has the following dependecies:

1. [CMake 2.8.12+](https://cmake.org/download/)
2. [FTDI D2XX Drivers](https://www.ftdichip.com/Drivers/D2XX.htm)
3. [davemers0160 common code repository](https://github.com/davemers0160/Common)

Follow the instruction for each of the dependencies according to your operating system.  For the FTDI drivers on a Linux based system an additional script needs to be run to add a udev rule that allows users without elevated privaledges to access the USB device.  In a terminla window run the following script in the main repository:

```
sudo sh ftdi_config.sh
```

## Build

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

To run the code enter the following:

Windows:

```
motor_cli
```

Linux

```
./motor_cli
```

Once running and the motor controller has been selected a set of control commands will be provided.

```
Motor Driver CLI Commands:
  ? - print this menu
  q - quit
  e <0/1> - enable (1)/disable (0) motors
  f <step> - step the focus motor, use '-' for CCW otherwise CW
  z <step> - step the zoom motor, use '-' for CCW otherwise CW
  x - zero the motor counter
```

