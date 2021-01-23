message(" ")
message(STATUS "Looking for FlyCapture installation...")

find_path(FC2_INCLUDE_DIRS FlyCapture2.h
    HINTS ENV FC2PATH
    PATHS /usr/local "D:/Point Grey Research/FlyCapture2" "C:/Program Files/Point Grey Research/FlyCapture2" ENV CPATH 
    PATH_SUFFIXES include
    )

find_library(FC2_LIBS FlyCapture2_v140
    HINTS ENV FC2PATH 
    PATHS /usr/local "D:/Point Grey Research/FlyCapture2" "C:/Program Files/Point Grey Research/FlyCapture2"
    PATH_SUFFIXES lib64/vs2015 lib x64
    )
mark_as_advanced(FC2_LIBS FC2_INCLUDE_DIRS)

if (FC2_LIBS AND FC2_INCLUDE_DIRS)
    set(FC2_FOUND TRUE)
    add_compile_definitions(USE_FC2)
else()
    message("--- FlyCapture Drivers not found! ---")
    set(FC2_FOUND FALSE)
endif()