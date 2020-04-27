# ----------------------------------------------------------------------------
#  Root CMake file for the MNIST Dgigit Classification Library
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
# linux: cmake --build . --config Release -- -j10
#

cmake_minimum_required(VERSION 3.9)

project(sn_lib VERSION 1.0 DESCRIPTION "Shared library for an MNIST network with internal taps to view network during operation")
set(CMAKE_CXX_STANDARD 11)

set(LIB_NAME "sn_lib")
set(default_build_type "Release")

set(PLATFORM "$ENV{PLATFORM}" CACHE INTERNAL "Copied from environment variable")
if("${PLATFORM}" STREQUAL "")
    set(PLATFORM "NOT_SET")
    message("The PLATFORM Environment variable is not set on this system.  Setting it to:")
endif()
message("Platform: " ${PLATFORM})

# Build Options
option(USE_AVX_INSTRUCTIONS  "Compile your program with AVX instructions"  ON)
option(BUILD_LIB  "Build the project into a library (.dll/.so) vs a standalone executable"  ON)


if(WIN32)
    if(${PLATFORM} STREQUAL "Laptop")
        set(COMMON_ROOT "D:/Common")
    elseif(${PLATFORM} STREQUAL "LaptopN")
        set(COMMON_ROOT "C:/Projects/Common")
    elseif(${PLATFORM} STREQUAL "BEAST")
        set(COMMON_ROOT "D:/Common")
    else()
        set(COMMON_ROOT "D:/Common")
    endif()
    
else()
    set(HOME "$ENV{HOME}" CACHE INTERNAL "Copied from environment variable")
    
    if(${PLATFORM} STREQUAL "HPC")
        set(COMMON_ROOT "${HOME}/Projects/Common")
    elseif(${PLATFORM} STREQUAL "MainGear")
        set(COMMON_ROOT "${HOME}/Projects/Common")
    else()
        set(COMMON_ROOT "${HOME}/Projects/Common")
    endif()
    
endif()

include(${COMMON_ROOT}/cmake_utils/use_cpp_11.cmake)
include(${COMMON_ROOT}/cmake_utils/use_sse_avx.cmake)

# get the source files for the Library
file(GLOB SOURCES "src/*.cpp")

set(project_includes "include" 
    "${COMMON_ROOT}/include" 
    )

#if(BUILD_LIB AND UNIX)
#    set(SOURCES ${SOURCES} "${DLIB_ROOT}/dlib/all/source.cpp")
#    set(project_includes ${project_includes} ${DLIB_ROOT})
#endif()
     

message(" ")
message("--------------------------------------------------------")
message("Source Files: ")
foreach(S ${SOURCES})
    message("  " ${S})
endforeach()
message("--------------------------------------------------------")

message(" ")

message("--------------------------------------------------------")
message("Project Includes: ")
foreach(S ${project_includes})
    message("  " ${S})
endforeach()
message("--------------------------------------------------------")
message(" ")

include_directories(${project_includes})

if(BUILD_LIB)
    message("Building Simplex Noise Library...")
    
    add_definitions(-DBUILD_LIB)

    if(WIN32)

        add_definitions(-DSN_DLL_EXPORTS)
        
        add_library(${LIB_NAME} SHARED ${SOURCES})
        set_target_properties(${LIB_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
        #target_link_libraries(${LIB_NAME} dlib::dlib)
        
        if(MSVC AND (${CMAKE_VERSION} VERSION_GREATER "3.6.0"))
            message("MSVC Version: " ${MSVC_VERSION})
            target_compile_options(${PROJECT_NAME} PUBLIC "/MP" "/bigobj")
            set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
        endif() 
    
    elseif(UNIX)
                
        # Change these to point to the right location for a particular machine
        #add_definitions("-fPIC")
        
        find_package(X11 REQUIRED)
        add_library(${LIB_NAME} SHARED ${SOURCES})
        set_target_properties(${LIB_NAME} PROPERTIES VERSION ${PROJECT_VERSION})
        target_link_libraries(${LIB_NAME} ${X11_LIBRARIES})
        #target_link_libraries(${LIB_NAME} dlib::dlib ${X11_LIBRARIES})
        #target_link_libraries(${LIB_NAME} dlib::dlib)
        
    elseif(APPLE)
        message(FATAL_ERROR "Apple -> Not Even!")
        
    endif()

else()
    message("Building MNIST executable...")

    add_executable(${LIB_NAME} ${SOURCES})
    #target_link_libraries(${LIB_NAME} dlib::dlib)
    
    if(MSVC AND (${CMAKE_VERSION} VERSION_GREATER "3.6.0"))
        message("MSVC Version: " ${MSVC_VERSION})
        target_compile_options(${PROJECT_NAME} PUBLIC "/MP" "/bigobj")
        set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})
    endif() 
    
endif()


message(" ")
message(STATUS "Build Complete!")
message(" ")



