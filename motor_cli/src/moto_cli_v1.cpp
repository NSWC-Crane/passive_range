#define _CRT_SECURE_NO_WARNINGS

// FTDI Driver Includes
#include "ftd2xx_functions.h"

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)
//#include <windows.h> 
#include <conio.h>
#elif defined(__linux__)
#include <termios.h>           // Contains POSIX terminal control definitions
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
#include "file_ops.h"
#include "file_parser.h"
//#include "ftdi_motor_driver.h"

// Project Includes
#include "motor_driver.h"
#include "dynamixel_packet_v2.h"

//-----------------------------------------------------------------------------
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
    std::cout << "  d - step the motors using the arrow keys (a,d - focus motor; w,s - zoom motor)" << std::endl;
    std::cout << "  af <step> - set the absolute focus motor step value [0 - " << max_focus_steps << "]" << std::endl;
    std::cout << "  az <step> - set the absolute zoom motor step value [0 - " << max_zoom_steps << "]" << std::endl;
    //std::cout << "  sf <step> - set the focus motor step speed [" << min_pw << " - " << max_pw << "]" << std::endl;
    //std::cout << "  sz <step> - set the zoom motor step speed [" << min_pw << " - " << max_pw << "]" << std::endl;

    //std::cout << "  x - zero the motor counter" << std::endl;
    std::cout << "----------------------------------------------------------------" << std::endl;
    std::cout << std::endl;
}

//-----------------------------------------------------------------------------
int main(int argc, char** argv)
{
    uint32_t idx;
	int8_t stop = 0;
    bool status;
    std::string console_input;
	std::string value_str;
	int32_t value = 0;
    bool direct = false;

    motor_driver md;

    // Motor Driver Variables
    uint32_t ftdi_device_count = 0;
    ftdiDeviceDetails driver_details;
    FT_HANDLE md_handle = NULL;
    uint32_t driver_device_num = 0;
    uint32_t connect_count = 0;
    uint32_t read_timeout = 30000;
    uint32_t write_timeout = 1000;
    std::vector<ftdiDeviceDetails> ftdi_devices;

    std::string motor_type = "";
    dynamixel_packet dyn_packet;
    int32_t steps, pw;
    int32_t focus_step = 0;
    int32_t zoom_step = 0;
    int32_t focus_pw = 0;
    int32_t zoom_pw = 0;

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
        while ((md_handle == NULL) && (connect_count < 10))
        {
            md_handle = open_com_port(ftdi_devices[driver_device_num], read_timeout, write_timeout);
            ++connect_count;
        }

        if (md_handle == NULL)
        {
            std::cout << "No Motor Controller found... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        md.tx = data_packet(MD_CONNECT);

        // send connection request packet and get response back
        md.send_packet(md_handle, md.tx);
        status = md.receive_packet(md_handle, 6, md.rx);

        if (status == false)
        {
            std::cout << "Error communicating with Motor Controller... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        md.set_driver_info(md.rx);
        std::cout << md << std::endl;

        //-----------------------------------------------------------------------------
        // get the current step count reported by the controller
        //dyn_packet.init(FOCUS_MOTOR_ID, 7, DYN_READ);
        //dyn_packet.add_uint16(ADD_GOAL_POSITION);
/*
        md.tx = data_packet(GET_MOTOR_STEP, (uint8_t)dyn_packet.data.size(), dyn_packet.data);
        md.send_packet(md_handle, md.tx);
        status = md.receive_packet(md_handle, 6, md.rx);
        if (status)
            focus_step = (md.rx.data[0] << 24) | (md.rx.data[1] << 16) | (md.rx.data[2] << 8) | (md.rx.data[3]);
        else
            focus_step = -1;

        md.tx = data_packet(GET_ZM_MOT_STEP);
        md.send_packet(md_handle, md.tx);
        status = md.receive_packet(md_handle, 6, md.rx);
        if (status)
            zoom_step = (md.rx.data[0] << 24) | (md.rx.data[1] << 16) | (md.rx.data[2] << 8) | (md.rx.data[3]);
        else
            zoom_step = -1;

        std::cout << "Focus Step: " << focus_step << ", Zoom Step: " << zoom_step << std::endl;

        //-----------------------------------------------------------------------------
        // get the current motor pulse widths reported by the controller
        md.tx = data_packet(GET_FOC_MOT_SPD);
        md.send_packet(md_handle, md.tx);
        status = md.receive_packet(md_handle, 6, md.rx);
        if (status)
            focus_pw = (md.rx.data[0] << 24) | (md.rx.data[1] << 16) | (md.rx.data[2] << 8) | (md.rx.data[3]);
        else
            focus_pw = -1;

        md.tx = data_packet(GET_ZM_MOT_SPD);
        md.send_packet(md_handle, md.tx);
        status = md.receive_packet(md_handle, 6, md.rx);
        if (status)
            zoom_pw = (md.rx.data[0] << 24) | (md.rx.data[1] << 16) | (md.rx.data[2] << 8) | (md.rx.data[3]);
        else
            zoom_pw = -1;

        std::cout << "Focus PW: " << focus_pw << ", Zoom PW: " << zoom_pw << std::endl;

        //-----------------------------------------------------------------------------
		// print out a short menu of commands for the CLI
		help_menu();
*/
        // start the while loop
		while(stop >= 0)
		{
			std::cout << "motor_cli> ";
			std::getline(std::cin, console_input);
			
            // quit the application
			if(console_input[0] == 'q')
			{
				stop = -1;
				break;
			}
            // get the help menu
			else if(console_input[0] == '?')
			{
				help_menu();
			}
            // experimental - not listed in help
            else if (console_input[0] == 'x')
            {
                md.tx = data_packet(MOTOR_CTRL, 0);
                md.send_packet(md_handle, md.tx);
                //status = md.receive_packet(md_handle, 3, md.rx);
            }
            /*
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

                    md.send_packet(md_handle, md.tx);
                    status = md.receive_packet(md_handle, 3, md.rx);
                }
                else
                {
                    std::cout << "The number of parameters passed for '" << console_input[0] << "' is incorrect." << std::endl;
                }
            }

            //-----------------------------------------------------------------------------
            else if(console_input[0] == 'f')
			{
                if (console_input.length() >= 3)
                {
                    focus_step = std::stoi(console_input.substr(2, console_input.length()-1));

                    md.tx = data_packet(CMD_FOCUS_CTRL, focus_step);
                    md.send_packet(md_handle, md.tx);
                    status = md.receive_packet(md_handle, 6, md.rx);

                    if (status)
                    {
                        steps = (md.rx.data[0] << 24) | (md.rx.data[1] << 16) | (md.rx.data[2] << 8) | (md.rx.data[3]);
                        std::cout << "focus steps: " << steps << std::endl;
                    }
                }
                else
                {
                    std::cout << "The number of parameters passed for '" << console_input[0] << "' is incorrect." << std::endl;
                }
            }

            //-----------------------------------------------------------------------------
            else if(console_input[0] == 'z')
			{
                if (console_input.length() >= 3)
                {
                    zoom_step = std::stoi(console_input.substr(2, console_input.length() - 1));

                    md.tx = data_packet(CMD_ZOOM_CTRL, zoom_step);
                    md.send_packet(md_handle, md.tx);
                    status = md.receive_packet(md_handle, 6, md.rx);

                    if (status)
                    {
                        steps = (md.rx.data[0] << 24) | (md.rx.data[1] << 16) | (md.rx.data[2] << 8) | (md.rx.data[3]);
                        std::cout << "zoom steps: " << steps << std::endl;
                    }
                }
                else
                {
                    std::cout << "The number of parameters passed for '" << console_input[0] << "' is incorrect." << std::endl;
                }				
			}

            //-----------------------------------------------------------------------------
            else if (console_input[0] == 'a')
            {
                if (console_input.length() > 3)
                {
                    status = false;
                    motor_type = "";
                    switch (console_input[1])
                    {
                    case 'f':
                        motor_type = "focus";
                        focus_step = std::stoi(console_input.substr(3, console_input.length() - 1));
                        md.tx = data_packet(ABS_FOCUS_CTRL, focus_step);
                        md.send_packet(md_handle, md.tx);
                        status = md.receive_packet(md_handle, 6, md.rx);

                        break;

                    case 'z':
                        motor_type = "zoom";
                        zoom_step = std::stoi(console_input.substr(3, console_input.length() - 1));
                        md.tx = data_packet(ABS_ZOOM_CTRL, zoom_step);
                        md.send_packet(md_handle, md.tx);
                        status = md.receive_packet(md_handle, 6, md.rx);

                        break;

                    }

                    if (status)
                    {
                        steps = (md.rx.data[0] << 24) | (md.rx.data[1] << 16) | (md.rx.data[2] << 8) | (md.rx.data[3]);
                        std::cout << motor_type << " steps: " << steps << std::endl;
                    }
                }
            }

            //-----------------------------------------------------------------------------
            else if (console_input[0] == 's')
            {
                if (console_input.length() > 3)
                {
                    status = false;
                    motor_type = "";
                    switch (console_input[1])
                    {
                    case 'f':
                        motor_type = "focus";
                        focus_pw = std::abs(std::stoi(console_input.substr(3, console_input.length() - 1)));
                        focus_pw = min(max_pw, max(min_pw, focus_pw));
                        md.tx = data_packet(SET_FOC_MOT_SPD, focus_pw);
                        md.send_packet(md_handle, md.tx);
                        status = md.receive_packet(md_handle, 6, md.rx);

                        //steps = (md.rx.data[0] << 24) | (md.rx.data[1] << 16) | (md.rx.data[2] << 8) | (md.rx.data[3]);
                        //std::cout << "steps: " << steps << std::endl;
                        break;

                    case 'z':
                        motor_type = "zoom";
                        zoom_pw = std::abs(std::stoi(console_input.substr(3, console_input.length() - 1)));
                        zoom_pw = min(max_pw, max(min_pw, zoom_pw));
                        md.tx = data_packet(SET_ZM_MOT_SPD, zoom_pw);
                        md.send_packet(md_handle, md.tx);
                        status = md.receive_packet(md_handle, 6, md.rx);

                        break;

                    }

                    if (status)
                    {
                        pw = (md.rx.data[0] << 24) | (md.rx.data[1] << 16) | (md.rx.data[2] << 8) | (md.rx.data[3]);
                        std::cout << motor_type << " pw: " << pw << std::endl;
                    }
                }
            }

            //-----------------------------------------------------------------------------
            else if (console_input[0] == 'd')
            {
                std::cout << "Step the motors using the arrow keys (a,d - focus motor; w,s - zoom motor)" << std::endl;
                std::cout << "Press any other key to exit direct control mode." << std::endl;

                direct = true;
                //wchar_t key;
                int key = 0;

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)
                do
                {

                    //std::wcin >> key;
                    //key = _getch();
                    //if (key == 0xE0)
                    //{
                    key = _getch();

#elif defined(__linux__)
                // http://shtrom.ssji.net/skb/getc.html
                tcgetattr(STDIN_FILENO, &old_term); //get the current terminal I/O structure
                new_term = old_term;
                new_term.c_lflag &= (~ICANON & ~ECHO); //Manipulate the flag bits to do what you want it to do
                tcsetattr(STDIN_FILENO, TCSANOW, &new_term); //Apply the new settings

                do
                {
                    key = std::getchar();

#endif
                    status = false;
                    motor_type = "";

                    switch (key)
                    {
                    case (int)('w'):
                    //case 72:    // up arrow key
                        zoom_step = -16;
                        motor_type = "zoom";
                        md.tx = data_packet(CMD_ZOOM_CTRL, zoom_step);
                        md.send_packet(md_handle, md.tx);
                        status = md.receive_packet(md_handle, 6, md.rx);
                        break;

                    case (int)('a') :
                    //case 75:    // left arrow key
                        focus_step = -16;
                        motor_type = "focus";
                        md.tx = data_packet(CMD_FOCUS_CTRL, focus_step);
                        md.send_packet(md_handle, md.tx);
                        status = md.receive_packet(md_handle, 6, md.rx);
                        //printf("left");
                        //std::cout << "left" << std::endl;
                        break;

                    case (int)('d') :
                    //case 77:    // right arrow key
                        focus_step = 16;
                        motor_type = "focus";
                        md.tx = data_packet(CMD_FOCUS_CTRL, focus_step);
                        md.send_packet(md_handle, md.tx);
                        status = md.receive_packet(md_handle, 6, md.rx);
                        //std::cout << "right" << std::endl;
                        break;

                    case (int)('s') :
                    //case 80:    // down arrow key
                        zoom_step = 16;
                        motor_type = "zoom";
                        md.tx = data_packet(CMD_ZOOM_CTRL, zoom_step);
                        md.send_packet(md_handle, md.tx);
                        status = md.receive_packet(md_handle, 6, md.rx);
                        //std::cout << "down" << std::endl;
                        break;

                    default:
                        direct = false;

                        break;
                    }

                    if (status)
                    {
                        steps = (md.rx.data[0] << 24) | (md.rx.data[1] << 16) | (md.rx.data[2] << 8) | (md.rx.data[3]);
                        std::cout << motor_type << " steps: " << steps << std::endl;
                    }

                } while (direct == true);

#if defined(__linux__)
                tcsetattr(STDIN_FILENO, TCSANOW, &old_term); //Apply the old settings
#endif
            }
*/		
		}	// end of while loop


/*
        // send the motor enable command
        md.tx = data_packet(CMD_MOTOR_ENABLE, 1, { ENABLE_MOTOR });
        md.send_packet(md_handle, md.tx);
        status = md.receive_packet(md_handle, 3, md.rx);


        // try to step the focus motor
        focus_step = 3216 | MOTOR_CW;
        md.tx = data_packet(CMD_FOCUS_CTRL, focus_step);
        md.send_packet(md_handle, md.tx);
        status = md.receive_packet(md_handle, 3, md.rx);

        focus_step = 3216 | MOTOR_CCW;
        md.tx = data_packet(CMD_FOCUS_CTRL, focus_step);
        md.send_packet(md_handle, md.tx);
        status = md.receive_packet(md_handle, 3, md.rx);

        // try to step the focus motor
        zoom_step = 3216 | MOTOR_CW;
        md.tx = data_packet(CMD_ZOOM_CTRL, zoom_step);
        md.send_packet(md_handle, md.tx);
        status = md.receive_packet(md_handle, 3, md.rx);

        zoom_step = 3216 | MOTOR_CCW;
        md.tx = data_packet(CMD_ZOOM_CTRL, zoom_step);
        md.send_packet(md_handle, md.tx);
        status = md.receive_packet(md_handle, 3, md.rx);

        // send the motor disable command
        md.tx = data_packet(CMD_MOTOR_ENABLE, 1, { DISABLE_MOTOR });
        md.send_packet(md_handle, md.tx);
        status = md.receive_packet(md_handle, 3, md.rx);
*/

    }
    catch (std::exception e)
    {
        std::cout << "Error: " << e.what() << std::endl;
    }

	close_com_port(md_handle);

    std::cout << "Program Compete!" << std::endl;
    return 0;

}   // end of main
