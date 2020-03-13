//=============================================================================
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
#include "make_dir.h"

// Project Includes
#include "motor_driver.h"


int main(int argc, char** argv)
{

    uint32_t idx, jdx;
    //uint8_t status;
    std::string console_input;
    std::ofstream data_log_stream;

    motor_driver md;

    // Motor Driver Variables
    uint32_t ftdi_device_count = 0;
    ftdiDeviceDetails driver_details;
    FT_HANDLE driver_handle = NULL;
    uint32_t driver_device_num = 0;
    uint32_t connect_count = 0;
    uint64_t read_timeout = 15000;
    uint64_t write_timeout = 1000;
    std::vector<ftdiDeviceDetails> ftdi_devices;
    std::vector<uint32_t> focus_range, zoom_range;

    //uint32_t focus_step, zoom_step;

    // camera variables
    uint32_t cam_index;
    uint32_t num_cams;
    uint32_t width, height, x_offset, y_offset;
    uint32_t x_padding, y_padding;
    uint32_t sharpness;
    double camera_temp;
    std::vector<double> exp_time;
    uint32_t avg_count;
    std::vector<std::string> cam_sn;
    Spinnaker::CameraPtr cam;
    Spinnaker::PixelFormatEnums pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_RGB8;
    double camera_gain;
    Spinnaker::GainAutoEnums gain_mode = Spinnaker::GainAutoEnums::GainAuto_Once;
    Spinnaker::ExposureAutoEnums exp_mode = Spinnaker::ExposureAutoEnums::ExposureAuto_Once;
    double frame_rate, frame_count;
    Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_Continuous;
    //Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_MultiFrame;
    //Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_SingleFrame;
    Spinnaker::TriggerSourceEnums trigger_source;
    Spinnaker::ImagePtr image;
    Spinnaker::SystemPtr system = Spinnaker::System::GetInstance();
    Spinnaker::CameraList cam_list;
    std::string exposure_str;

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
    std::string image_capture_name = "image_";
    std::string output_save_location = "";

    const std::string params =
        "{help h ?   |  | Display Usage message }"
        "{cfg_file   |  | Alternate input method to supply all parameters, all parameters must be included in the file }"
        "{focus_step | 0:216:21384 | voltage step range}"
        "{x_off      | 8 | X offset for camera }"
        "{y_off      | 4 | Y offset for camera }"
        "{width      | 1264 | Width of the captured image }"
        "{height     | 1020 | Height of the captured image }"
        "{sharpness  | 3072 | Sharpness setting for the camera }"
        "{fps        | 10.0 | Frames per second setting for the camera }"
        "{exp_time   | 15000:-2000:1000 | Exposure time (us) range settings for the camera }"
        "{gain       | 5.0 | Inital gain setting before letting the camera find one }"
        "{avg        | 11 | Number of images to capture for an average }"
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
        // Line 1: colon separated values of the motor driver step range(start : inc : stop)
        // Line 2: comma separated values for the x offset, y offset, width, height of the camera
        // Line 3: comma separated values for the camera properties : sharpness, fps, shutter range, gain
        // Line 4: single value for the number of images to capture to average
        // Line 5: trigger source (0 -> Line0, 1 -> Software)
        // Line 6: base directory where the results will be saved
        if (cfg_params.size() == 6)
        {
            // line 1: setup the focus motor steps in a vector
            parse_input_range(cfg_params[0][0], focus_range);

            // line 2: image properties
            x_offset = std::stoi(cfg_params[1][0]);
            y_offset = std::stoi(cfg_params[1][1]);
            width = std::stoi(cfg_params[1][2]);
            height = std::stoi(cfg_params[1][3]);

            // line 3: camera properties settings
            sharpness = std::stoi(cfg_params[2][0]);
            frame_rate = std::stod(cfg_params[2][1]);
            parse_input_range(cfg_params[2][2], exp_time);
            camera_gain = std::stod(cfg_params[2][3]);

            // line 4: average frame capture
            avg_count = std::stoi(cfg_params[3][0]);

            // line 5: trigger source
            uint8_t ts = std::stoi(cfg_params[4][0]);
            if (ts == 0)
                trigger_source = Spinnaker::TriggerSourceEnums::TriggerSource_Line0;
            else
                trigger_source = Spinnaker::TriggerSourceEnums::TriggerSource_Software;

            // line 6: output save location
            output_save_location = cfg_params[5][0];
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
        // line 1: setup the focus motor steps in a vector
        parse_input_range(parser.get<string>("focus_step"), focus_range);
        
        // line 2: image properties
        x_offset = parser.get<uint32_t>("x_off");		// 40
        y_offset = parser.get<uint32_t>("y_off");		// 228;
        width = parser.get<uint32_t>("width");		    // 1200;
        height = parser.get<uint32_t>("height");		// 720;

        // line 3: camera properties settings
        sharpness = parser.get<uint32_t>("sharpness");
        frame_rate = parser.get<double>("fps");
        parse_input_range(parser.get<string>("exp_time"), exp_time);
        camera_gain = parser.get<double>("gain");

        // line 4: average frame capture
        avg_count = parser.get<uint32_t>("avg");

        // line 5: trigger source
        uint32_t ts = parser.get<uint32_t>("source");
        if (ts == 0)
            trigger_source = Spinnaker::TriggerSourceEnums::TriggerSource_Line0;
        else
            trigger_source = Spinnaker::TriggerSourceEnums::TriggerSource_Software;

        // line 6: output save location
        output_save_location = parser.get<std::string>("output");
    }

    output_save_location = path_check(output_save_location);

    get_current_time(sdate, stime);
    log_filename = log_filename + sdate + "_" + stime + ".txt";

    std::cout << "Log File: " << (output_save_location + log_filename) << std::endl << std::endl;
    data_log_stream.open((output_save_location + log_filename), ios::out | ios::app);

    // Add the date and time to the start of the log file
    data_log_stream << "#------------------------------------------------------------------" << std::endl;
    data_log_stream << "Version: 1.0    Date: " << sdate << "    Time: " << stime << std::endl << std::endl;
    data_log_stream << "#------------------------------------------------------------------" << std::endl;

    try {
        // Print out current library version
        std::cout << system->GetLibraryVersion() << std::endl;

        cam_list = system->GetCameras();
/*
        ftdi_device_count = get_device_list(ftdi_devices);
        if (ftdi_device_count == 0)
        {
            std::cout << "No ftdi devices found... Exiting!" << std::endl;
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
            data_log_stream << "No Motor Controller found... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        md.tx = data_packet(CONNECT);

        // send connection request packet and get response back
        md.send_packet(driver_handle, md.tx);
        status = md.receive_packet(driver_handle, 6, md.rx);

        if (status == false)
        {
            std::cout << "No Motor Controller found... Exiting!" << std::endl;
            data_log_stream << "No Motor Controller found... Exiting!" << std::endl;
            std::cin.ignore();
            return -1;
        }

        md.set_driver_info(md.rx);
        std::cout << md << std::endl;
        data_log_stream << md << std::endl;
        data_log_stream << "#------------------------------------------------------------------" << std::endl;
*/
        num_cams = get_camera_selection(cam_list, cam_index, cam_sn);

        // Finish if there are no cameras
        if (num_cams == 0)
        {
            // Clear camera list before releasing system
            cam_list.Clear();

            // Release system
            system->ReleaseInstance();

            std::cout << "No Camera found... Exiting!" << std::endl;
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
        //cam->Init();
        init_camera(cam);
        cam->TriggerActivation.SetValue(Spinnaker::TriggerActivationEnums::TriggerActivation_RisingEdge);

        get_temperature(cam, camera_temp);
        std::cout << "  Camera Temp:       " << camera_temp << std::endl << std::endl;
        data_log_stream << "  Camera Temp:       " << camera_temp << std::endl << std::endl;

        // config the image size
        //get_image_size(cam, height, width, y_offset, x_offset);
        //std::cout << "Image Size (h x w): " << height << " x " << width << ", [" << x_offset << ", " << y_offset << "]" << std::endl;

        // configure the camera
        //set_image_size(cam, height, width, y_offset, x_offset);
        set_pixel_format(cam, pixel_format);
        set_gain_mode(cam, gain_mode);
        set_exposure_mode(cam, exp_mode);
        set_exposure_time(cam, exp_time[0]);
        set_acquisition_mode(cam, acq_mode); //acq_mode

        // print out the camera configuration
        std::cout << "------------------------------------------------------------------" << std::endl;
        std::cout << "Camera Configuration:" << std::endl;
        data_log_stream << "------------------------------------------------------------------" << std::endl;
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
       
        std::cout << "Image Size (h x w):  " << height << " x " << width << ", [" << x_offset << ", " << y_offset << "]" << std::endl;
        std::cout << "Pixel Format:        " << pixel_format << std::endl;
        std::cout << "Gain mode/value:     " << gain_mode << "/" << camera_gain << std::endl;
        std::cout << "Exposure mode/value: " << exp_mode << "/" << tmp_exp_time << std::endl;
        std::cout << "Acq mode/value:      " << acq_mode << "/" << frame_rate << std::endl;
        std::cout << "Avg Capture Number:  " << avg_count << std::endl;
        //std::cout << "ADC Bit Depth:       " << cam->AdcBitDepth.GetValue() << std::endl;
        std::cout << std::endl;

        data_log_stream << "Image Size (h x w):  " << height << " x " << width << ", [" << x_offset << ", " << y_offset << "]" << std::endl;
        data_log_stream << "Pixel Format:        " << pixel_format << std::endl;
        data_log_stream << "Gain mode/value:     " << gain_mode << "/" << camera_gain << std::endl;
        data_log_stream << "Exposure mode/value: " << exp_mode << "/" << tmp_exp_time << std::endl;
        data_log_stream << "Acq mode/value:      " << acq_mode << "/" << frame_rate << std::endl;
        data_log_stream << "Avg Capture Number:  " << avg_count << std::endl;
        //data_log_stream << "ADC Bit Depth:       " << cam->AdcBitDepth.GetValue() << std::endl;
        data_log_stream << std::endl;

        std::cout << "------------------------------------------------------------------" << std::endl;
        std::cout << "Root save location: " << output_save_location << std::endl << std::endl;

        std::cout << "------------------------------------------------------------------" << std::endl;
        std::cout << "Beginning Acquisition:" << std::endl;
        std::cout << std::endl << "Press the following keys to perform actions:" << std::endl;
        std::cout << "  s - Save an image" << std::endl;
        std::cout << "  q - Quit" << std::endl;
        std::cout << std::endl;

        // get an image
        if(acq_mode == Spinnaker::AcquisitionModeEnums::AcquisitionMode_Continuous)
            cam->BeginAcquisition();

        image_window = image_window + "_" + cam_sn[cam_index];

        // set trigger mode and enable
        set_trigger_source(cam, trigger_source);
        //config_trigger(cam, true);
        config_trigger(cam, false);

        // grab an initial image to get the padding 
        //fire_software_trigger(cam);
        acquire_image(cam, image);
        //acquire_single_image(cam, image);
        x_padding = image->GetXPadding();
        y_padding = image->GetYPadding();

        do
        {
            //fire_software_trigger(cam);
            acquire_image(cam, image);
            //acquire_single_image(cam, image);

            //image data contains padding. When allocating Mat container size, you need to account for the X,Y image data padding. 
            cv_image = cv::Mat(height + y_padding, width + x_padding, CV_8UC3, image->GetData(), image->GetStride());

            cv::namedWindow(image_window, cv::WindowFlags::WINDOW_NORMAL);
            cv::imshow(image_window, cv_image);
            //cv::resizeWindow(image_window, height / 2, width / 2);
            key = cv::waitKey(1);

            // check to save the image
            if (key == 's')
            {
                get_current_time(sdate, stime);
                //ld.send_lens_packet(focus_packets[0], lens_driver_handle);

                sleep_ms(10);

                for (idx = 0; idx < exp_time.size(); ++idx)
                {
                    std::string combined_save_location = "";
                    std::string sub_dir = "exp_" + num2str(exp_time[idx], "%02.0f");
                    int32_t stat = make_dir(output_save_location, sub_dir);
                    if (stat != 1 && stat != (int32_t)ERROR_ALREADY_EXISTS)
                    {
                        std::cout << "Error creating directory: " << stat << std::endl;
                        data_log_stream << "Error creating directory: " << stat << std::endl;
                        combined_save_location = output_save_location;
                    }
                    else
                    {
                        combined_save_location = output_save_location + sub_dir + "/";
                    }

                    set_exposure_time(cam, exp_time[idx]);
                    get_exposure_time(cam, tmp_exp_time);

                    exposure_str = num2str(tmp_exp_time, "%2.2f");
                    std::cout << "exp_str: " << exposure_str << std::endl;

                    acquire_image(cam, image);
                    //acquire_single_image(cam, image);
                    cv_image = cv::Mat(height + y_padding, width + x_padding, CV_8UC3, image->GetData(), image->GetStride());

                    cv::namedWindow(image_window, cv::WindowFlags::WINDOW_NORMAL);
                    cv::imshow(image_window, cv_image);
                    key = cv::waitKey(1);
                    //sleep_ms(100);

                }

                set_exposure_time(cam, exp_time[0]);

            }   // end of key == 's'

        } while (key != 'q');

        if (acq_mode == Spinnaker::AcquisitionModeEnums::AcquisitionMode_Continuous)
            cam->EndAcquisition();

        // deactivate the camera trigger
        cam->TriggerMode.SetValue(Spinnaker::TriggerModeEnums::TriggerMode_Off);

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

    // de-initialize the camera
    cam->DeInit();

    // Release reference to the camera
    cam = nullptr;

    // Clear camera list before releasing system
    cam_list.Clear();

    // Release system
    system->ReleaseInstance();

    cv::destroyAllWindows();

    data_log_stream.close();
    close_com_port(driver_handle);

    std::cout << std::endl << "Program Complete! Press Enter to close..." << std::endl;
    std::cin.ignore();

    return 0;

}   // end of main
