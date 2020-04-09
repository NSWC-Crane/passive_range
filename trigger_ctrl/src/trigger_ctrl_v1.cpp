#define _CRT_SECURE_NO_WARNINGS

// FTDI Driver Includes
#include "ftd2xx_functions.h"

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)
//#include <windows.h> 
#include <conio.h>
#elif defined(__linux__)
#include <termios.h>
#endif

// C++ Includes
#include <cstdio>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <fstream>
#include <string>
#include <vector>
#include <chrono>

// Custom Includes
#include "num2string.h"
#include "get_current_time.h"
#include "file_parser.h"
#include "file_ops.h"
//#include "make_dir.h"
//#include "ftdi_motor_driver.h"

// Project Includes
#include "trigger_ctrl.h"



int main(int argc, char** argv)
{
    uint32_t idx;
	int8_t stop = 0;
    uint8_t status;
    std::string console_input;
	std::string value_str;
	int32_t value = 0;
    //bool direct = false;

    trigger_ctrl tc;

    // Motor Driver Variables
    uint32_t ftdi_device_count = 0;
    ftdiDeviceDetails driver_details;
    FT_HANDLE driver_handle = NULL;
    uint32_t driver_device_num = 0;
    uint32_t connect_count = 0;
    uint32_t read_timeout = 25000;
    uint32_t write_timeout = 1000;
    std::vector<ftdiDeviceDetails> ftdi_devices;
    //int32_t steps;
    //int32_t focus_step = 0;
    //int32_t zoom_step = 0;

#if defined(__linux__)
    struct termios old_term, new_term;
#endif


    try
    {
        ftdi_device_count = get_device_list(ftdi_devices);
        if (ftdi_device_count == 0)
        {
            std::cout << "No ftdi devices found... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        for (idx = 0; idx < ftdi_devices.size(); ++idx)
        {
            std::cout << ftdi_devices[idx];
        }

        std::cout << "Select Motor Controller: ";
        std::getline(std::cin, console_input);
        driver_device_num = stoi(console_input);

        std::cout << std::endl << "Connecting to Motor Controller..." << std::endl;
        ftdi_devices[driver_device_num].baud_rate = 250000;
        while ((driver_handle == NULL) && (connect_count < 10))
        {
            driver_handle = open_com_port(ftdi_devices[driver_device_num], read_timeout, write_timeout);
            ++connect_count;
        }

        if (driver_handle == NULL)
        {
            std::cout << "No Motor Controller found... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        tc.tx = data_packet(CONNECT);

        // send connection request packet and get response back
        tc.send_packet(driver_handle, tc.tx);
        status = tc.receive_packet(driver_handle, 6, tc.rx);

        if (status == false)
        {
            std::cout << "Error communicating with Motor Controller... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        tc.set_trigger_info(tc.rx);
        std::cout << tc << std::endl;    
    }
    catch (std::exception e)
    {
        std::cout << "Error: " << e.what() << std::endl;
    }

	close_com_port(driver_handle);

    std::cout << "Program Compete!" << std::endl;
    return 0;
    
}   // end of main

