cmake_minimum_required(VERSION 3.5)

set(project_name capture_qt)

project(${project_name} LANGUAGES CXX)

set(CMAKE_INCLUDE_CURRENT_DIR ON)

set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTORCC ON)

set(CMAKE_CXX_STANDARD 11)
set(CMAKE_CXX_STANDARD_REQUIRED ON)


# find the root of the main project
string(FIND ${PROJECT_SOURCE_DIR} "/" INDEX REVERSE)
string(SUBSTRING ${PROJECT_SOURCE_DIR} 0 ${INDEX} PR_ROOT)

message("PR_ROOT:" ${PR_ROOT})

#include(${PR_ROOT}/../../common/cmake_utils/use_cpp_11.cmake)
#include(${PR_ROOT}/../common/cmake_utils/use_sse_avx.cmake)

# look for any *.c and *.cpp files
file(GLOB SOURCES "src/*.cpp"
    "src/*.ui"
    "include/capture_gui.h"
    "${PR_ROOT}/common/include/control_driver.h"
    "${PR_ROOT}/common/include/spinnaker_utilities.h"
    )

set(project_includes
    "include"
    "${PR_ROOT}/common/include"
    )

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

# set(PROJECT_SOURCES
        # main.cpp
        # sr_control.cpp
        # sr_control.h
        # sr_control.ui
# )


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

set(pid_file "pid_config.txt")
file(COPY "${PR_ROOT}/common/${pid_file}" DESTINATION ${CMAKE_CURRENT_BINARY_DIR})

include_directories(${project_includes})

#if(${QT_VERSION_MAJOR} GREATER_EQUAL 6)
#    qt_add_executable(${project_name}
#        ${SOURCES}
#    )
#else()
    add_executable(${project_name}
            ${SOURCES}
    )
#endif()

target_link_libraries(${project_name} Qt5::Widgets Qt5::Core Qt5::Gui ${project_libs})
