# Passive Range Project

This project contains the project files for the Passive Range Project.


## Dependencies

The code in this repository has the following dependecies

1. [CMake 2.8.12+](https://cmake.org/download/)
2. [FTDI D2XX Drivers](https://www.ftdichip.com/Drivers/D2XX.htm)
3. [OpenCV v4+](https://opencv.org/releases/)
4. [Spinnaker SDK](https://www.flir.com/products/spinnaker-sdk/)
5. [MPLAB X IDE v4+](https://www.microchip.com/mplab/mplab-x-ide)
6. [MPLAB XC32/32++ Compiler v2+](https://www.microchip.com/mplab/compilers)

## Repo Breakdown

### camera_capture

This folder contains the project code to capture images from the camera and adjust the motors to change the zoom and focus.

### common

This folder contains code that is common across multiple projects.

### drawings

This folder contains the design files for the mechanical interfaces between the optics and motors.

### md.X

This folder contains the firmware code for the motor driver.

### motor_cli

This folder contains the code for a simple motor controller command line interface (cli)



