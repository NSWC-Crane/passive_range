// ----------------------------------------------------------------------------------------
#define _CRT_SECURE_NO_WARNINGS

// FTDI Driver Includes
#include "ftd2xx_functions.h"

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)

#elif defined(__linux__)

#endif

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"
#include <cstdint>
#include <iostream>
#include <sstream>
#include <thread>
#include <atomic>

// OpenCV Includes
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

// Custom Includes
#include "spinnaker_utilities.h"
#include "num2string.h"
#include "get_current_time.h"
#include "file_parser.h"
#include "file_ops.h"

// Project Includes
#include "control_driver.h"

// ----------------------------------------------------------------------------
// GLOBALS
// ----------------------------------------------------------------------------
std::atomic<bool> entry(false);
std::atomic<bool> run(true);

std::atomic<bool> image_capture(false);
// global variable to store the status of the image aquisition thread
std::atomic<bool> image_aquisition_complete(false);
// global variable for the Spinnacker camera
Spinnaker::CameraPtr cam;
// global variable for the opencv image container
cv::Mat cv_image;
// global variables to store the image properties
uint64_t width, height, x_offset, y_offset;
uint32_t x_padding, y_padding;
uint8_t cv_type;

std::string console_input;

// ----------------------------------------------------------------------------
void image_aquisition()
{
    std::string image_window = "Image Capture";
    Spinnaker::ImagePtr image;

    // create the window
    cv::namedWindow(image_window, cv::WindowFlags::WINDOW_NORMAL);

    cv::resizeWindow(image_window, 512, 512);

    while (image_capture)
    {
        //if (!image_aquisition_complete)
        //{
            aquire_software_trigger_image(cam, image);

            //image data contains padding. When allocating Mat container size, you need to account for the X,Y image data padding. 
            cv_image = cv::Mat(height + y_padding, width + x_padding, cv_type, image->GetData(), image->GetStride());

            cv::imshow(image_window, cv_image);

            cv::waitKey(1);
            image_aquisition_complete = true;
        //}
    }

    // destroy the window
    cv::destroyWindow(image_window);
}

// ----------------------------------------------------------------------------
void print_progress(double progress)
{
    uint32_t bar_width = 70;

    if (progress == 0.0)
    {
        std::cout << "[" << std::string(bar_width, ' ') << "] " << std::fixed << std::setprecision(2) << (progress * 100.0) << "%\r";
        std::cout.flush();
        return;
    }
    else
    {
        uint32_t bar_count = (uint32_t)(bar_width * progress);

        std::cout << "[" << std::string(bar_count, '=') << std::string(bar_width - bar_count, ' ') << "] " << std::fixed << std::setprecision(2) << (progress * 100.0) << "%\r";
        std::cout.flush();
    }

}

// ----------------------------------------------------------------------------
void read_linear_stage_params(std::string filename, std::vector<double> &coeffs)
{
    uint32_t idx;
    coeffs.clear();

    try
    {
        std::vector<std::vector<std::string>> params;
        parse_csv_file(filename, params);

        for (idx = 0; idx < params[0].size(); ++idx)
        {
            coeffs.push_back(std::stod(params[0][idx]));
        }
    }
    catch (std::exception e)
    {
        std::cout << "error reading the linear stage calibration file: " << e.what() << std::endl;
    }

}   // end of read_linear_stage_params

// ----------------------------------------------------------------------------
double calculate_stage_point(double range, std::vector<double> coeffs)
{
    double step = 0.0;

    // x^4
    step += coeffs[0] * range * range * range * range;
    // x^3
    step += coeffs[1] * range * range * range;
    // x^2
    step += coeffs[2] * range * range;
    // x^1
    step += coeffs[3] * range;
    // x^0
    step += coeffs[4];

    return step;
}

// ----------------------------------------------------------------------------
void get_input(void)
{
    while (1)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(50));
        if (run == true)
        {
            std::getline(std::cin, console_input, '\n');
            entry = true;
        }
        else
        {
            break;
        }
    }
}


// ----------------------------------------------------------------------------
int main(int argc, char** argv)
{

    uint32_t idx;
    uint32_t exp_idx, focus_idx, zoom_idx, img_idx;

    //std::string console_input;
    std::ofstream data_log_stream;

    // FTDI variables
    uint32_t ftdi_device_count = 0;
    //ftdiDeviceDetails controller_details;
    uint32_t controller_device_num = 0;
    uint32_t connect_count = 0;
    uint32_t read_timeout = 60000;
    uint32_t write_timeout = 1000;
    std::vector<ftdiDeviceDetails> ftdi_devices;
    bool status = false;

    // Controller Variables
    FT_HANDLE ctrl_handle = NULL;
    controller ctrl;
    bool ctrl_connected = false;

    std::string linear_stage_filename = "linear_stage_cal.txt";
    std::vector<double> coeffs;
    std::vector<uint32_t> linear_stage_range;
    uint32_t range_idx;

    // motor variables
    std::string pid_config_filename = "pid_config.txt";
    std::vector<int32_t> focus_range, zoom_range;
    motor_info focus_motor;
    motor_info zoom_motor;
    int32_t focus_step = 0;
    int32_t zoom_step = 0;
    bool mtr_moving = false;
    std::vector<uint16_t> pid_values;
    double focus_progress = 0.0;

    // trigger variables
    std::vector<uint8_t> tc_ch1(9);
    std::vector<uint8_t> tc_ch2(9);
    trigger_info t1_info;
    trigger_info t2_info;
    uint8_t t1_polarity, t2_polarity;
    uint32_t t1_offset, t2_offset;
    uint32_t t1_length, t2_length;

    // camera variables
    uint32_t cam_index;
    uint32_t num_cams;
    //uint64_t width, height, x_offset, y_offset;
    //uint32_t x_padding, y_padding;
    uint32_t ts = 0;
    //uint32_t sharpness;
    double camera_temp;
//    std::vector<double> exp_time;
    double exp_time;
    uint32_t cap_num;
    std::vector<std::string> cam_sn;
    //Spinnaker::CameraPtr cam;
    Spinnaker::PixelFormatEnums pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_BGR8;
    double camera_gain;
    double frame_rate;

    //Spinnaker::GainAutoEnums gain_mode = Spinnaker::GainAutoEnums::GainAuto_Once;
    Spinnaker::GainAutoEnums gain_mode = Spinnaker::GainAutoEnums::GainAuto_Continuous;

    //Spinnaker::ExposureAutoEnums exp_mode = Spinnaker::ExposureAutoEnums::ExposureAuto_Off;
    //Spinnaker::ExposureAutoEnums exp_mode = Spinnaker::ExposureAutoEnums::ExposureAuto_Once;
    Spinnaker::ExposureAutoEnums exp_mode = Spinnaker::ExposureAutoEnums::ExposureAuto_Continuous;

    Spinnaker::AdcBitDepthEnums bit_depth = Spinnaker::AdcBitDepthEnums::AdcBitDepth_Bit12;

    Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_Continuous;
    //Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_MultiFrame;
    //Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_SingleFrame;

    Spinnaker::TriggerSourceEnums trigger_source;
    Spinnaker::TriggerActivationEnums trigger_activation = Spinnaker::TriggerActivation_RisingEdge;
    //Spinnaker::ImagePtr image;
    Spinnaker::SystemPtr system = Spinnaker::System::GetInstance();
    Spinnaker::CameraList cam_list;
    bool cam_connected = false;

    // OpenCV Variables
    char key, key2;
    //cv::Mat cv_image;
    cv::Size img_size;
    std::string image_window = "Camera: ";
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(4);
    //uint8_t cv_type = CV_8UC3;

    std::string sdate, stime;
    std::string log_filename = "camera_capture_log_";
    std::string image_header = "image_";
    std::string image_capture_name = "image_";
    std::string output_save_location = "";
    std::string focus_str;
    std::string zoom_str;
    std::string exposure_str;
    std::string image_str;

    std::string img_save_folder;
    std::string sub_dir;
    int32_t stat;

    const std::string params =
        "{help h ?   |  | Display Usage message }"
        "{cfg_file   |  | Alternate input method to supply all parameters, all parameters must be included in the file }"
        "{focus_step | 2000:3200:40575 | focus motor step range }"
        "{zoom_step  | 0:925:4625 | zoom motor step range }"
        "{x_off      | 0 | X offset for camera }"
        "{y_off      | 0 | Y offset for camera }"
        "{width      | 2048 | Width of the captured image }"
        "{height     | 1536 | Height of the captured image }"
        "{px_format  | 3 | Pixel Type }"
        //"{sharpness  | 3072 | Sharpness setting for the camera }"
        //"{fps        | 10.0 | Frames per second setting for the camera }"
        //"{exp_time   | 15000:-2000:1000 | Exposure time (us) range settings for the camera }"
        "{linear_stage | 500:10:1100 | Linear stage simulated range (m) }"
        "{gain       | 5.0 | Inital gain setting before letting the camera find one }"
        "{cap_num    | 1 | Number of images to capture for an average }"
        "{source     | 1  | source for the trigger (0 -> Line0, 1 -> Software) }"
        "{output     | ../results/       | Output directory to save lidar images }"
        ;

    // use opencv's command line parser
	// to use the individual parameters you can enter the following:
	// -parameter=N
    cv::CommandLineParser parser(argc, argv, params);

    if (parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }

    focus_range.clear();
    zoom_range.clear();

    std::cout << "------------------------------------------------------------------" << std::endl;
    
    // if the input is a config file use this over all other input parameters
    if (parser.has("cfg_file"))
    {

        // input config file should contain all required inputs
        std::string cfg_filename = parser.get<std::string>("cfg_file");
        std::vector<std::vector<std::string>> cfg_params;
        
        std::cout << "reading parameters from: " << cfg_filename << std::endl;
        parse_csv_file(cfg_filename, cfg_params);

        // config file should be in the following format
        // Line 1: colon separated values of the focus motor step range (start:inc:stop)
        // Line 2: colon separated values of the zoom motor step range (start:inc:stop)
        // Line 3: comma separated values for the x offset, y offset, width, height of the camera
        // Line 4: comma separated values for the camera properties: exposure time range (us) (start:inc:stop), gain
        // Line 5: number of images to capture for each focus/zoom/exposure setting
        // Line 6: trigger source (0 -> Line0, 1 -> Software)
        // Line 7: base directory where the results will be saved
        if (cfg_params.size() == 7)
        {
            // line 1: focus motor step values
            focus_str = cfg_params[0][0];
            validate_input_range(cfg_params[0][0], min_focus_steps, max_focus_steps, focus_range);

            // line 2: zoom motor step values
            zoom_str = cfg_params[1][0];
            validate_input_range(cfg_params[1][0], min_zoom_steps, max_zoom_steps, zoom_range);

            // line 3: image size/location configuration
            x_offset = std::stoi(cfg_params[2][0]);
            y_offset = std::stoi(cfg_params[2][1]);
            width = std::stoi(cfg_params[2][2]);
            height = std::stoi(cfg_params[2][3]);

            uint32_t px_type = std::stoi(cfg_params[2][4]);

            switch (px_type)
            {
            case 0:
                pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_Mono8;
                cv_type = CV_8UC1;
                break;

            case 1:
                pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_Mono10p;
                cv_type = CV_16UC1;
                break;

            case 2:
                pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_Mono12p;
                cv_type = CV_16UC1;
                break;

            case 3:
                pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_Mono16;
                cv_type = CV_16UC1;
                break;

            case 4:
                pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_BGR8;
                cv_type = CV_8UC3;
                break;

            default:
                pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_BGR8;
                cv_type = CV_8UC3;
                break;
            }

            // line 4: camera properties settings
            //sharpness = std::stoi(cfg_params[2][0]);
            //frame_rate = std::stod(cfg_params[2][1]);
            //exposure_str = cfg_params[3][0];
            parse_input_range(cfg_params[3][0], linear_stage_range);
            //exp_time = std::stod(cfg_params[3][0]);
            //camera_gain = std::stod(cfg_params[3][1]);

            //if (camera_gain < 0)
            //{
            //    gain_mode = Spinnaker::GainAutoEnums::GainAuto_Once;
            //}
            //else
            //{
            //    gain_mode = Spinnaker::GainAutoEnums::GainAuto_Off;
            //}

            // line 5: number of images to capture for each focus/zoom/exposure setting
            cap_num = std::stoi(cfg_params[4][0]);

            // line 6: trigger source
            ts = std::stoi(cfg_params[5][0]);
            if (ts == 0)
            {
                trigger_source = Spinnaker::TriggerSourceEnums::TriggerSource_Line0;
                // parse the trigger settings
                if (cfg_params[5].size() == 7)
                {
                    //tc_ch1[0] = std::stoi(cfg_params[5][1]) & 0x01;

                    // get the trigger line polarity
                    tc_ch1[0] = t1_polarity = std::stoi(cfg_params[5][1]) & 0x01;

                    // get the trigger offset
                    t1_offset = min((uint32_t)(std::stoi(cfg_params[5][2]) & 0x0000FFFF), max_offset);
                    tc_ch1[1] = (t1_offset >> 24) & 0xFF;
                    tc_ch1[2] = (t1_offset >> 16) & 0xFF;
                    tc_ch1[3] = (t1_offset >> 8) & 0xFF;
                    tc_ch1[4] = t1_offset & 0xFF;

                    // get the trigger length
                    t1_length = min((uint32_t)(std::stoi(cfg_params[5][3]) & 0x0000FFFF), max_length);// +offset;
                    tc_ch1[5] = (t1_length >> 24) & 0xFF;
                    tc_ch1[6] = (t1_length >> 16) & 0xFF;
                    tc_ch1[7] = (t1_length >> 8) & 0xFF;
                    tc_ch1[8] = t1_length & 0xFF;

                    // get the trigger line polarity
                    tc_ch2[0] = t2_polarity = std::stoi(cfg_params[5][4]) & 0x01;

                    // get the trigger offset
                    t2_offset = min((uint32_t)(std::stoi(cfg_params[5][5]) & 0x0000FFFF), max_offset);
                    tc_ch2[1] = (t2_offset >> 24) & 0xFF;
                    tc_ch2[2] = (t2_offset >> 16) & 0xFF;
                    tc_ch2[3] = (t2_offset >> 8) & 0xFF;
                    tc_ch2[4] = t2_offset & 0xFF;

                    // get the trigger length
                    t2_length = min((uint32_t)(std::stoi(cfg_params[5][6]) & 0x0000FFFF), max_length);// +offset;
                    tc_ch2[5] = (t2_length >> 24) & 0xFF;
                    tc_ch2[6] = (t2_length >> 16) & 0xFF;
                    tc_ch2[7] = (t2_length >> 8) & 0xFF;
                    tc_ch2[8] = t2_length & 0xFF;
                }
                else
                {
                    tc_ch1.clear();
                    tc_ch1 = { 0, 0,0,0,0, 0,0,0x61,0xA8 };
                    tc_ch2.clear();
                    tc_ch2 = { 0, 0,0,0,0, 0,0,0x61,0xA8 };
                }
            }
            else
            {
                trigger_source = Spinnaker::TriggerSourceEnums::TriggerSource_Software;
            }

            // line 7: output save location
            output_save_location = cfg_params[6][0];
        }
        else
        {
            std::cout << "The number of supplied parameters in the file does not meet the required criteria: N = " << cfg_params.size() << std::endl;
            std::cin.ignore();
            return -1;
        }
    }
    else
    {
        // line 1: focus motor step values
        focus_str = parser.get<string>("focus_step");
        validate_input_range(parser.get<string>("focus_step"), min_focus_steps, max_focus_steps, focus_range);

        // line 2: zoom motor step values
        zoom_str = parser.get<string>("zoom_step");
        validate_input_range(parser.get<string>("zoom_step"), min_zoom_steps, max_zoom_steps, zoom_range);

        // line 3: image size/location configuration
        x_offset = parser.get<uint32_t>("x_off");		// 40
        y_offset = parser.get<uint32_t>("y_off");		// 228;
        width = parser.get<uint32_t>("width");		    // 1200;
        height = parser.get<uint32_t>("height");		// 720;

        uint32_t px_type = parser.get<uint32_t>("px_format");

        switch (px_type)
        {
        case 0:
            pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_Mono8;
            cv_type = CV_8UC1;
            break;

        case 1:
            pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_Mono10p;
            cv_type = CV_16UC1;
            break;

        case 2:
            pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_Mono12p;
            cv_type = CV_16UC1;
            break;

        case 3:
            pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_Mono16;
            cv_type = CV_16UC1;
            break;

        case 4:
            pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_BGR8;
            cv_type = CV_8UC3;
            break;

        default:
            pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_BGR8;
            cv_type = CV_8UC3;
            break;
        }

        // line 4: camera properties settings
        //sharpness = parser.get<uint32_t>("sharpness");
        //frame_rate = parser.get<double>("fps");
        //exposure_str = parser.get<string>("exp_time");
        parse_input_range(parser.get<string>("linear_stage"), linear_stage_range);
        //exp_time = parser.get<double>("exp_time");
        //camera_gain = parser.get<double>("gain");

        //if (camera_gain < 0)
        //{
        //    gain_mode = Spinnaker::GainAutoEnums::GainAuto_Once;
        //}
        //else
        //{
        //    gain_mode = Spinnaker::GainAutoEnums::GainAuto_Off;
        //}

        // line 5: number of images to capture for each focus/zoom/exposure settinge
        cap_num = parser.get<uint32_t>("cap_num");

        // line 6: trigger source
        ts = parser.get<uint32_t>("source");
        if (ts == 0)
            trigger_source = Spinnaker::TriggerSourceEnums::TriggerSource_Line0;
        else
            trigger_source = Spinnaker::TriggerSourceEnums::TriggerSource_Software;

        tc_ch1.clear();
        tc_ch1 = { 0, 0,0,0,0, 0,0,0x61,0xA8 };
        tc_ch2.clear();
        tc_ch2 = { 0, 0,0,0,0, 0,0,0x61,0xA8 };
        
        // line 7: output save location
        output_save_location = parser.get<std::string>("output");
    }

    output_save_location = path_check(output_save_location);

    int32_t dir_status = mkdir(output_save_location);
    if (dir_status != 0)
        std::cout << "Error creating folder: " << dir_status << std::endl;

    get_current_time(sdate, stime);
    log_filename = log_filename + sdate + "_" + stime + ".txt";

    std::cout << "Log File: " << (output_save_location + log_filename) << std::endl << std::endl;
    data_log_stream.open((output_save_location + log_filename), ios::out | ios::app);

    // Add the date and time to the start of the log file
    data_log_stream << "#------------------------------------------------------------------------------" << std::endl;
    data_log_stream << "Version: 1.0    Date: " << sdate << "    Time: " << stime << std::endl << std::endl;

    // print out the info about what was passed in to the code
    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << "Input Parameters:" << std::endl;
    std::cout << "Image Size (h x w):  " << height << " x " << width << std::endl;
    std::cout << "Image Offset (x, y): " << x_offset << ", " << y_offset << std::endl;
    std::cout << "Focus Step Range:    " << focus_str << std::endl;
    std::cout << "Zoom Step Range:     " << zoom_str << std::endl;
    //std::cout << "Exposure Time Range: " << exposure_str << std::endl;
    //std::cout << "Camera Gain:         " << camera_gain << std::endl;
    std::cout << "Number of Captures:  " << cap_num << std::endl;
    if(ts == 1)
    {
        std::cout << "Trigger Source:      " << ts << std::endl;
    }
    else
    {
        std::cout << "Trigger Source:      " << ts << ", " << (uint32_t)t1_polarity << "," << t1_offset << "," << t1_length;
        std::cout << ", " << (uint32_t)t2_polarity << "," << t2_offset << "," << t2_length <<std::endl;
    }
    std::cout <<  std::endl;

    data_log_stream << "#------------------------------------------------------------------------------" << std::endl;
    data_log_stream << "Input Parameters:" << std::endl;
    data_log_stream << "Image Size (h x w):  " << height << " x " << width << std::endl;
    data_log_stream << "Image Offset (x, y): " << x_offset << ", " << y_offset << std::endl;
    data_log_stream << "Focus Step Range:    " << focus_str << std::endl;
    data_log_stream << "Zoom Step Range:     " << zoom_str << std::endl;
    //data_log_stream << "Exposure Time Range: " << exposure_str << std::endl;
    //data_log_stream << "Camera Gain:         " << camera_gain << std::endl;
    data_log_stream << "Number of Captures:  " << cap_num << std::endl;
    if(ts == 1)
    {
        data_log_stream << "Trigger Source:      " << ts << std::endl;
    }
    else
    {
        data_log_stream << "Trigger Source:      " << ts << ", " << (uint32_t)t1_polarity << "," << t1_offset << "," << t1_length;
        data_log_stream << ", " << (uint32_t)t2_polarity << "," << t2_offset << "," << t2_length <<std::endl;
    }
    data_log_stream << std::endl;


    try {

// ----------------------------------------------------------------------------------------
// Scan the system and get the motor controller connected to the computer
// ----------------------------------------------------------------------------------------
        std::cout << "------------------------------------------------------------------" << std::endl;
        ftdi_device_count = get_device_list(ftdi_devices);
        if (ftdi_device_count == 0)
        {
            std::cout << "No ftdi devices found... Press Enter to Exit!" << std::endl;
            data_log_stream << "No ftdi devices found... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        for (idx = 0; idx < ftdi_devices.size(); ++idx)
        {
            std::cout << ftdi_devices[idx];
        }

        std::cout << "Select Controller: ";
        std::getline(std::cin, console_input);
        controller_device_num = stoi(console_input);

        if ((controller_device_num < 0) || (controller_device_num > ftdi_devices.size() - 1))
        {
            std::cout << "The device number specified is beyond the range of available FTDI devices!  Exiting..." << std::endl;
            data_log_stream << "The device number specified is beyond the range of available FTDI devices!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        // this needs to happen so that zero is that same for each run
        std::cout << std::endl << "Rotate the focus and the zoom lens to the zero position.  " << std::endl 
            << "If the lens won't turn, remove power from the motors and then reconnect. Press Enter when complete...";
        std::cin.ignore();

        std::cout << std::endl << "Connecting to Controller..." << std::endl;
        ftdi_devices[controller_device_num].baud_rate = 250000;
        while ((ctrl_handle == NULL) && (connect_count < 10))
        {
            ctrl_handle = open_com_port(ftdi_devices[controller_device_num], read_timeout, write_timeout);
            ++connect_count;
        }

        if (ctrl_handle == NULL)
        {
            std::cout << "No Controller found... Press Enter to Exit!" << std::endl;
            data_log_stream << "No Controller found... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        ctrl.tx = data_packet(DRIVER_CONNECT);

        // send connection request packet and get response back
        ctrl.send_packet(ctrl_handle, ctrl.tx);
        status = ctrl.receive_packet(ctrl_handle, 6, ctrl.rx);

        if (status == false)
        {
            std::cout << "No Controller found... Press Enter to Exit!" << std::endl;
            data_log_stream << "No Controller found... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        // get the controller information and display
        ctrl.set_driver_info(ctrl.rx);
        std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << ctrl;
        std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;

        data_log_stream << "#------------------------------------------------------------------------------" << std::endl;
        data_log_stream << ctrl << std::endl;
        //data_log_stream << "#------------------------------------------------------------------------------" << std::endl ;

        sleep_ms(20);
        
        //-----------------------------------------------------------------------------
        // ping the motors to get the model number and firmware version
        status = ctrl.ping_motor(ctrl_handle, FOCUS_MOTOR_ID, focus_motor);

        std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << "Focus Motor Information: " << std::endl;

        data_log_stream << "#------------------------------------------------------------------------------" << std::endl;
        data_log_stream << "Focus Motor Information: " << std::endl;

        if (status)
        {
            std::cout << focus_motor;
            data_log_stream << focus_motor;
        }
        else
        {
            std::cout << "  Error getting focus motor info" << std::endl;
            data_log_stream << "  Error getting focus motor info" << std::endl;
        }

        // configure the homing offsets for the focus motor
        // set the offset to zero
        status = ctrl.set_offset(ctrl_handle, FOCUS_MOTOR_ID);

        // get the current motor positions
        status = ctrl.get_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);

        std::cout << "  Current Step:     " << focus_step << std::endl;
        data_log_stream << "  Current Step:     " << focus_step << std::endl;

        sleep_ms(20);
        
        //-----------------------------------------------------------------------------
        status = ctrl.ping_motor(ctrl_handle, ZOOM_MOTOR_ID, zoom_motor);

        std::cout << std::endl;
        std::cout << "Zoom Motor Information: " << std::endl;
        //data_log_stream << "-----------------------------------------------------------------------------" << std::endl;
        data_log_stream << "Zoom Motor Information: " << std::endl;
        
        if (status)
        {
            std::cout << zoom_motor;
            data_log_stream << zoom_motor;
        }
        else
        {
            std::cout << "  Error getting zoom motor info" << std::endl;
            data_log_stream << "  Error getting zoom motor info" << std::endl;
        }

        // configure the homing offsets for the zoom motor
        // set the offset to zero
        status = ctrl.set_offset(ctrl_handle, ZOOM_MOTOR_ID);

        // get the current motor positions
        status = ctrl.get_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_step);

        std::cout << "  Current Step:     " << zoom_step << std::endl;
        std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;

        data_log_stream << "  Current Step:     " << zoom_step << std::endl << std::endl;
        //data_log_stream << "-----------------------------------------------------------------------------" << std::endl ;

        /*
        //-----------------------------------------------------------------------------
        // ping the motors to get the model number and firmware version
        status = ctrl.ping_motor(ctrl_handle, FOCUS_MOTOR_ID, focus_motor);

        std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << "Focus Motor Information: " << std::endl;
        data_log_stream << "-----------------------------------------------------------------------------" << std::endl;
        data_log_stream << "Focus Motor Information: " << std::endl;

        if (status)
        {
            std::cout << focus_motor;
            data_log_stream << focus_motor;
        }
        else
        {
            std::cout << "  Error getting focus motor info" << std::endl;
            data_log_stream << "  Error getting focus motor info" << std::endl;
        }
        std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;
        data_log_stream << "-----------------------------------------------------------------------------" << std::endl << std::endl;


        status = ctrl.ping_motor(ctrl_handle, ZOOM_MOTOR_ID, zoom_motor);

        std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << "Zoom Motor Information: " << std::endl;
        data_log_stream << "-----------------------------------------------------------------------------" << std::endl;
        data_log_stream << "Zoom Motor Information: " << std::endl;

        if (status)
        {
            std::cout << zoom_motor;
            data_log_stream << zoom_motor;
        }
        else
        {
            std::cout << "  Error getting zoom motor info" << std::endl;
            data_log_stream << "  Error getting zoom motor info" << std::endl;
        }
        std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;
        data_log_stream << "-----------------------------------------------------------------------------" << std::endl << std::endl;

        //-----------------------------------------------------------------------------
        // get the current motor positions
        status = ctrl.get_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);
        status = ctrl.get_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_step);

        std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << "Focus Step: " << focus_step << ", Zoom Step: " << zoom_step << std::endl;
        std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;

        data_log_stream << "-----------------------------------------------------------------------------" << std::endl;
        data_log_stream << "Focus Step: " << focus_step << ", Zoom Step: " << zoom_step << std::endl;
        data_log_stream << "-----------------------------------------------------------------------------" << std::endl << std::endl;
        */
        
        // configure the trigger according to the inputs
        // channel 1
        status = ctrl.config_channel(ctrl_handle, CONFIG_T1, tc_ch1);

        // channel 2
        status = ctrl.config_channel(ctrl_handle, CONFIG_T2, tc_ch2);
        
        //-----------------------------------------------------------------------------
        // get the current trigger configurations and display the information
        status = ctrl.get_trigger_info(ctrl_handle, t1_info, t2_info);

        std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << "Trigger Information: " << std::endl;
        std::cout << t1_info << std::endl;
        std::cout << t2_info;
        std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;

        data_log_stream << "#------------------------------------------------------------------------------" << std::endl;
        data_log_stream << "Trigger Information: " << std::endl;
        data_log_stream << t1_info << std::endl;
        data_log_stream << t2_info << std::endl;

        //-----------------------------------------------------------------------------
        // read in the position PID config file and set the PID values for each motor 
        status = read_pid_config(pid_config_filename, (uint32_t)(ctrl.ctrl_info.serial_number - 1), pid_values);

        if (status == false)
        {
            std::cout << "The pid_config.txt file does not have enough entries based on supplied serial number.  Using default values." << std::endl;

            // fill in the pid_values with default values
            pid_values.push_back(0);
            pid_values.push_back(2);
            pid_values.push_back(800);
            pid_values.push_back(0);
            pid_values.push_back(2);
            pid_values.push_back(800);
        }

        // config the motors with the specified pid values
        ctrl.set_pid_value(ctrl_handle, FOCUS_MOTOR_ID, ADD_POSITION_D, pid_values[0]);
        ctrl.set_pid_value(ctrl_handle, FOCUS_MOTOR_ID, ADD_POSITION_I, pid_values[1]);
        ctrl.set_pid_value(ctrl_handle, FOCUS_MOTOR_ID, ADD_POSITION_P, pid_values[2]);
        ctrl.set_pid_value(ctrl_handle, ZOOM_MOTOR_ID, ADD_POSITION_D, pid_values[3]);
        ctrl.set_pid_value(ctrl_handle, ZOOM_MOTOR_ID, ADD_POSITION_I, pid_values[4]);
        ctrl.set_pid_value(ctrl_handle, ZOOM_MOTOR_ID, ADD_POSITION_P, pid_values[5]);

        //-----------------------------------------------------------------------------
        // start off at the fist supplied lens position for both focus and zoom motors
        status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, true);
        status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, true);

        std::cout << "Setting motors to initial position:" << std::endl;
        std::cout << "focus motor: " << focus_range[0] << std::endl;
        std::cout << "zoom motor: " << zoom_range[0] << std::endl;
        std::cout << std::endl;

        status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_range[0]);
        status &= ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_range[0]);

        if (!status)
        {
            std::cout << "Error setting motor positions." << std::endl;
        }

        // disable the motors
        status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);
        status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);

        ctrl_connected = true;

// ----------------------------------------------------------------------------------------
// Scan the system and get the camera connected to the computer
// ----------------------------------------------------------------------------------------

        std::cout << "------------------------------------------------------------------" << std::endl;
        // Print out current library version
        std::cout << system->GetLibraryVersion() << std::endl;

        cam_list = system->GetCameras();
        
        num_cams = get_camera_selection(cam_list, cam_index, cam_sn);

        // Finish if there are no cameras
        if ((num_cams == 0) || (cam_index < 0) || (cam_index > num_cams - 1))
        {
            // Clear camera list before releasing system
            cam_list.Clear();

            // Release system
            system->ReleaseInstance();

            std::cout << "No Camera found... Press Enter to Exit!" << std::endl;
            data_log_stream << "No Camera found... Exiting!" << std::endl;
            std::cin.ignore();

            return -1;
        }

        // get the selected camera
        cam = cam_list.GetByIndex(cam_index);

        // print out some information about the camera
        std::cout << "------------------------------------------------------------------" << std::endl;
        std::cout << cam;
        data_log_stream << "#------------------------------------------------------------------------------" << std::endl;
        data_log_stream << cam;

        // initialize the camera
        init_camera(cam);
        cam_connected = true;
        //cam->TriggerActivation.SetValue(Spinnaker::TriggerActivationEnums::TriggerActivation_RisingEdge);

        get_temperature(cam, camera_temp);
        std::cout << "  Camera Temp:       " << camera_temp << std::endl << std::endl;
        data_log_stream << "  Camera Temp:       " << camera_temp << std::endl << std::endl;

        // config the image size
        //get_image_size(cam, height, width, y_offset, x_offset);
        //std::cout << "Image Size (h x w): " << height << " x " << width << ", [" << x_offset << ", " << y_offset << "]" << std::endl;

        // configure the camera
        set_pixel_format(cam, pixel_format);
        set_image_size(cam, height, width, y_offset, x_offset);
        set_gain_mode(cam, gain_mode);
        set_exposure_mode(cam, exp_mode);
        //set_exposure_time(cam, exp_time);
        set_acquisition_mode(cam, acq_mode); //acq_mode
        
        // start the acquistion if the mode is set to continuous
        if(acq_mode == Spinnaker::AcquisitionModeEnums::AcquisitionMode_Continuous)
            cam->BeginAcquisition();
        
        // set trigger mode and enable
        set_trigger_source(cam, trigger_source, trigger_activation);
        //config_trigger(cam, OFF);
        config_trigger(cam, ON);
        sleep_ms(1000); // blackfy camera needs a 1 second delay after setting the trigger mode to ON
                        
        // start up the image acquisition thread
        image_capture = true;
        image_aquisition_complete = false;
        std::thread image_acquisition_thread(image_aquisition);
        while (!image_aquisition_complete);


/*        // grab an image: either from the continuous, single or triggered
        switch (ts)
        {
        case 0:
            cam->BeginAcquisition();
            status = ctrl.trigger(ctrl_handle, TRIG_ALL);
            aquire_trigger_image(cam, image);
            cam->EndAcquisition();
            break;
        case 1:
            aquire_software_trigger_image(cam, image);
            break;
        }
*/
        //sleep_ms(100);

        // if the gain is 0 or greater then them is set to off and a value must be set
        //if (camera_gain >= 0)
        //{
        //    set_gain_value(cam, camera_gain);
        //}

        // print out the camera configuration
        std::cout << "------------------------------------------------------------------" << std::endl;
        std::cout << "Camera Configuration:" << std::endl;

        data_log_stream << "#------------------------------------------------------------------------------" << std::endl;
        data_log_stream << "Camera Configuration:" << std::endl;

        get_image_size(cam, height, width, y_offset, x_offset);

        // pixel format
        get_pixel_format(cam, pixel_format);
        get_gain_value(cam, camera_gain);

        // exposure
        double tmp_exp_time = 0.0;
        get_exposure_mode(cam, exp_mode);
        get_exposure_time(cam, tmp_exp_time);
        get_acquisition_mode(cam, acq_mode);
        
        std::cout << "Image Size (h x w):       " << height << " x " << width << std::endl;
        std::cout << "Image Offset (x, y):      " << x_offset << ", " << y_offset << std::endl;
        std::cout << "Pixel Format:             " << cam->PixelFormat.GetCurrentEntry()->GetSymbolic() << std::endl;
        std::cout << "ADC Bit Depth:            " << cam->AdcBitDepth.GetCurrentEntry()->GetSymbolic() << std::endl;
        std::cout << "Gain Mode/Value:          " << cam->GainAuto.GetCurrentEntry()->GetSymbolic() << " / " << camera_gain << std::endl;
        std::cout << "Exposure Mode/Value (ms): " << cam->ExposureAuto.GetCurrentEntry()->GetSymbolic() << " / " << tmp_exp_time/1000.0 << std::endl;
//        std::cout << "Acq mode/frame rate: " << cam->AcquisitionMode.GetCurrentEntry()->GetSymbolic() << " / " << frame_rate << std::endl;
        std::cout << "Acquistion Mode:          " << cam->AcquisitionMode.GetCurrentEntry()->GetSymbolic() << std::endl;
        std::cout << "Number of Captures:       " << cap_num << std::endl;
        std::cout << "Trigger Source:           " << cam->TriggerSource.GetCurrentEntry()->GetSymbolic() << std::endl;
        std::cout << "Min/Max Gain:             " << cam->Gain.GetMin() << " / " << cam->Gain.GetMax() << std::endl;
        std::cout << "Min/Max Exposure (ms):    " << cam->ExposureTime.GetMin()/1000.0 << " / " << (uint64_t)cam->ExposureTime.GetMax()/1000.0 << std::endl;
        std::cout << std::endl;

        data_log_stream << "Image Size (h x w):       " << height << " x " << width << std::endl;
        data_log_stream << "Image Offset (x, y):      " << x_offset << ", " << y_offset << std::endl;
        data_log_stream << "Pixel Format:             " << cam->PixelFormat.GetCurrentEntry()->GetSymbolic() << std::endl;
        data_log_stream << "ADC Bit Depth:            " << cam->AdcBitDepth.GetCurrentEntry()->GetSymbolic() << std::endl;
        data_log_stream << "Gain Mode/Value:          " << cam->GainAuto.GetCurrentEntry()->GetSymbolic() << " / " << camera_gain << std::endl;
        data_log_stream << "Exposure Mode/Value (ms): " << cam->ExposureAuto.GetCurrentEntry()->GetSymbolic() << " / " << tmp_exp_time/1000.0 << std::endl;
//        data_log_stream << "Acq mode/value:      " << cam->AcquisitionMode.GetCurrentEntry()->GetSymbolic() << " / " << frame_rate << std::endl;
        data_log_stream << "Acquistion Mode:          " << cam->AcquisitionMode.GetCurrentEntry()->GetSymbolic() << std::endl;
        data_log_stream << "Number of Captures:       " << cap_num << std::endl;
        data_log_stream << "Trigger Source:           " << cam->TriggerSource.GetCurrentEntry()->GetSymbolic() << std::endl;
        data_log_stream << "Min/Max Gain:             " << cam->Gain.GetMin() << " / " << cam->Gain.GetMax() << std::endl;
        data_log_stream << "Min/Max Exposure (ms):    " << cam->ExposureTime.GetMin()/1000.0 << " / " << (uint64_t)cam->ExposureTime.GetMax()/1000.0 << std::endl;       
        data_log_stream << std::endl;

        std::cout << "------------------------------------------------------------------" << std::endl;
        std::cout << "Save location: " << output_save_location << std::endl << std::endl;
        //data_log_stream << "Save location: " << output_save_location << std::endl;

        std::cout << "------------------------------------------------------------------" << std::endl;
        std::cout << "Beginning Acquisition:" << std::endl;
        std::cout << std::endl << "Press the following keys to perform actions:" << std::endl;
        std::cout << "  f <value> - move the focus motor to the given step value [" << min_focus_steps << " - " << max_focus_steps << "]" << std::endl;
        //std::cout << "  z <value> - move the zoom motor to the given step value [" << min_zoom_steps << " - " << max_zoom_steps << "]" << std::endl;
        //std::cout << "  g <value> - Set the camera gain" << std::endl;
        //std::cout << "  e <value> - set the camera exposure time (us)" << std::endl;
        std::cout << "  s - Start the image capture sequence" << std::endl;
        std::cout << "  q - Quit" << std::endl;
        std::cout << std::endl;

        image_window = image_window + cam_sn[cam_index];

        // grab an initial image to get the padding 
        //acquire_image(cam, image);
/*
        switch (ts)
        {
        case 0:
            cam->BeginAcquisition();
            status = ctrl.trigger(ctrl_handle, TRIG_ALL);
            aquire_trigger_image(cam, image);
            cam->EndAcquisition();
            break;
        case 1:
            aquire_software_trigger_image(cam, image);
            break;
        }

        x_padding = (uint32_t)image->GetXPadding();
        y_padding = (uint32_t)image->GetYPadding();

        cv::namedWindow(image_window, cv::WindowFlags::WINDOW_NORMAL);
*/
        //std::thread io_thread(get_input);
        image_aquisition_complete = false;
        while (!image_aquisition_complete);

        // read in the file to describe the linear stage interpolation
        read_linear_stage_params(linear_stage_filename, coeffs);

        do
        {

            //acquire_image(cam, image);
            //acquire_single_image(cam, image);
/*
            switch (ts)
            {
            case 0:
                cam->BeginAcquisition();
                status = ctrl.trigger(ctrl_handle, TRIG_ALL);
                aquire_trigger_image(cam, image);
                cam->EndAcquisition();
                break;
            case 1:
                aquire_software_trigger_image(cam, image);
                break;
            }

            //image data contains padding. When allocating Mat container size, you need to account for the X,Y image data padding. 
            cv_image = cv::Mat(height + y_padding, width + x_padding, cv_type, image->GetData(), image->GetStride());

            cv::imshow(image_window, cv_image);

            key = cv::waitKey(1);
*/
            //if (entry)
            //{
            //    // get the fisrt character
            //    key = console_input[0];
            image_aquisition_complete = false;
            while (!image_aquisition_complete);

            std::getline(std::cin, console_input, '\n');
            key = console_input[0];
            //std::cin >> key;
            //std::cout << std::endl;

            switch (key)
            {
            // check to save the image
            case 's':

                // save the linear stage parameters
                data_log_stream << "#------------------------------------------------------------------------------" << std::endl;
                data_log_stream << "linear stage curve coefficients: ";
                for (idx = 0; idx < coeffs.size()-1; ++idx)
                {
                    data_log_stream << num2str(coeffs[idx], "%17.15g") << ", ";
                }
                data_log_stream << num2str(coeffs[idx], "%17.15g") << std::endl << std::endl;

                get_current_time(sdate, stime);
                img_save_folder = output_save_location + sdate + "_" + stime + "/";

                stat = mkdir(img_save_folder);
                if (stat != 0 && stat != (int32_t)ERROR_ALREADY_EXISTS)
                {
                    std::cout << "Error creating directory: " << stat << std::endl;
                }

                data_log_stream << "#------------------------------------------------------------------------------" << std::endl;
                data_log_stream << "save location: " << img_save_folder << std::endl << std::endl;
                data_log_stream << "#------------------------------------------------------------------------------" << std::endl;

                std::cout << "------------------------------------------------------------------" << std::endl;

                // enable the motors
                status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, true);
                status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, true);

                sleep_ms(10);

                focus_step = 0;
                zoom_step = 0;

                // set the focus and zoom steps to their starting values
                status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_range[0]);
                status = ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_range[0]);

                // get the actual focus and zoom position 
                //status = ctrl.get_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);
                //status = ctrl.get_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_step);

                for (range_idx = 0; range_idx < linear_stage_range.size(); ++range_idx)
                {
                    double step_value = calculate_stage_point(linear_stage_range[range_idx], coeffs);
                    std::cout << "Simulated Range = " << linear_stage_range[range_idx] << ", set linear stage to: " << num2str(step_value, "%2.4f") << std::endl;
                    std::cout << "Press 'c' to continue, press f <value> to adjust focus for alignment, or press 'q' <enter> to quit..." << std::endl;
                    
                    do
                    {
                        std::getline(std::cin, console_input, '\n');
                        key2 = console_input[0];

                        if (key2 == 'q')
                        {
                            key = 'q';
                            break;
                        }
                        else if (key2 == 'f')
                        {
                            try
                            {
                                focus_step = std::stoi(console_input.substr(2, console_input.length() - 1));
                                //status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, true);
                                status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);
                                sleep_ms(10);
                                status = ctrl.get_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);
                                //status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);
                                std::cout << "Focus step: " << focus_step << std::endl;
                            }
                            catch (std::exception e)
                            {
                                std::cout << "error converting step: " << console_input << " error msg: " << e.what() << std::endl;
                            }
                        }

                    } while (key2 != 'c');

                    //std::cout << std::endl;
                    if (key == 'q')
                        break;

                    for (zoom_idx = 0; zoom_idx < zoom_range.size(); ++zoom_idx)
                    {
                        // set the zoom motor value to each value in focus_range
                        status = ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_range[zoom_idx]);
                        status = ctrl.get_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_step);

                        zoom_str = num2str(zoom_step, "z%04d_");

                        std::cout << "Zoom setting: " << num2str(zoom_range[zoom_idx], "%04d") << std::endl;

                        // set the camera exposure time and gain to auto for each zoom setting and then freeze
                        exp_mode = Spinnaker::ExposureAutoEnums::ExposureAuto_Continuous;
                        gain_mode = Spinnaker::GainAutoEnums::GainAuto_Continuous;
                        set_exposure_mode(cam, exp_mode);
                        set_gain_mode(cam, gain_mode);

                        sub_dir = num2str((uint32_t)linear_stage_range[range_idx], "%04d") + "/" + num2str(zoom_range[zoom_idx], "z%04d") + "/";

                        stat = mkdir(img_save_folder + sub_dir);
                        if (stat != 0 && stat != (int32_t)ERROR_ALREADY_EXISTS)
                        {
                            std::cout << "Error creating directory: " << stat << std::endl;
                        }

/*                        // grab an image: either from the continuous, single or triggered
                        switch (ts)
                        {
                        case 0:
                            cam->BeginAcquisition();
                            status = ctrl.trigger(ctrl_handle, TRIG_ALL);
                            aquire_trigger_image(cam, image);
                            cam->EndAcquisition();
                            break;
                        case 1:
                            aquire_software_trigger_image(cam, image);
                            break;
                        }
*/
                        sleep_ms(40);
                        exp_mode = Spinnaker::ExposureAutoEnums::ExposureAuto_Off;
                        gain_mode = Spinnaker::GainAutoEnums::GainAuto_Off;
                        set_exposure_mode(cam, exp_mode);
                        set_gain_mode(cam, gain_mode);

                        // get the final camera exposure time and camera gain values
                        get_exposure_time(cam, tmp_exp_time);
                        exposure_str = num2str(tmp_exp_time, "e%05.0f_");
                        get_gain_value(cam, camera_gain);

                        data_log_stream << "# exposure time: " << num2str(tmp_exp_time, "%6.4f") << ", camera gain: " << num2str(camera_gain, "%6.4f") << std::endl;

                        for (focus_idx = 0; focus_idx < focus_range.size(); ++focus_idx)
                        {
                            // set the focus motor value to each value in focus_range
                            status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_range[focus_idx]);
                            status = ctrl.get_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);

                            focus_str = num2str(focus_step, "f%05d_");

                            focus_progress = focus_idx / (double)(focus_range.size() - 1);

                            print_progress(focus_progress);

                            sleep_ms(10);

                            for (img_idx = 0; img_idx < cap_num; ++img_idx)
                            {
                                image_capture_name = image_header + zoom_str + focus_str + exposure_str + num2str(img_idx, "i%02d") + ".png";

/*                                // grab an image: either from the continuous, single or triggered
                                switch (ts)
                                {
                                case 0:
                                    cam->BeginAcquisition();
                                    status = ctrl.trigger(ctrl_handle, TRIG_ALL);
                                    aquire_trigger_image(cam, image);
                                    cam->EndAcquisition();
                                    break;
                                case 1:
                                    aquire_software_trigger_image(cam, image);
                                    break;
                                }


                                cv_image = cv::Mat(height + y_padding, width + x_padding, cv_type, image->GetData(), image->GetStride());

                                cv::imshow(image_window, cv_image);
                                key = cv::waitKey(1);
*/

                                // new method to capture images
                                image_aquisition_complete = false;
                                while (!image_aquisition_complete);
                                
                                // save the image
                                //std::cout << "saving: " << img_save_folder << sub_dir << image_capture_name << std::endl;
                                data_log_stream << sub_dir << image_capture_name << std::endl;

                                cv::imwrite((img_save_folder + sub_dir + image_capture_name), cv_image, compression_params);
                                //std::cout << image_capture_name << "," << num2str(tmp_exp_time, "%2.2f") << std::endl;
                                sleep_ms(10);

                            }   // end of img_idx loop

                        }   // end of focus_idx loop

                        std::cout << std::endl;

                        // set the focus step to the first focus_range setting
                        status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_range[0]);

                    }   // end of zoom_idx loop

                }   // end of range_idx loop

                // set the focus step to the first focus_range setting
                status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_range[0]);
                status = ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_range[0]);

                // disable the motors
                status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);
                status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);

                // set the camera exposure time and gain to auto
                exp_mode = Spinnaker::ExposureAutoEnums::ExposureAuto_Continuous;
                gain_mode = Spinnaker::GainAutoEnums::GainAuto_Continuous;
                set_exposure_mode(cam, exp_mode);
                set_gain_mode(cam, gain_mode);

                std::cout << std::endl;
                std::cout << "------------------------------------------------------------------" << std::endl;
                std::cout << "Data Collection Complete!" << std::endl;
                std::cout << "------------------------------------------------------------------" << std::endl;

                data_log_stream << "#------------------------------------------------------------------------------" << std::endl;
                break;      // end of key == 's'
                

            case 'f':
                //std::cout << "Enter focus step: ";
                //std::getline(std::cin, console_input.substr(2, console_input.length()-1));
                try
                {
                    focus_step = std::stoi(console_input.substr(2, console_input.length() - 1));
                    status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, true);
                    status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);
                    sleep_ms(10);
                    status = ctrl.get_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);
                    status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);
                    std::cout << "Focus step: " << focus_step << std::endl;
                }
                catch (std::exception e)
                {
                    std::cout << "error converting step: " << console_input << " error msg: " << e.what() << std::endl;
                }
                break;      // end of key == 'f'
               

            //case 'z':

            //    //std::cout << "Enter zoom step: ";
            //    //std::getline(std::cin, console_input);
            //    try
            //    {
            //        zoom_step = std::stoi(console_input.substr(2, console_input.length() - 1));
            //        status = ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, true);
            //        status = ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_step);
            //        sleep_ms(10);
            //        status = ctrl.get_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_step);
            //        status = ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);
            //        std::cout << "Zoom step: " << zoom_step << std::endl;
            //    }
            //    catch (std::exception e)
            //    {
            //        std::cout << "error converting step: " << console_input << " error msg: " << e.what() << std::endl;
            //    }
            //    break;

                //case 'e':
                //    //std::cout << "Enter exposure time (us): ";
                //    //std::getline(std::cin, console_input);
                //    try
                //    {
                //        double tmp_exp = floor(std::stod(console_input.substr(2, console_input.length() - 1)));
                //        set_exposure_time(cam, tmp_exp);
                //        sleep_ms(10);
                //        get_exposure_time(cam, tmp_exp);
                //        std::cout << "Exposure time (us): " << tmp_exp << std::endl;

                //    }
                //    catch (std::exception e)
                //    {
                //        std::cout << "error converting exposure time: " << console_input << " error msg: " << e.what() << std::endl;
                //    }
                //    break;

                //case 'g':
                //    //std::cout << "Enter gain: ";
                //    //std::getline(std::cin, console_input);
                //    try
                //    {
                //        double tmp_gain = std::stod(console_input.substr(2, console_input.length() - 1));
                //        set_gain_value(cam, tmp_gain);
                //        sleep_ms(10);
                //        get_gain_value(cam, tmp_gain);
                //        std::cout << "Gain: " << tmp_gain << std::endl;
                //    }
                //    catch (std::exception e)
                //    {
                //        std::cout << "error converting gain value: " << console_input << " error msg: " << e.what() << std::endl;
                //    }

                //    break;

                case 'q':
                    run = false;
                    break;

                default:
                    break;

                }   // end of switch case

                sleep_ms(10);
            //    entry = false;
            //}   // end if(entry)

//        } while (run);
        } while (key != 'q');

        //io_thread.join();
        image_capture = false;
        image_acquisition_thread.join();

        sleep_ms(20);

        if (acq_mode == Spinnaker::AcquisitionModeEnums::AcquisitionMode_Continuous)
            cam->EndAcquisition();

        // deactivate the camera trigger
        config_trigger(cam, OFF);

    }
    catch (Spinnaker::Exception &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        data_log_stream << "Error: " << e.what() << std::endl;

        std::cout << "Press Enter to close!" << std::endl;
        std::cin.ignore();
    }
    catch (std::exception e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        data_log_stream << "Error: " << e.what() << std::endl;

        std::cout << "Press Enter to close!" << std::endl;
        std::cin.ignore();
    }
    
    // set the focus and zoom steps to zero
    focus_step = 0;
    zoom_step = 0;
    
    std::cout << std::endl << "Setting motors back to zero..." << std::endl;
    status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, true);
    status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);
    status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);

    status = ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, true);    
    status = ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_step);
    status = ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);
    
    // close the motor driver port first
    std::cout << std::endl << "Closing the Controller port..." << std::endl;
    close_com_port(ctrl_handle);
    ctrl_connected = false;

    // close out the camera
    std::cout << std::endl << "Closing Camera..." << std::endl;

    // de-initialize the camera
    cam->DeInit();

    // Release reference to the camera
    cam = nullptr;

    // Clear camera list before releasing system
    cam_list.Clear();

    // Release system
    system->ReleaseInstance();
    cam_connected = false;
      
    cv::destroyAllWindows();

    data_log_stream.close();
    
    std::cout << std::endl << "Program Complete!" << std::endl;

    return 0;

}   // end of main
