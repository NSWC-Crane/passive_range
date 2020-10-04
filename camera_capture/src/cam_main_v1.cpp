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
//#include "motor_driver.h"
//#include "trigger_ctrl.h"
#include "control_driver.h"

// ----------------------------------------------------------------------------
int main(int argc, char** argv)
{

    uint32_t idx;
    uint32_t exp_idx, focus_idx, zoom_idx, img_idx;

    std::string console_input;
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

    // motor variables
    std::vector<uint32_t> focus_range, zoom_range;
    motor_info focus_motor;
    motor_info zoom_motor;
    int32_t focus_step = 0;
    int32_t zoom_step = 0;
    bool mtr_moving = false;

    // trigger variables
    std::vector<uint8_t> tc_ch1(5);
    std::vector<uint8_t> tc_ch2(5);
    trigger_info t1_info;
    trigger_info t2_info;

    // camera variables
    uint32_t cam_index;
    uint32_t num_cams;
    uint64_t width, height, x_offset, y_offset;
    uint32_t x_padding, y_padding;
    uint32_t ts = 0;
    //uint32_t sharpness;
    double camera_temp;
    std::vector<double> exp_time;
    uint32_t cap_num;
    std::vector<std::string> cam_sn;
    Spinnaker::CameraPtr cam;
    Spinnaker::PixelFormatEnums pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_BGR8;
    double camera_gain;
    Spinnaker::GainAutoEnums gain_mode = Spinnaker::GainAutoEnums::GainAuto_Once;
    Spinnaker::ExposureAutoEnums exp_mode = Spinnaker::ExposureAutoEnums::ExposureAuto_Once;
    double frame_rate;
    Spinnaker::AdcBitDepthEnums bit_depth = Spinnaker::AdcBitDepthEnums::AdcBitDepth_Bit12;
    //Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_Continuous;
    //Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_MultiFrame;
    Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_SingleFrame;
    Spinnaker::TriggerSourceEnums trigger_source;
    Spinnaker::TriggerActivationEnums trigger_activation = Spinnaker::TriggerActivation_RisingEdge;
    Spinnaker::ImagePtr image;
    Spinnaker::SystemPtr system = Spinnaker::System::GetInstance();
    Spinnaker::CameraList cam_list;
    bool cam_connected = false;

    // OpenCV Variables
    char key;
    cv::Mat cv_image;
    cv::Size img_size;
    std::string image_window = "Image";
    std::vector<int> compression_params;
    compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(4);

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
        //"{sharpness  | 3072 | Sharpness setting for the camera }"
        //"{fps        | 10.0 | Frames per second setting for the camera }"
        "{exp_time   | 15000:-2000:1000 | Exposure time (us) range settings for the camera }"
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

    // if the input is a config file use this over all other input parameters
    if (parser.has("cfg_file"))
    {
        // input config file should contain all required inputs
        std::string cfg_filename = parser.get<std::string>("cfg_file");
        std::vector<std::vector<std::string>> cfg_params;
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
            parse_input_range(cfg_params[0][0], focus_range);

            // line 2: zoom motor step values
            zoom_str = cfg_params[1][0];
            parse_input_range(cfg_params[1][0], zoom_range);

            // line 3: image size/location configuration
            x_offset = std::stoi(cfg_params[2][0]);
            y_offset = std::stoi(cfg_params[2][1]);
            width = std::stoi(cfg_params[2][2]);
            height = std::stoi(cfg_params[2][3]);

            // line 4: camera properties settings
            //sharpness = std::stoi(cfg_params[2][0]);
            //frame_rate = std::stod(cfg_params[2][1]);
            exposure_str = cfg_params[3][0];
            parse_input_range(cfg_params[3][0], exp_time);
            camera_gain = std::stod(cfg_params[3][1]);

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
                    tc_ch1[0] = std::stoi(cfg_params[5][1]) & 0x01;

                    // get the trigger offset
                    uint32_t offset = min((uint32_t)(std::stoi(cfg_params[5][2]) & 0x0000FFFF), max_offset);
                    tc_ch1[1] = (offset >> 24) & 0xFF;
                    tc_ch1[2] = (offset >> 16) & 0xFF;
                    tc_ch1[3] = (offset >> 8) & 0xFF;
                    tc_ch1[4] = offset & 0xFF;

                    // get the trigger length
                    uint32_t length = min((uint32_t)(std::stoi(cfg_params[5][3]) & 0x0000FFFF), max_length);// +offset;
                    tc_ch1[5] = (length >> 24) & 0xFF;
                    tc_ch1[6] = (length >> 16) & 0xFF;
                    tc_ch1[7] = (length >> 8) & 0xFF;
                    tc_ch1[8] = length & 0xFF;

                    // get the trigger line polarity
                    tc_ch2[0] = std::stoi(cfg_params[5][4]) & 0x01;

                    // get the trigger offset
                    uint32_t offset = min((uint32_t)(std::stoi(cfg_params[5][5]) & 0x0000FFFF), max_offset);
                    tc_ch2[1] = (offset >> 24) & 0xFF;
                    tc_ch2[2] = (offset >> 16) & 0xFF;
                    tc_ch2[3] = (offset >> 8) & 0xFF;
                    tc_ch2[4] = offset & 0xFF;

                    // get the trigger length
                    uint32_t length = min((uint32_t)(std::stoi(cfg_params[5][6]) & 0x0000FFFF), max_length);// +offset;
                    tc_ch2[5] = (length >> 24) & 0xFF;
                    tc_ch2[6] = (length >> 16) & 0xFF;
                    tc_ch2[7] = (length >> 8) & 0xFF;
                    tc_ch2[8] = length & 0xFF;
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
        parse_input_range(parser.get<string>("focus_step"), focus_range);

        // line 2: zoom motor step values
        zoom_str = parser.get<string>("zoom_step");
        parse_input_range(parser.get<string>("zoom_step"), zoom_range);

        // line 3: image size/location configuration
        x_offset = parser.get<uint32_t>("x_off");		// 40
        y_offset = parser.get<uint32_t>("y_off");		// 228;
        width = parser.get<uint32_t>("width");		    // 1200;
        height = parser.get<uint32_t>("height");		// 720;

        // line 4: camera properties settings
        //sharpness = parser.get<uint32_t>("sharpness");
        //frame_rate = parser.get<double>("fps");
        exposure_str = parser.get<string>("exp_time");
        parse_input_range(parser.get<string>("exp_time"), exp_time);
        camera_gain = parser.get<double>("gain");

        // line 5: number of images to capture for each focus/zoom/exposure settinge
        cap_num = parser.get<uint32_t>("cap_num");

        // line 6: trigger source
        ts = parser.get<uint32_t>("source");
        if (ts == 0)
            trigger_source = Spinnaker::TriggerSourceEnums::TriggerSource_Line0;
        else
            trigger_source = Spinnaker::TriggerSourceEnums::TriggerSource_Software;

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
    data_log_stream << "#------------------------------------------------------------------" << std::endl;
    data_log_stream << "Version: 1.0    Date: " << sdate << "    Time: " << stime << std::endl << std::endl;
    data_log_stream << "#------------------------------------------------------------------" << std::endl;

    // print out the info about what was passed in to the code
    std::cout << "------------------------------------------------------------------" << std::endl;
    std::cout << "Input Parameters:" << std::endl;
    std::cout << "Image Size (h x w):  " << height << " x " << width << std::endl;
    std::cout << "Image Offset (x, y): " << x_offset << ", " << y_offset << std::endl;
    std::cout << "Focus Step Range:    " << focus_str << std::endl;
    std::cout << "Zoom Step Range:     " << zoom_str << std::endl;
    std::cout << "Exposure Time Range: " << exposure_str << std::endl;
    std::cout << "Camera Gain:         " << camera_gain << std::endl;
    std::cout << "Number of Captures:  " << cap_num << std::endl;
    std::cout << "Trigger Source:      " << ts << std::endl;
    std::cout <<  std::endl;

    data_log_stream << "Input Parameters:" << std::endl;
    data_log_stream << "Image Size (h x w):  " << height << " x " << width << std::endl;
    data_log_stream << "Image Offset (x, y): " << x_offset << ", " << y_offset << std::endl;
    data_log_stream << "Focus Step Range:    " << focus_str << std::endl;
    data_log_stream << "Zoom Step Range:     " << zoom_str << std::endl;
    data_log_stream << "Exposure Time Range: " << exposure_str << std::endl;
    data_log_stream << "Camera Gain:         " << camera_gain << std::endl;
    data_log_stream << "Number of Captures:  " << cap_num << std::endl;
    data_log_stream << "Trigger Source:      " << ts << std::endl;
    data_log_stream << std::endl << "#------------------------------------------------------------------" << std::endl;


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

        std::cout << "Select Motor Controller: ";
        std::getline(std::cin, console_input);
        controller_device_num = stoi(console_input);

        if ((controller_device_num < 0) || (controller_device_num > ftdi_devices.size() - 1))
        {
            std::cout << "The device number specified is beyond the range of available FTDI devices!  Exiting..." << std::endl;
            data_log_stream << "The device number specified is beyond the range of available FTDI devices!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        std::cout << std::endl << "Connecting to Motor Controller..." << std::endl;
        ftdi_devices[controller_device_num].baud_rate = 250000;
        while ((ctrl_handle == NULL) && (connect_count < 10))
        {
            ctrl_handle = open_com_port(ftdi_devices[controller_device_num], read_timeout, write_timeout);
            ++connect_count;
        }

        if (ctrl_handle == NULL)
        {
            std::cout << "No Motor Controller found... Press Enter to Exit!" << std::endl;
            data_log_stream << "No Motor Controller found... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        ctrl.tx = data_packet(DRIVER_CONNECT);

        // send connection request packet and get response back
        ctrl.send_packet(ctrl_handle, ctrl.tx);
        status = ctrl.receive_packet(ctrl_handle, 6, ctrl.rx);

        if (status == false)
        {
            std::cout << "No Motor Controller found... Press Enter to Exit!" << std::endl;
            data_log_stream << "No Motor Controller found... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        // get the controller information and display
        ctrl.set_driver_info(ctrl.rx);
        std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << ctrl;
        std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;

        data_log_stream << "-----------------------------------------------------------------------------" << std::endl;
        data_log_stream << ctrl;
        data_log_stream << "-----------------------------------------------------------------------------" << std::endl << std::endl;

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

        // get the current trigger configurations and display the information
        status = ctrl.get_trigger_info(ctrl_handle, t1_info, t2_info);

        std::cout << "-----------------------------------------------------------------------------" << std::endl;
        std::cout << "Trigger Information: " << std::endl;
        std::cout << t1_info << std::endl;
        std::cout << t2_info;
        std::cout << "-----------------------------------------------------------------------------" << std::endl << std::endl;

        data_log_stream << "#----------------------------------------------------------------------------" << std::endl;
        data_log_stream << "Trigger Information: " << std::endl;
        data_log_stream << t1_info << std::endl;
        data_log_stream << t2_info;
        data_log_stream << "#----------------------------------------------------------------------------" << std::endl << std::endl;

        // start off with the motors disabled
        status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);
        status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);

        ctrl_connected = true;

        // configure the trigger according to the inputs
        // channel 1
        status = ctrl.config_channel(ctrl_handle, CONFIG_T1, tc_ch1);

        // channel 2
        status = ctrl.config_channel(ctrl_handle, CONFIG_T2, tc_ch2);

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
        set_image_size(cam, height, width, y_offset, x_offset);
        set_pixel_format(cam, pixel_format);
        set_gain_mode(cam, gain_mode);
        set_exposure_mode(cam, exp_mode);
        //set_exposure_time(cam, exp_time[0]);
        set_acquisition_mode(cam, acq_mode); //acq_mode

        // print out the camera configuration
        std::cout << "------------------------------------------------------------------" << std::endl;
        std::cout << "Camera Configuration:" << std::endl;
        data_log_stream << "#------------------------------------------------------------------" << std::endl;
        data_log_stream << "Camera Configuration:" << std::endl;

        get_image_size(cam, height, width, y_offset, x_offset);

        // pixel format
        get_pixel_format(cam, pixel_format);
        get_gain_value(cam, camera_gain);

        // exposure
        double tmp_exp_time;
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
        data_log_stream << std::endl << "#------------------------------------------------------------------" << std::endl;

        std::cout << "------------------------------------------------------------------" << std::endl;
        std::cout << "Save location: " << output_save_location << std::endl << std::endl;
        //data_log_stream << "Save location: " << output_save_location << std::endl;

        std::cout << "------------------------------------------------------------------" << std::endl;
        std::cout << "Beginning Acquisition:" << std::endl;
        std::cout << std::endl << "Press the following keys to perform actions:" << std::endl;
        std::cout << "  f - Step the focus motor by 160 steps" << std::endl;
        std::cout << "  g - Step the focus motor by -160 steps" << std::endl;
        std::cout << "  z - Step the zoom motor by 160 steps" << std::endl;
        std::cout << "  x - Step the zoom motor by -160 steps" << std::endl;

        std::cout << "  s - Save an image" << std::endl;
        std::cout << "  q - Quit" << std::endl;
        std::cout << std::endl;

        // start the acquistion if the mode is set to continuous
        if(acq_mode == Spinnaker::AcquisitionModeEnums::AcquisitionMode_Continuous)
            cam->BeginAcquisition();

        image_window = image_window + "_" + cam_sn[cam_index];

        // set trigger mode and enable
        set_trigger_source(cam, trigger_source, trigger_activation);
        //config_trigger(cam, OFF);
        config_trigger(cam, ON);
        sleep_ms(1000); // blackfy camera needs a 1 second delay after setting the trigger mode to ON

        // grab an initial image to get the padding 
        //acquire_image(cam, image);
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

        do
        {

            //acquire_image(cam, image);
            //acquire_single_image(cam, image);
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
            cv_image = cv::Mat(height + y_padding, width + x_padding, CV_8UC3, image->GetData(), image->GetStride());

            cv::namedWindow(image_window, cv::WindowFlags::WINDOW_NORMAL);
            cv::imshow(image_window, cv_image);
            //cv::resizeWindow(image_window, height / 2, width / 2);
            key = cv::waitKey(1);


            switch(key)
            {
            // check to save the image
            case 's':
                get_current_time(sdate, stime);
                img_save_folder = output_save_location + sdate + "_" + stime + "/";

                stat = mkdir(img_save_folder);
                if (stat != 0 && stat != (int32_t)ERROR_ALREADY_EXISTS)
                {
                    std::cout << "Error creating directory: " << stat << std::endl;
                }

                data_log_stream << "Save location: " << img_save_folder << std::endl;
                std::cout << "------------------------------------------------------------------" << std::endl;

                // enable the motors
                status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, true);
                status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, true);

                sleep_ms(10);
                
                // set the focus and zoom steps to zero
/*
                focus_step = 0;
                md.tx = data_packet(ABS_FOCUS_CTRL, focus_step);
                md.send_packet(md_handle, md.tx);
                status = md.receive_packet(md_handle, 6, md.rx);

                zoom_step = 0;
                md.tx = data_packet(ABS_ZOOM_CTRL, zoom_step);
                md.send_packet(md_handle, md.tx);
                status = md.receive_packet(md_handle, 6, md.rx);
*/
                //steps = (md.rx.data[0] << 24) | (md.rx.data[1] << 16) | (md.rx.data[2] << 8) | (md.rx.data[3]);
                //std::cout << "steps: " << steps << std::endl;               
                for (zoom_idx = 0; zoom_idx < zoom_range.size(); ++zoom_idx)
                {
                    // set the focus motor value to each value in focus_range
                    md.tx = data_packet(ABS_ZOOM_CTRL, zoom_range[zoom_idx]);
                    md.send_packet(md_handle, md.tx);
                    status = md.receive_packet(md_handle, 6, md.rx);

                    zoom_str = num2str(zoom_range[zoom_idx], "z%05d_");

                    for (focus_idx = 0; focus_idx < focus_range.size(); ++focus_idx)
                    {
                        // set the focus motor value to each value in focus_range
                        md.tx = data_packet(ABS_FOCUS_CTRL, focus_range[focus_idx]);
                        md.send_packet(md_handle, md.tx);
                        status = md.receive_packet(md_handle, 6, md.rx);

                        focus_str = num2str(focus_range[focus_idx], "f%05d_");
                        sleep_ms(5);

                        // create the directory to save the images at various focus steps
                        /*
                        std::string combined_save_location = "";
                        std::string sub_dir = "focus_" + num2str(focus_range[focus_idx], "%05d");
                        int32_t stat = make_dir(output_save_location, sub_dir);
                        if (stat != 0 && stat != (int32_t)ERROR_ALREADY_EXISTS)
                        {
                            std::cout << "Error creating directory: " << stat << std::endl;
                            data_log_stream << "Error creating directory: " << stat << std::endl;
                            combined_save_location = output_save_location;
                        }
                        else
                        {
                            combined_save_location = output_save_location + sub_dir + "/";
                        }
                        */
                        for (exp_idx = 0; exp_idx < exp_time.size(); ++exp_idx)
                        {
                            set_exposure_time(cam, exp_time[exp_idx]);

                            for (img_idx = 0; img_idx < cap_num; ++img_idx)
                            {

                                // grab an image: either from the continuous, single or triggered
                                //acquire_image(cam, image);
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

                                get_exposure_time(cam, tmp_exp_time);
                                exposure_str = num2str(tmp_exp_time, "e%05.0f_");

                                image_capture_name = image_header + zoom_str + focus_str + exposure_str + num2str(img_idx, "i%02d") + ".png";

                                cv_image = cv::Mat(height + y_padding, width + x_padding, CV_8UC3, image->GetData(), image->GetStride());

                                cv::namedWindow(image_window, cv::WindowFlags::WINDOW_NORMAL);
                                cv::imshow(image_window, cv_image);
                                key = cv::waitKey(1);

                                // save the image
                                std::cout << "saving: " << img_save_folder << image_capture_name << std::endl;
                                data_log_stream << image_capture_name << std::endl;

                                cv::imwrite((img_save_folder + image_capture_name), cv_image, compression_params);
                                //std::cout << image_capture_name << "," << num2str(tmp_exp_time, "%2.2f") << std::endl;
                                //sleep_ms(100);

                            }   // end of img_idx loop

                        }   // end of exp_idx loop

                        set_exposure_time(cam, exp_time[0]);

                    }   // end of focus_idx loop

                }   // end of zoom_idx loop

                // set the focus step to the first focus_range setting
                //focus_step = 0;
                md.tx = data_packet(ABS_FOCUS_CTRL, focus_range[0]);
                md.send_packet(ctrl_handle, md.tx);
                status = md.receive_packet(ctrl_handle, 6, md.rx);

                // set the focus step to zero
                //focus_step = 0;
                md.tx = data_packet(ABS_ZOOM_CTRL, zoom_range[0]);
                md.send_packet(ctrl_handle, md.tx);
                status = md.receive_packet(ctrl_handle, 6, md.rx);

                // disable the motors
                status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);
                status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);

                std::cout << "------------------------------------------------------------------" << std::endl;
                data_log_stream << "#------------------------------------------------------------------" << std::endl;
                break;
                // end of key == 's'

            case 'f':
                focus_step = 160;
                //md.step_focus_motor(ctrl_handle, focus_step, CMD_FOCUS_CTRL);
                std::cout << "focus step: " << focus_step << "   \r";
                std::cout.flush();
                break;

            case 'g':
                focus_step = -160;
                //md.step_focus_motor(ctrl_handle, focus_step, CMD_FOCUS_CTRL);
                std::cout << "focus step: " << focus_step << "   \r";
                std::cout.flush();
                break;

            case 'z':
                zoom_step = 160;
                //md.step_zoom_motor(ctrl_handle, zoom_step, CMD_ZOOM_CTRL);
                std::cout << "zoom step:  " << zoom_step << "   \r";
                std::cout.flush();
                break;

            case 'x':
                zoom_step = -160;
                //md.step_zoom_motor(ctrl_handle, zoom_step, CMD_ZOOM_CTRL);
                std::cout << "zoom step:  " << zoom_step << "   \r";
                std::cout.flush();
                break;

            default:
                break;
            }   // end of switch case

        } while (key != 'q');

        if (acq_mode == Spinnaker::AcquisitionModeEnums::AcquisitionMode_Continuous)
            cam->EndAcquisition();

        // deactivate the camera trigger
        config_trigger(cam, OFF);

    }
    catch (Spinnaker::Exception &e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        data_log_stream << "Error: " << e.what() << std::endl;
    }
    catch (std::exception e)
    {
        std::cout << "Error: " << e.what() << std::endl;
        data_log_stream << "Error: " << e.what() << std::endl;
    }

    std::cout << std::endl;
    
    // close the motor driver port first
    std::cout << "Closing the Controller port..." << std::endl;
    close_com_port(ctrl_handle);
    ctrl_connected = false;

    // close out the camera
    std::cout << "Closing Camera..." << std::endl;

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
    
    std::cout << std::endl << "Program Complete! Press Enter to close..." << std::endl;
    std::cin.ignore();

    return 0;

}   // end of main
