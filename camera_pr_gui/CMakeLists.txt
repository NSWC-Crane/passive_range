# ----------------------------------------------------------------------------
#  Root CMake file for Camera Capture Code
# ----------------------------------------------------------------------------
#
# To perform an out of source build do the following on the command line:
# 1. navigate to the root project directory (in this case DfD2)
# 2. mkdir build
# 3. cd build
# 4. depending on your system (windows/linux) type the following
# Example CMake build line for windows & VS2015
# windows: cmake -G "Visual Studio 14 2015 Win64" -DUSE_AVX_INSTRUCTIONS=ON ..
# linux: cmake -DUSE_AVX_INSTRUCTIONS=ON ..
#
# 5. To compile:
# windows: cmake --build . --config Release
# linux: cmake --build . --config Release -- -j4
#

cmake_minimum_required(VERSION 2.8.12)

PROJECT(cam_pr_gui_project)
set(exe_name "cam_pr_gui")

set(PLATFORM "$ENV{PLATFORM}" CACHE INTERNAL "Copied from environment variable")
if("${PLATFORM}" STREQUAL "")
    set(PLATFORM "NOT_SET")
    message("The PLATFORM Environment variable is not set on this system.  Setting it to:")
endif()
message("Platform: " ${PLATFORM})

# find the root of the main project
string(FIND ${PROJECT_SOURCE_DIR} "/" INDEX REVERSE)
string(SUBSTRING ${PROJECT_SOURCE_DIR} 0 ${INDEX} PR_ROOT)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Build Options
option(USE_AVX_INSTRUCTIONS  "Compile your program with AVX instructions"  ON)

if(WIN32)

    if(${PLATFORM} STREQUAL "Laptop")
        set(COMMON_ROOT "D:/Common")
        #set(DLIB_CONTRIB "D:/Projects/dlib-contrib")
        #set(DFD_COMMON_ROOT "D:/Projects/dfd_common")	
        #include(D:/dlib-19.22/dlib/cmake)
    elseif(${PLATFORM} STREQUAL "LaptopN")
        set(COMMON_ROOT "C:/Projects/Common")
        #set(DLIB_CONTRIB "C:/Projects/dlib-contrib")
        #set(DFD_COMMON_ROOT "C:/Projects/dfd_common")	
        #include(D:/dlib-19.22/dlib/cmake)
    elseif(${PLATFORM} STREQUAL "BEAST")
        set(COMMON_ROOT "E:/Projects/Common")
        #set(DLIB_CONTRIB "E:/Projects/dlib-contrib")
        #set(DFD_COMMON_ROOT "~/Projects/dfd_common")
        #include(E:/dlib-19.21/dlib/cmake)
    else()
        set(COMMON_ROOT "C:/Projects/Common")
        #set(DLIB_CONTRIB "C:/Projects/dlib-contrib")
        #set(DFD_COMMON_ROOT "C:/Projects/dfd_common")	
        #include(D:/dlib-19.19/dlib/cmake)
    endif()
    
    set(DFD_DNN_DIRS "${PR_ROOT}/common")
    set(DFD_DNN_LIBS "${PR_ROOT}/common/bin/dfd_dnn.lib")
    
    #find_library(DFD_DNN_LIBS dfd_dnn_d HINTS "${PR_ROOT}/common/bin/")

else()
    if(${PLATFORM} STREQUAL "MainGear")
        set(_ECLIPSE_VERSION "3.8")
        set(COMMON_ROOT "~/Projects/Common")
    elseif(${PLATFORM} STREQUAL "HPC")
        set(COMMON_ROOT "$HOME/Projects/Common")
    else()
        set(COMMON_ROOT "$HOME/Projects/Common")
    endif()

    set(DFD_DNN_DIRS "${PR_ROOT}/common")
    set(DFD_DNN_LIBS "${PR_ROOT}/common/bin/libdfd_dnn.so")
    
endif()

# look for any *.c and *.cpp files
file(GLOB SOURCES "src/*.cpp"
    "src/*.ui"
    "include/capture_pr_gui.h"
    "${PR_ROOT}/common/include/control_driver.h"
    "${PR_ROOT}/common/include/spinnaker_utilities.h"
    "${PR_ROOT}/common/include/dfd_dnn_lib.h"
    )
    
set(project_includes 
    "include" 
    "${PR_ROOT}/common/include"
    #"${DLIB_CONTRIB}/include"
    #"${DFD_COMMON_ROOT}/include"
    )
    
set(project_libs ${project_libs} ${DFD_DNN_LIBS})

message(" ")

include(${PR_ROOT}/common/cmake_utils/find_ftdi.cmake)
if(FTDI_FOUND)
    set(project_includes ${project_includes} ${FTDI_INCLUDE_DIRS})
    set(project_libs ${project_libs} ${FTDI_LIBS})
endif()

include(${PR_ROOT}/common/cmake_utils/find_spinnaker.cmake)
if(SPIN_FOUND)
    set(project_includes ${project_includes} ${SPIN_INCLUDE_DIRS})
    set(project_libs ${project_libs} ${SPIN_LIBS})
endif()

# find OpenCV
message(STATUS "--------------------------------------------------------------------------------")
message(STATUS "Looking for OpenCV installation...")
find_package(OpenCV REQUIRED)
if (OpenCV_FOUND)
	set(project_includes ${project_includes} ${OpenCV_INCLUDE_DIRS})
	set(project_libs ${project_libs} ${OpenCV_LIBS})
	message(STATUS "Found OpenCV: " ${OpenCV_DIR})
else()
	message("OpenCV not found...")
endif()
message(STATUS "--------------------------------------------------------------------------------")


find_package(Qt5 COMPONENTS Core Gui Widgets REQUIRED)

set(project_libs ${project_libs} Qt5::Widgets Qt5::Core Qt5::Gui)

message(" ")
message("--------------------------------------------------------------------------------")
message("Source Files: ")
foreach(S ${SOURCES})
    message("  " ${S})
endforeach()
message("--------------------------------------------------------------------------------")

message(" ")

message("--------------------------------------------------------------------------------")
message("Project Includes: ")
foreach(S ${project_includes})
    message("  " ${S})
endforeach()
message("--------------------------------------------------------------------------------")

message(" ")

message("--------------------------------------------------------------------------------")
message("Project Libraries: ")
foreach(S ${project_libs})
    message("  " ${S})
endforeach()
message("--------------------------------------------------------------------------------")

message(" ")

# copy the pid configuration file for the dynamixel motors
set(pid_file "pid_config.txt")
file(COPY "${PR_ROOT}/common/${pid_file}" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

# copy the linear stage calibration file 
#set(linear_stage_file "linear_stage_cal.txt")
#file(COPY "${PR_ROOT}/common/${linear_stage_file}" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

if(UNIX)
    SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pthread")
endif()

include_directories(${project_includes})
add_executable(${exe_name} ${SOURCES})
target_link_libraries(${exe_name} ${project_libs})

if(WIN32)
    message("MSVC Version: " ${MSVC_VERSION})
    # uncomment for MSVC and cmake greater than 3.6.0
    set(default_build_type "Release")
    if(MSVC AND (${CMAKE_VERSION} VERSION_GREATER "3.6.0"))
        target_compile_options(${exe_name} PRIVATE "/MP")
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${exe_name})
    endif()

endif()

message(" ")
message(STATUS "Build Complete!")
message(" ")
