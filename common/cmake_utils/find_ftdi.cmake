message(STATUS "--------------------------------------------------------------------------------")
message(STATUS "Looking for FTDI D2XX drivers...")

find_path(FTDI_INCLUDE_DIRS ftd2xx.h
    PATHS /usr/local "D:/CDM v2.12.28 WHQL Certified" "C:/CDM v2.12.28 WHQL Certified" ENV CPATH 
    PATH_SUFFIXES include
    )

find_library(FTDI_LIBS ftd2xx
    HINTS ${FTDI_INCLUDE_DIRS}
    PATHS /usr/local "D:/CDM v2.12.28 WHQL Certified" "C:/CDM v2.12.28 WHQL Certified"
    PATH_SUFFIXES lib amd64 lib64 x64 
    )
    
mark_as_advanced(FTDI_LIBS FTDI_INCLUDE_DIRS)

if (FTDI_LIBS AND FTDI_INCLUDE_DIRS)
    set(FTDI_FOUND TRUE)
	add_compile_definitions(USE_FTDI)
	message(STATUS "Found FTDI Library: " ${FTDI_LIBS})
else()
    message("--- FTDI drivers were not found! ---")
    message("--- Drviers can be found at https://www.ftdichip.com/Drivers/D2XX.htm ---")
    set(FTDI_FOUND FALSE)
endif()

message(STATUS "--------------------------------------------------------------------------------")
message(STATUS " ")
