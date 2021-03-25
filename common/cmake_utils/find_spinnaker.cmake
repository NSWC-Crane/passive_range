message(STATUS "--------------------------------------------------------------------------------")
message(STATUS "Looking for Spinnaker installation...")

find_path(SPIN_INCLUDE_DIRS Spinnaker.h
    HINTS ENV SPIN_PATH
    PATHS /usr /usr/local "C:/Program Files/Point Grey Research/Spinnaker" "D:/Program Files/Point Grey Research/Spinnaker" ENV CPATH 
    PATH_SUFFIXES include include/spinnaker
    )
    
if(WIN32)
    find_library(SPIN_LIBS Spinnaker_v140
        HINTS ENV SPIN_PATH 
        PATHS "C:/Program Files/Point Grey Research/Spinnaker" "D:/Program Files/Point Grey Research/Spinnaker"
        PATH_SUFFIXES lib64/vs2015 lib x64
        )

else()
    find_library(SPIN_LIBS Spinnaker
            HINTS ENV SPIN_PATH 
            PATHS /usr/lib /usr/local
            )
endif()

mark_as_advanced(SPIN_LIBS SPIN_INCLUDE_DIRS)

if (SPIN_LIBS AND SPIN_INCLUDE_DIRS)
    set(SPIN_FOUND TRUE)
    add_definitions(USE_SPINNAKER)
	message(STATUS "Found Spinnaker Library: " ${SPIN_LIBS})
else()
    message("--- Spinnaker drivers not found! ---")
    set(SPIN_FOUND FALSE)
endif()

message(STATUS "--------------------------------------------------------------------------------")
message(STATUS " ")
