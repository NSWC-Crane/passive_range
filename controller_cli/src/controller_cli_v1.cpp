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
#include "sleep_ms.h"

// Project Includes
#include "control_driver.h"
#include "dynamixel_packet_v2.h"

//-----------------------------------------------------------------------------
void help_menu(void)
{
    std::cout << "-----------------------------------------------------------------------------" << std::endl;
	std::cout << "Controller CLI Commands:" << std::endl;
	std::cout << "  ? - print this menu" << std::endl;
	std::cout << "  q - quit" << std::endl;
    std::cout << "  e <0/1> - enable (1)/disable (0) motors" << std::endl;
    std::cout << "  f <step> - move the focus motor to the given step" << std::endl;
	std::cout << "  z <step> - move the zoom motor to the given step" << std::endl;
    std::cout << "  c <channel,polarity,offset,length> - Configure channel timing parameters" << std::endl;
    std::cout << "  t <channel> - trigger channel: (a) all, (1) channel 1, (2) channel 2" << std::endl;

    //std::cout << "  af <step> - set the absolute focus motor step value [0 - " << max_focus_steps << "]" << std::endl;
    //std::cout << "  az <step> - set the absolute zoom motor step value [0 - " << max_zoom_steps << "]" << std::endl;
    //std::cout << "  im - get information about the motors" << std::endl;
    std::cout << "  it - get the current trigger configuration parameters" << std::endl;
    //std::cout << "  d - step the motors using the arrow keys (a,d - focus motor; w,s - zoom motor)" << std::endl;

    std::cout << std::endl << "Examples:" << std::endl;
    std::cout << "  enable motor:             controller> e 1" << std::endl;
    std::cout << "  set focus motor position: controller> f 1000" << std::endl;
    std::cout << "  set zoom motor position:  controller> z 200" << std::endl;
    std::cout << "  trigger all channels:     controller> t a" << std::endl;
    std::cout << "  config trigger channel 1: controller> c 1,0,10000,25000" << std::endl;

    std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;
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

    controller ctrl;

    // Controller Variables
    uint32_t ftdi_device_count = 0;
    ftdiDeviceDetails driver_details;
    FT_HANDLE ctrl_handle = NULL;
    uint32_t driver_device_num = 0;
    uint32_t connect_count = 0;
    uint32_t read_timeout = 30000;
    uint32_t write_timeout = 1000;
    std::vector<ftdiDeviceDetails> ftdi_devices;

    // motor variables
    std::string motor_type = "";
    dynamixel_packet mtr_packet;
    int32_t steps;
    int32_t focus_step = 0, focus_offset = 0;
    int32_t zoom_step = 0, zoom_offset = 0;
    //uint8_t mtr_error;
    motor_info focus_motor;
    motor_info zoom_motor;
    //uint32_t step_delta = 3;
    bool motor_enabled = false;
    bool mtr_moving = false;

    // trigger variables
    trigger_info t1_info;
    trigger_info t2_info;

    //uint32_t pw;
    //int32_t focus_pw = 0;
    //int32_t zoom_pw = 0;

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

        std::cout << "Select Controller: ";
        std::getline(std::cin, console_input);
        driver_device_num = stoi(console_input);

        std::cout << std::endl << "Rotate the focus and the zoom lens to the zero position.  Press Enter when complete...";
        std::cin.ignore();

        std::cout << std::endl << "Connecting to Controller..." << std::endl;
        ftdi_devices[driver_device_num].baud_rate = 250000;
        while ((ctrl_handle == NULL) && (connect_count < 10))
        {
            ctrl_handle = open_com_port(ftdi_devices[driver_device_num], read_timeout, write_timeout);
            ++connect_count;
        }

        if (ctrl_handle == NULL)
        {
            std::cout << "No Controller found... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        flush_port(ctrl_handle);
        ctrl.tx = data_packet(DRIVER_CONNECT);

        // send connection request packet and get response back
        ctrl.send_packet(ctrl_handle, ctrl.tx);
        status = ctrl.receive_packet(ctrl_handle, 6, ctrl.rx);

        if (status == false)
        {
            std::cout << "Error communicating with Controller... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        ctrl.set_driver_info(ctrl.rx);
        std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << ctrl;
        std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;

        //-----------------------------------------------------------------------------
        // ping the motors to get the model number and firmware version
        status = ctrl.ping_motor(ctrl_handle, FOCUS_MOTOR_ID, focus_motor);

        std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << "Focus Motor Information: " << std::endl;

        if (status)
        {
            std::cout << focus_motor;
        }
        else
        {
            std::cout << "  Error getting focus motor info" << std::endl;
        }
        //std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;

        status = ctrl.ping_motor(ctrl_handle, ZOOM_MOTOR_ID, zoom_motor);

        //std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << std::endl;
        std::cout << "Zoom Motor Information: " << std::endl;
        
        if (status)
        {
            std::cout << zoom_motor;
        }
        else
        {
            std::cout << "  Error getting zoom motor info" << std::endl;
        }
        //std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;

        //-----------------------------------------------------------------------------
        // configure the homing offsets for the focus motor and the zoom motor
        // set the offset to zero for both
        status = ctrl.set_offset(ctrl_handle, FOCUS_MOTOR_ID);
        status = ctrl.set_offset(ctrl_handle, ZOOM_MOTOR_ID);

        //-----------------------------------------------------------------------------
        // get the current motor positions
        status = ctrl.get_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);
        status = ctrl.get_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_step);

        //std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << std::endl;
        std::cout << "Current focus Step: " << focus_step << std::endl; 
        std::cout << "Current zoom Step:  " << zoom_step << std::endl;
        std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;

        // get the current trigger configurations
        status = ctrl.get_trigger_info(ctrl_handle, t1_info, t2_info);

        std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << "Trigger Information: " << std::endl;
        std::cout << t1_info << std::endl;
        std::cout << t2_info;
        std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;


        //-----------------------------------------------------------------------------
		// print out a short menu of commands for the CLI
		help_menu();

        // start the while loop
		while(stop >= 0)
		{
			std::cout << "controller> ";
			std::getline(std::cin, console_input);
			
            switch (console_input[0])
            {
            // quit the application
            case 'q':
                stop = -1;
                break;

            // display the help menu
            case '?':
                help_menu();
                break;

            // experimental - not listed in help
            case 'x':

                focus_step = std::stoi(console_input.substr(2, console_input.length() - 1));

                status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);
                break;

            // information about the various components
            case 'i':

                switch (console_input[1])
                {
                //motor info
                case 'm':


                    break;

                // trigger info
                case 't':

                    status = ctrl.get_trigger_info(ctrl_handle, t1_info, t2_info);

                    std::cout << "-----------------------------------------------------------------------------" << std::endl;
                    std::cout << "Trigger Information: " << std::endl;
                    std::cout << t1_info << std::endl;
                    std::cout << t2_info;
                    std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;

                    break;

                }
                //status = ctrl.ping_motor(ctrl_handle, FOCUS_MOTOR_ID, focus_motor);

                //if (status)
                //    std::cout << focus_motor << std::endl;
                //else
                //    std::cout << "Error getting focus motor info" << std::endl;

                break;

            // enable/disable the motors
            case 'e':

                if (console_input.length() >= 3)
                {
                    value = std::stoi(console_input.substr(2, 1));

                    // send the motor enable command
                    if (value == 1)
                    {
                        status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, true);
                        status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, true);
                    }
                    else
                    {
                        status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);
                        status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);
                    }

                }
                else
                {
                    std::cout << "The number of parameters passed for '" << console_input[0] << "' is incorrect." << std::endl;
                }
                break;

            //-----------------------------------------------------------------------------
            // control the focus motor
            case 'f':
                if (console_input.length() >= 3)
                {
                    //mtr_moving = true;
                    steps = std::stoi(console_input.substr(2, console_input.length() - 1));
                    //steps = min(max(steps, min_focus_steps), max_focus_steps);

                    status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, steps);

                    if (status == true)
                    {
                        //sleep_ms(50);
                        status = ctrl.get_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);
                        std::cout << "current focus position: " << focus_step << std::endl;
                    }
                }
                else
                {
                    std::cout << "The number of parameters passed for '" << console_input[0] << "' is incorrect." << std::endl;
                }
                break;

            //-----------------------------------------------------------------------------
            // control the zoom motor
            case 'z':
                if (console_input.length() >= 3)
                {
                    //mtr_moving = true;
                    steps = std::stoi(console_input.substr(2, console_input.length() - 1));
                    //steps = min(max(steps, min_zoom_steps), max_zoom_steps);

                    status = ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, steps);

                    if (status)
                    {
                        //sleep_ms(50);
                        status = ctrl.get_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_step);
                        std::cout << "current zoom position: " << zoom_step << std::endl;
                    }
                }
                else
                {
                    std::cout << "The number of parameters passed for '" << console_input[0] << "' is incorrect." << std::endl;
                }
                break;

            // trigger an individual channel
            case 't':
                if (console_input.length() < 3)
                {
                    break;
                }

                if (console_input[2] == 'a')
                {
                    status = ctrl.trigger(ctrl_handle, TRIG_ALL);
                }
                else if (console_input[2] == '1')
                {
                    status = ctrl.trigger(ctrl_handle, TRIG_CH1);
                }
                else if (console_input[2] == '2')
                {
                    status = ctrl.trigger(ctrl_handle, TRIG_CH2);
                }
                break;

            // configure the trigger channels
            case 'c':

                if (console_input.length() < 8)
                {
                    std::cout << "Not enough parameters..." << std::endl;
                    break;
                }

                if (console_input[2] == '1')
                {
                    //command = CONFIG_T1;
                    status = ctrl.config_channel(ctrl_handle, CONFIG_T1, console_input.substr(3, console_input.length()));
                }
                else if (console_input[2] == '2')
                {
                    //command = CONFIG_T2;
                    status = ctrl.config_channel(ctrl_handle, CONFIG_T2, console_input.substr(3, console_input.length()));
                }
                break;
                /*
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
                            ctrl.tx = data_packet(SET_FOC_MOT_SPD, focus_pw);
                            ctrl.send_packet(ctrl_handle, ctrl.tx);
                            status = ctrl.receive_packet(ctrl_handle, 6, ctrl.rx);

                            //steps = (ctrl.rx.data[0] << 24) | (ctrl.rx.data[1] << 16) | (ctrl.rx.data[2] << 8) | (ctrl.rx.data[3]);
                            //std::cout << "steps: " << steps << std::endl;
                            break;

                        case 'z':
                            motor_type = "zoom";
                            zoom_pw = std::abs(std::stoi(console_input.substr(3, console_input.length() - 1)));
                            zoom_pw = min(max_pw, max(min_pw, zoom_pw));
                            ctrl.tx = data_packet(SET_ZM_MOT_SPD, zoom_pw);
                            ctrl.send_packet(ctrl_handle, ctrl.tx);
                            status = ctrl.receive_packet(ctrl_handle, 6, ctrl.rx);

                            break;

                        }

                        if (status)
                        {
                            pw = (ctrl.rx.data[0] << 24) | (ctrl.rx.data[1] << 16) | (ctrl.rx.data[2] << 8) | (ctrl.rx.data[3]);
                            std::cout << motor_type << " pw: " << pw << std::endl;
                        }
                    }
                }
                */

                /*
                //-----------------------------------------------------------------------------
                else if (console_input[0] == 'd')
                {
                    std::cout << "Step the motors using the following keys (a,s - focus motor; k,l - zoom motor)" << std::endl;
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
                        case (int)('k'):
                            zoom_step = -16;
                            motor_type = "zoom";
                            ctrl.tx = data_packet(CMD_ZOOM_CTRL, zoom_step);
                            ctrl.send_packet(ctrl_handle, ctrl.tx);
                            status = ctrl.receive_packet(ctrl_handle, 6, ctrl.rx);
                            break;

                        case (int)('l') :
                            zoom_step = 16;
                            motor_type = "zoom";
                            ctrl.tx = data_packet(CMD_ZOOM_CTRL, zoom_step);
                            ctrl.send_packet(ctrl_handle, ctrl.tx);
                            status = ctrl.receive_packet(ctrl_handle, 6, ctrl.rx);
                            //std::cout << "down" << std::endl;
                            break;
                        case (int)('a') :
                            focus_step = -16;
                            motor_type = "focus";
                            ctrl.tx = data_packet(CMD_FOCUS_CTRL, focus_step);
                            ctrl.send_packet(ctrl_handle, ctrl.tx);
                            status = ctrl.receive_packet(ctrl_handle, 6, ctrl.rx);
                            //printf("left");
                            //std::cout << "left" << std::endl;
                            break;

                        case (int)('d') :
                        //case 77:    // right arrow key
                            focus_step = 16;
                            motor_type = "focus";
                            ctrl.tx = data_packet(CMD_FOCUS_CTRL, focus_step);
                            ctrl.send_packet(ctrl_handle, ctrl.tx);
                            status = ctrl.receive_packet(ctrl_handle, 6, ctrl.rx);
                            //std::cout << "right" << std::endl;
                            break;



                        default:
                            direct = false;

                            break;
                        }

                        if (status)
                        {
                            steps = (ctrl.rx.data[0] << 24) | (ctrl.rx.data[1] << 16) | (ctrl.rx.data[2] << 8) | (ctrl.rx.data[3]);
                            std::cout << motor_type << " steps: " << steps << std::endl;
                        }

                    } while (direct == true);

    #if defined(__linux__)
                    tcsetattr(STDIN_FILENO, TCSANOW, &old_term); //Apply the old settings
    #endif
                }
            */
            default:
                break;
            }
		}	// end of while loop

        // disable the motors before shutting down
        status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);
        status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);

/*
        // send the motor enable command
        ctrl.tx = data_packet(CMD_MOTOR_ENABLE, 1, { ENABLE_MOTOR });
        ctrl.send_packet(ctrl_handle, ctrl.tx);
        status = ctrl.receive_packet(ctrl_handle, 3, ctrl.rx);


        // try to step the focus motor
        focus_step = 3216 | MOTOR_CW;
        ctrl.tx = data_packet(CMD_FOCUS_CTRL, focus_step);
        ctrl.send_packet(ctrl_handle, ctrl.tx);
        status = ctrl.receive_packet(ctrl_handle, 3, ctrl.rx);

        focus_step = 3216 | MOTOR_CCW;
        ctrl.tx = data_packet(CMD_FOCUS_CTRL, focus_step);
        ctrl.send_packet(ctrl_handle, ctrl.tx);
        status = ctrl.receive_packet(ctrl_handle, 3, ctrl.rx);

        // try to step the focus motor
        zoom_step = 3216 | MOTOR_CW;
        ctrl.tx = data_packet(CMD_ZOOM_CTRL, zoom_step);
        ctrl.send_packet(ctrl_handle, ctrl.tx);
        status = ctrl.receive_packet(ctrl_handle, 3, ctrl.rx);

        zoom_step = 3216 | MOTOR_CCW;
        ctrl.tx = data_packet(CMD_ZOOM_CTRL, zoom_step);
        ctrl.send_packet(ctrl_handle, ctrl.tx);
        status = ctrl.receive_packet(ctrl_handle, 3, ctrl.rx);

        // send the motor disable command
        ctrl.tx = data_packet(CMD_MOTOR_ENABLE, 1, { DISABLE_MOTOR });
        ctrl.send_packet(ctrl_handle, ctrl.tx);
        status = ctrl.receive_packet(ctrl_handle, 3, ctrl.rx);
*/

    }
    catch (std::exception e)
    {
        std::cout << "Error: " << e.what() << std::endl;
    }

	close_com_port(ctrl_handle);

    std::cout << "Program Compete!" << std::endl;
    return 0;

}   // end of main
