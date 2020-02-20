//=============================================================================
#define _CRT_SECURE_NO_WARNINGS

// FTDI Driver Includes
#include "ftdi_functions.h"

#if defined(_WIN32) | defined(__WIN32__) | defined(__WIN32) | defined(_WIN64) | defined(__WIN64)

#else defined(__linux__)

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

    uint32_t idx, jdx, kdx;
    uint8_t status;
    std::string console_input;

    motor_driver md;

    // Motor Driver Variables
    uint32_t ftdi_device_count = 0;
    ftdiDeviceDetails driver_details;
    FT_HANDLE driver_handle = NULL;
    uint32_t driver_device_num = 0;
    uint32_t connect_count = 0;
    std::vector<ftdiDeviceDetails> ftdi_devices;

    uint32_t focus_step, zoom_step;

    // camera variables
    uint32_t cam_index;
    uint32_t num_cams;
    uint64_t width, height, x_offset, y_offset;
    std::vector<std::string> cam_sn;
    Spinnaker::CameraPtr cam;
    Spinnaker::PixelFormatEnums pixel_format;
    double camera_gain;
    Spinnaker::GainAutoEnums gain_mode;
    double exp_time;
    Spinnaker::ExposureAutoEnums  exp_mode;
    double frame_rate, frame_count;
    Spinnaker::AcquisitionModeEnums acq_mode;
    Spinnaker::ImagePtr image;
    Spinnaker::SystemPtr system = Spinnaker::System::GetInstance();

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
        "{help h ?  |  | Display Usage message }"
        "{cfg_file  |  | Alternate input method to supply all parameters, all parameters must be included in the file }"
        "{v_step    | 127:1:145 | voltage step range}"
        "{x_off     | 8 | X offset for camera }"
        "{y_off     | 4 | Y offset for camera }"
        "{width     | 1264 | Width of the captured image }"
        "{height    | 1020 | Height of the captured image }"
        "{sharpness | 3072 | Sharpness setting for the camera }"
        "{fps       | 10.0 | Frames per second setting for the camera }"
        "{shutter   | 60:-10:10 | Shutter speed range settings for the camera }"
        "{gain      | 5.0 | Inital gain setting before letting the camera find one }"
        "{avg       | 11 | Number of images to capture for an average }"
        "{output    | ../results/       | Output directory to save lidar images }"
        ;

    // use opencv's command line parser
    cv::CommandLineParser parser(argc, argv, params);

    if (parser.has("help"))
    {
        parser.printMessage();
        return 0;
    }



    // Print out current library version
    std::cout << system->GetLibraryVersion() << std::endl;

    Spinnaker::CameraList cam_list = system->GetCameras();

    try {

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
            driver_handle = open_com_port(ftdi_devices[driver_device_num]);
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


        num_cams = get_camera_selection(cam_list, cam_index, cam_sn);

        // Finish if there are no cameras
        if (num_cams == 0)
        {
            // Clear camera list before releasing system
            cam_list.Clear();

            // Release system
            system->ReleaseInstance();

            std::cout << "No Cameras Detected! Press Enter to exit..." << std::endl;
            std::cin.ignore();

            return -1;
        }

        // get the selected camera
        cam = cam_list.GetByIndex(cam_index);

        // print out some information about the camera
        std::cout << cam << std::endl;


        // initialize the camera
        cam->Init();

        // config the image size
        get_image_size(cam, height, width, y_offset, x_offset);
        std::cout << "Image Size (h x w): " << height << " x " << width << ", [" << x_offset << ", " << y_offset << "]" << std::endl;
        width = 1024;
        height = 1024;
        x_offset = 1024;
        y_offset = 1024;
        set_image_size(cam, height, width, y_offset, x_offset);
        get_image_size(cam, height, width, y_offset, x_offset);
        std::cout << "Image Size (h x w): " << height << " x " << width << ", [" << x_offset << ", " << y_offset << "]" << std::endl;

        // pixel format
        get_pixel_format(cam, pixel_format);
        std::cout << "Pixel Format " << pixel_format << std::endl;
        pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_RGB8;
        set_pixel_format(cam, pixel_format);
        get_pixel_format(cam, pixel_format);
        std::cout << "Pixel Format " << pixel_format << std::endl;

        // gain operations
        get_gain(cam, camera_gain, gain_mode);
        std::cout << "Gain mode/vale " << gain_mode << "/" << camera_gain << std::endl;
        gain_mode = Spinnaker::GainAutoEnums::GainAuto_Off;
        camera_gain = 10.23;
        set_gain(cam, camera_gain, gain_mode);
        get_gain(cam, camera_gain, gain_mode);
        std::cout << "Gain mode/vale " << gain_mode << "/" << camera_gain << std::endl;

        // exposure
        get_exposure(cam, exp_time, exp_mode);
        std::cout << "Exposure mode/vale " << exp_mode << "/" << exp_time << std::endl;
        exp_mode = Spinnaker::ExposureAutoEnums::ExposureAuto_Off;
        exp_time = 10200.2;
        set_exposure(cam, exp_time, exp_mode);
        get_exposure(cam, exp_time, exp_mode);
        std::cout << "Exposure mode/vale " << exp_mode << "/" << exp_time << std::endl;
        
        /*
        exp_time = cam->ExposureTime.GetValue();
        exp_mode = cam->ExposureAuto.GetValue();

        cam->ExposureAuto.SetValue(Spinnaker::ExposureAutoEnums::ExposureAuto_Off);

        cam->ExposureTime.SetValue(20000);

        exp_time = cam->ExposureTime.GetValue();
        */


        /*
        // acquisition
        get_acquisition(cam, frame_rate, acq_mode);
        std::cout << "Acq mode/vale " << acq_mode << "/" << frame_rate << std::endl;
        acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_SingleFrame;
        set_acquisition(cam, frame_rate, acq_mode);
        get_acquisition(cam, frame_rate, acq_mode);
        std::cout << "Acq mode/vale " << acq_mode << "/" << frame_rate << std::endl;
        */

        // get an image
        cam->BeginAcquisition();

        acquire_image(cam, image);
        unsigned int XPadding = image->GetXPadding();
        unsigned int YPadding = image->GetYPadding();
        //image data contains padding. When allocating Mat container size, you need to account for the X,Y image data padding. 
        cv::Mat cvimg = cv::Mat(height + YPadding, width + XPadding, CV_8UC3, image->GetData(), image->GetStride());
        cv::namedWindow("current Image", cv::WINDOW_AUTOSIZE);
        cv::imshow("current Image", cvimg);
        //resizeWindow("current Image", rowsize / 2, colsize / 2);
        cv::waitKey(1);//otherwise the image will not display..

        Spinnaker::ImagePtr image2;
        acquire_image(cam, image);
        cv::Mat cvimg2 = cv::Mat(height + YPadding, width + XPadding, CV_8UC3, image2->GetData(), image2->GetStride());



        cam->EndAcquisition();




    }
    catch (Spinnaker::Exception &e)
    {
        std::cout << "Error: " << e.what() << std::endl;


    }
    catch (std::exception e)
    {
        std::cout << "Error: " << e.what() << std::endl;
    }



    // de-initialize the camera
    cam->DeInit();

    // Release reference to the camera
    cam = nullptr;

    // Clear camera list before releasing system
    cam_list.Clear();

    // Release system
    system->ReleaseInstance();


    std::cout << "Program Complete! Press Enter to close..." << std::endl;
    std::cin.ignore();

    return 0;

}   // end of main
