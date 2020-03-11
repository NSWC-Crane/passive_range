#define _CRT_SECURE_NO_WARNINGS

// FTDI Driver Includes
#include "ftdi_functions.h"

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)
//#include <windows.h> 
#include <conio.h>
#elif defined(__linux__)

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
#include "make_dir.h"
//#include "ftdi_motor_driver.h"

// Project Includes
#include "motor_driver.h"


void help_menu(void)
{
    std::cout << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;
	std::cout << "Motor Driver CLI Commands:" << std::endl;
	std::cout << "  ? - print this menu" << std::endl;
	std::cout << "  q - quit" << std::endl;
	std::cout << "  e <0/1> - enable (1)/disable (0) motors" << std::endl;
	std::cout << "  f <step> - step the focus motor, use '-' for CCW otherwise CW" << std::endl;
	std::cout << "  z <step> - step the zoom motor, use '-' for CCW otherwise CW" << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;
    std::cout << std::endl;
}

//
int main(int argc, char** argv)
{
    uint32_t idx;
	int8_t stop = 0;
    uint8_t status;
    std::string console_input;
	std::string value_str;
	int32_t value = 0;
    bool direct = false;

    motor_driver md;

    // Motor Driver Variables
    uint32_t ftdi_device_count = 0;
    ftdiDeviceDetails driver_details;
    FT_HANDLE driver_handle = NULL;
    uint32_t driver_device_num = 0;
    uint32_t connect_count = 0;
    uint32_t read_timeout = 15000;
    uint32_t write_timeout = 1000;
    std::vector<ftdiDeviceDetails> ftdi_devices;
    int32_t steps;

    int32_t focus_step, zoom_step;

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

        md.tx = data_packet(CONNECT);

        // send connection request packet and get response back
        md.send_packet(driver_handle, md.tx);
        status = md.receive_packet(driver_handle, 6, md.rx);

        if (status == false)
        {
            std::cout << "Error communicating with Motor Controller... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        md.set_driver_info(md.rx);
        std::cout << md << std::endl;


		// print out a short menu of commands for the CLI
		help_menu();

        // start the while loop
		while(stop >= 0)
		{
			std::cout << "motor_cli> ";
			std::getline(std::cin, console_input);
			
			if(console_input[0] == 'q')
			{
				stop = -1;
				break;
			}
			else if(console_input[0] == '?')
			{
				help_menu();
			}
            else if (console_input[0] == 'e')
            {
                if (console_input.length() >= 3)
                {
                    value = std::stoi(console_input.substr(2,1));
                    // send the motor enable command
                    if(value == 1)
                        md.tx = data_packet(CMD_MOTOR_ENABLE, 1, { ENABLE_MOTOR });
                    else
                        md.tx = data_packet(CMD_MOTOR_ENABLE, 1, { DISABLE_MOTOR });

                    md.send_packet(driver_handle, md.tx);
                    status = md.receive_packet(driver_handle, 3, md.rx);
                }
                else
                {
                    std::cout << "The number of parameters passed for '" << console_input[0] << "' is incorrect." << std::endl;
                }
            }
			else if(console_input[0] == 'f')
			{
                if (console_input.length() >= 3)
                {
                    focus_step = std::stoi(console_input.substr(2, console_input.length()-1));
                    // try to step the focus motor
                    //if (value < 0)
                    //    focus_step = abs(value) | MOTOR_CCW;
                    //else
                    //    focus_step = value | MOTOR_CW;

                    md.tx = data_packet(CMD_FOCUS_CTRL, focus_step);
                    md.send_packet(driver_handle, md.tx);
                    status = md.receive_packet(driver_handle, 6, md.rx);

                    steps = (md.rx.data[0] << 24) | (md.rx.data[1] << 16) | (md.rx.data[2] << 8) | (md.rx.data[3]);
                    std::cout << "steps: " << steps << std::endl;
                }
                else
                {
                    std::cout << "The number of parameters passed for '" << console_input[0] << "' is incorrect." << std::endl;
                }
            }
			else if(console_input[0] == 'z')
			{
                if (console_input.length() >= 3)
                {
                    zoom_step = std::stoi(console_input.substr(2, console_input.length() - 1));

                    //if (value < 0)
                    //    zoom_step = abs(value) | MOTOR_CCW;
                    //else
                    //    zoom_step = value | MOTOR_CW;

                    md.tx = data_packet(CMD_ZOOM_CTRL, zoom_step);
                    md.send_packet(driver_handle, md.tx);
                    status = md.receive_packet(driver_handle, 6, md.rx);

                    steps = (md.rx.data[0] << 24) | (md.rx.data[1] << 16) | (md.rx.data[2] << 8) | (md.rx.data[3]);
                    std::cout << "steps: " << steps << std::endl;
                }
                else
                {
                    std::cout << "The number of parameters passed for '" << console_input[0] << "' is incorrect." << std::endl;
                }				
			}
#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)
            else if (console_input[0] == 'd')
            {
                direct = true;
                //wchar_t key;
                int key = 0;
                do
                {
                    //std::wcin >> key;
                    key = _getch();
                    if (key == 0xE0)
                    {
                        key = _getch();
                        switch (key)
                        {
                        case 72:    // up arrow key
                            zoom_step = 16 | MOTOR_CW;

                            md.tx = data_packet(CMD_ZOOM_CTRL, zoom_step);
                            md.send_packet(driver_handle, md.tx);
                            status = md.receive_packet(driver_handle, 6, md.rx);
                            break;
                        case 75:    // left arrow key
                            focus_step = 16 | MOTOR_CW;

                            md.tx = data_packet(CMD_FOCUS_CTRL, focus_step);
                            md.send_packet(driver_handle, md.tx);
                            status = md.receive_packet(driver_handle, 6, md.rx);
                            //printf("left");
                            //std::cout << "left" << std::endl;
                            break;
                        case 77:    // right arrow key
                            focus_step = 16 | MOTOR_CCW;

                            md.tx = data_packet(CMD_FOCUS_CTRL, focus_step);
                            md.send_packet(driver_handle, md.tx);
                            status = md.receive_packet(driver_handle, 6, md.rx);
                            //std::cout << "right" << std::endl;
                            break;
                        case 80:    // down arrow key
                            zoom_step = 16 | MOTOR_CCW;

                            md.tx = data_packet(CMD_ZOOM_CTRL, zoom_step);
                            md.send_packet(driver_handle, md.tx);
                            status = md.receive_packet(driver_handle, 6, md.rx);
                            //std::cout << "down" << std::endl;
                            break;
                        }
                    }
                    else
                    {
                        direct = false;
                    }
                } while (direct == true);

            }
#endif
		
		}	// end of while loop


/*
        // send the motor enable command
        md.tx = data_packet(CMD_MOTOR_ENABLE, 1, { ENABLE_MOTOR });
        md.send_packet(driver_handle, md.tx);
        status = md.receive_packet(driver_handle, 3, md.rx);


        // try to step the focus motor
        focus_step = 3216 | MOTOR_CW;
        md.tx = data_packet(CMD_FOCUS_CTRL, focus_step);
        md.send_packet(driver_handle, md.tx);
        status = md.receive_packet(driver_handle, 3, md.rx);

        focus_step = 3216 | MOTOR_CCW;
        md.tx = data_packet(CMD_FOCUS_CTRL, focus_step);
        md.send_packet(driver_handle, md.tx);
        status = md.receive_packet(driver_handle, 3, md.rx);

        // try to step the focus motor
        zoom_step = 3216 | MOTOR_CW;
        md.tx = data_packet(CMD_ZOOM_CTRL, zoom_step);
        md.send_packet(driver_handle, md.tx);
        status = md.receive_packet(driver_handle, 3, md.rx);

        zoom_step = 3216 | MOTOR_CCW;
        md.tx = data_packet(CMD_ZOOM_CTRL, zoom_step);
        md.send_packet(driver_handle, md.tx);
        status = md.receive_packet(driver_handle, 3, md.rx);

        // send the motor disable command
        md.tx = data_packet(CMD_MOTOR_ENABLE, 1, { DISABLE_MOTOR });
        md.send_packet(driver_handle, md.tx);
        status = md.receive_packet(driver_handle, 3, md.rx);
*/

    }
    catch (std::exception e)
    {
        std::cout << "Error: " << e.what() << std::endl;
    }

	close_com_port(driver_handle);

    std::cout << "Program Compete!" << std::endl;
    return 0;

}   // end of main
