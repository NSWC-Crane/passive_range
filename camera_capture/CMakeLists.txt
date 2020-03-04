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

PROJECT(cam_ctrl_project)
set(exe_name "cam_ctrl")

set(PLATFORM "$ENV{PLATFORM}" CACHE INTERNAL "Copied from environment variable")

message("Platform: " ${PLATFORM})

# find the root of the main project
string(FIND ${PROJECT_SOURCE_DIR} "/" INDEX REVERSE)
string(SUBSTRING ${PROJECT_SOURCE_DIR} 0 ${INDEX} PR_ROOT)

if(${PLATFORM} STREQUAL "Laptop")
    set(COMMON_ROOT "D:/Common")
elseif(${PLATFORM} STREQUAL "MainGear")
    set(_ECLIPSE_VERSION "3.8")
    set(COMMON_ROOT "~/Common")
elseif(${PLATFORM} STREQUAL "HPC")
    set(COMMON_ROOT "$HOME/Common")
elseif(${PLATFORM} STREQUAL "LaptopN")
    set(COMMON_ROOT "C:/Projects/Common")
else()
    set(COMMON_ROOT "D:/Common")
endif()

include(${COMMON_ROOT}/cmake_utils/use_cpp_11.cmake)
include(${COMMON_ROOT}/cmake_utils/use_sse_avx.cmake)

# look for any *.c and *.cpp files
file(GLOB SOURCES "src/*.c*"
    "${COMMON_ROOT}/src/*.cpp"
    )

set(project_includes 
    "include" 
    "${PR_ROOT}/common/include"
    "${COMMON_ROOT}/include"
    )

message(" ")
include(${COMMON_ROOT}/cmake_utils/find_ftdi.cmake)
if(FTDI_FOUND)
    set(project_includes ${project_includes} ${FTDI_INCLUDE_DIRS})
    set(project_libs ${project_libs} ${FTDI_LIBS})
endif()

include(${COMMON_ROOT}/cmake_utils/find_spinnaker.cmake)
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
else()
	message("OpenCV not found...")
endif()
message(STATUS "--------------------------------------------------------------------------------")

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
