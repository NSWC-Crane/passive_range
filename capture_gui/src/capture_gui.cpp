#include "capture_gui.h"
#include "./ui_capture_gui.h"


//#include <capture_gui.h>
#include <cstdint>
#include <cstdlib>
#include <iostream>
#include <sstream>

// OpenCV Includes
#include <opencv2/core.hpp>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"

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

std::vector<ftdiDeviceDetails> ftdi_devices;
controller ctrl;
std::ofstream data_log_stream;

// motor variables
motor_info focus_motor;
motor_info zoom_motor;

// trigger variables
trigger_info t1_info;
trigger_info t2_info;

// camera variables
Spinnaker::CameraPtr cam;
Spinnaker::CameraList cam_list;
Spinnaker::PixelFormatEnums pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_BGR8;
Spinnaker::GainAutoEnums gain_mode = Spinnaker::GainAutoEnums::GainAuto_Once;
//Spinnaker::ExposureAutoEnums exp_mode = Spinnaker::ExposureAutoEnums::ExposureAuto_Once;
Spinnaker::ExposureAutoEnums exp_mode = Spinnaker::ExposureAutoEnums::ExposureAuto_Off;
Spinnaker::AdcBitDepthEnums bit_depth = Spinnaker::AdcBitDepthEnums::AdcBitDepth_Bit12;
//Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_Continuous;
//Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_MultiFrame;
Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_SingleFrame;
Spinnaker::TriggerSourceEnums trigger_source;
Spinnaker::TriggerActivationEnums trigger_activation = Spinnaker::TriggerActivation_RisingEdge;

// ----------------------------------------------------------------------------
capture_gui::capture_gui(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::capture_gui)
{
    ui->setupUi(this);

    uint32_t idx;
    uint32_t ftdi_device_count = 0;

    Spinnaker::SystemPtr system = Spinnaker::System::GetInstance();

    // camera variables
    uint32_t cam_index;
    uint32_t num_cams;
    std::vector<std::string> cam_sn;


//    ui->statusbar->hide();

    // connect the focus slider and focus spinner
    // connect(ui->focus_slider, SIGNAL(valueChanged(int)), this, SLOT(on_focus_slider_change()));
    // connect(ui->focus_spinner, SIGNAL(valueChanged(int)), this, SLOT(on_focus_spinner_change()));

    // connect(ui->zoom_slider, SIGNAL(valueChanged(int)), this, SLOT(on_zoom_slider_change()));
    // connect(ui->zoom_spinner, SIGNAL(valueChanged(int)), this, SLOT(on_zoom_spinner_change()));

    // connect(ui->iris_slider, SIGNAL(valueChanged(int)), this, SLOT(on_iris_slider_change()));
    // connect(ui->iris_spinner, SIGNAL(valueChanged(int)), this, SLOT(on_iris_spinner_change()));

    //connect(ui->ftdi_connect_btn, SIGNAL(clicked()), this, SLOT(on_ftdi_connect_btn_clicked()));


    // ----------------------------------------------------------------------------
    // FTDI section for finding attached devices and filling in the combo box
    ui->ftdi_cb->clear();

    ftdi_device_count = get_device_list(ftdi_devices);

    for (idx = 0; idx < ftdi_devices.size(); ++idx)
    {
        //std::cout << ftdi_devices[idx];
        ui->ftdi_cb->addItem(QString::fromStdString(ftdi_devices[idx].description + " " + ftdi_devices[idx].serial_number));
    }

    tc_ch1.resize(9);
    tc_ch2.resize(9);

    focus_range.clear();
    focus_range.push_back(min_focus_steps);
    zoom_range.clear();
    zoom_range.push_back(min_zoom_steps);

    // ----------------------------------------------------------------------------
    cam_list = system->GetCameras();

    //num_cams = get_camera_selection(cam_list, cam_index, cam_sn);

    cam_sn.clear();

    num_cams = cam_list.GetSize();
    std::cout << "Number of cameras detected: " << num_cams << std::endl;

    for (idx = 0; idx < num_cams; ++idx)
    {
        Spinnaker::CameraPtr cam = cam_list.GetByIndex(idx);
        Spinnaker::GenApi::CStringPtr sn = cam->GetTLDeviceNodeMap().GetNode("DeviceSerialNumber");
        Spinnaker::GenApi::CStringPtr model = cam->GetTLDeviceNodeMap().GetNode("DeviceModelName");
        cam_sn.push_back(std::string(sn->ToString()));

        ui->cam_cb->addItem(QString::fromStdString(std::string(model->ToString()) + " [" + cam_sn[idx] + "]"));
        //std::cout << "[" << idx << "] " << model->ToString() << " - Serial Number: " << cam_sn[idx] << std::endl;
    }

    // Finish if there are no cameras
    if ((num_cams == 0) || (cam_index < 0) || (cam_index > num_cams - 1))
    {
        // Clear camera list before releasing system
        cam_list.Clear();

        // Release system
        system->ReleaseInstance();

        ui->console_te->append("No Camera found...");
        data_log_stream << "No Camera found... Exiting!" << std::endl;
        //std::cin.ignore();
        //return -1;
    }

//    ui->px_format->addItem("BGR8");
//    ui->px_format->addItem("Mono8");

    ui->gain->setValidator( new QDoubleValidator(0, 8, 4, this) );
    ui->exposure->setValidator( new QDoubleValidator(0, 8, 4, this) );

    on_toolButton_clicked();
}

capture_gui::~capture_gui()
{
    delete ui;
}

// ----------------------------------------------------------------------------
void capture_gui::on_ftdi_connect_btn_clicked()
{
    bool status = false;
    std::stringstream ss;

    QMessageBox msgBox;
    msgBox.setWindowTitle(" ");

    uint32_t connect_count = 0;
    uint32_t read_timeout = 60000;
    uint32_t write_timeout = 1000;

    uint32_t controller_device_num = ui->ftdi_cb->currentIndex();

//    std::cout << std::endl << "Rotate the focus and the zoom lens to the zero position.  Press Enter when complete...";
    msgBox.setText("Rotate the focus and the zoom lens to the zero position.  Press OK when complete...");
    msgBox.exec();

    ui->console_te->append("Connecting to Controller...");
    ftdi_devices[controller_device_num].baud_rate = 250000;
    while ((ctrl_handle == NULL) && (connect_count < 10))
    {
        ctrl_handle = open_com_port(ftdi_devices[controller_device_num], read_timeout, write_timeout);
        ++connect_count;
    }

    if (ctrl_handle == NULL)
    {
        ui->console_te->append("No Controller found...");
        ui->console_te->show();
        data_log_stream << "No Controller found... Exiting!" << std::endl;
        //std::cin.ignore();
        //return -1;
    }

    ctrl.tx = data_packet(DRIVER_CONNECT);

    // send connection request packet and get response back
    ctrl.send_packet(ctrl_handle, ctrl.tx);
    status = ctrl.receive_packet(ctrl_handle, 6, ctrl.rx);

    if (status == false)
    {
        ui->console_te->append("No Controller found...");
        ui->console_te->show();
        data_log_stream << "No Controller found... Exiting!" << std::endl;
        //std::cin.ignore();
        //return -1;
    }

    // get the controller information and display
    ctrl.set_driver_info(ctrl.rx);
    ui->console_te->append("-----------------------------------------------------------------------------");
    ss << ctrl;
    ui->console_te->append(QString::fromStdString(ss.str()));
    ui->console_te->show();
    //ui->console_te->append("-----------------------------------------------------------------------------");

    data_log_stream << "-----------------------------------------------------------------------------" << std::endl;
    data_log_stream << ctrl;
    data_log_stream << "-----------------------------------------------------------------------------" << std::endl << std::endl;

    //-----------------------------------------------------------------------------
    // ping the motors to get the model number and firmware version
    status = ctrl.ping_motor(ctrl_handle, FOCUS_MOTOR_ID, focus_motor);

    ui->console_te->append("-----------------------------------------------------------------------------");
    ui->console_te->append("Focus Motor Information: ");

    data_log_stream << "-----------------------------------------------------------------------------" << std::endl;
    data_log_stream << "Focus Motor Information: " << std::endl;

    if (status)
    {
        ss << focus_motor;
        ui->console_te->append(QString::fromStdString(ss.str()));
        data_log_stream << focus_motor;
    }
    else
    {
        ui->console_te->append("  Error getting focus motor info");
        data_log_stream << "  Error getting focus motor info" << std::endl;
    }

    // configure the homing offsets for the focus motor
    // set the offset to zero
    status = ctrl.set_offset(ctrl_handle, FOCUS_MOTOR_ID);

    // get the current motor positions
    status = ctrl.get_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);

    ui->console_te->append("  Current Step:     " + QString::number(focus_step));
    data_log_stream << "  Current Step:     " << focus_step << std::endl;


    //-----------------------------------------------------------------------------
    status = ctrl.ping_motor(ctrl_handle, ZOOM_MOTOR_ID, zoom_motor);

    std::cout << std::endl;
    ui->console_te->append("\nZoom Motor Information: ");
    //ui->console_te->show();

    //data_log_stream << "-----------------------------------------------------------------------------" << std::endl;
    data_log_stream << "Zoom Motor Information: " << std::endl;

    if (status)
    {
        ss << zoom_motor;
        ui->console_te->append(QString::fromStdString(ss.str()));
        data_log_stream << zoom_motor;
    }
    else
    {
        ui->console_te->append("  Error getting zoom motor info");
        data_log_stream << "  Error getting zoom motor info" << std::endl;
    }

    // configure the homing offsets for the zoom motor
    // set the offset to zero
    status = ctrl.set_offset(ctrl_handle, ZOOM_MOTOR_ID);

    // get the current motor positions
    status = ctrl.get_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_step);

    ui->console_te->append("  Current Step:     " + QString::number(zoom_step));
    ui->console_te->append("-----------------------------------------------------------------------------\n");
    ui->console_te->show();

    data_log_stream << "  Current Step:     " << zoom_step << std::endl;
    data_log_stream << "-----------------------------------------------------------------------------" << std::endl << std::endl;

    // configure the trigger according to the inputs
    // channel 1
    status = ctrl.config_channel(ctrl_handle, CONFIG_T1, tc_ch1);

    // channel 2
    status = ctrl.config_channel(ctrl_handle, CONFIG_T2, tc_ch2);

    //-----------------------------------------------------------------------------
    // get the current trigger configurations and display the information
    status = ctrl.get_trigger_info(ctrl_handle, t1_info, t2_info);

    ui->console_te->append("-----------------------------------------------------------------------------");
    ui->console_te->append("Trigger Information: ");
    ss << t1_info;
    ui->console_te->append(QString::fromStdString(ss.str()) + "\n");

    ss << t2_info;
    ui->console_te->append(QString::fromStdString(ss.str()));
    ui->console_te->append("-----------------------------------------------------------------------------\n");
    ui->console_te->show();

    data_log_stream << "#----------------------------------------------------------------------------" << std::endl;
    data_log_stream << "Trigger Information: " << std::endl;
    data_log_stream << t1_info << std::endl;
    data_log_stream << t2_info;
    data_log_stream << "#----------------------------------------------------------------------------" << std::endl << std::endl;

    //-----------------------------------------------------------------------------
    // read in the position PID config file and set the PID values for each motor
    status = read_pid_config(pid_config_filename, (uint32_t)(ctrl.ctrl_info.serial_number - 1), pid_values);

    if (status == false)
    {
        ui->console_te->append("The pid_config.txt file does not have enough entries based on supplied serial number.  Using default values.");
        ui->console_te->show();

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

    ui->console_te->append("Setting motors to intial position:");
    ui->console_te->append("focus motor: " + QString::number(focus_range[0]));
    ui->console_te->append("zoom motor: " + QString::number(zoom_range[0]));
    ui->console_te->show();

    status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_range[0]);
    status &= ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_range[0]);

    if (!status)
    {
        ui->console_te->append("Error setting motor positions.");
        ui->console_te->show();
    }

    // disable the motors
    status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);
    status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);

    ctrl_connected = true;


}

void capture_gui::on_cam_connect_btn_clicked()
{
    std::stringstream ss;

    // get the selected camera
    cam = cam_list.GetByIndex(ui->cam_cb->currentIndex());

    // print out some information about the camera
    ui->console_te->append("------------------------------------------------------------------");
    ss << cam;
    ui->console_te->append(QString::fromStdString(ss.str()));
    ui->console_te->show();

    data_log_stream << cam;

    // initialize the camera
    init_camera(cam);
    cam_connected = true;
    //cam->TriggerActivation.SetValue(Spinnaker::TriggerActivationEnums::TriggerActivation_RisingEdge);

    get_temperature(cam, camera_temp);
    ui->console_te->append("  Camera Temp:       " + QString::number(camera_temp) + "\n");
    ui->console_te->show();

    data_log_stream << "  Camera Temp:       " << camera_temp << std::endl << std::endl;

    // config the image size
    //get_image_size(cam, height, width, y_offset, x_offset);
    //std::cout << "Image Size (h x w): " << height << " x " << width << ", [" << x_offset << ", " << y_offset << "]" << std::endl;

    // configure the camera
    set_image_size(cam, height, width, y_offset, x_offset);
    set_pixel_format(cam, pixel_format);
    set_gain_mode(cam, gain_mode);
    set_exposure_mode(cam, exp_mode);
    set_exposure_time(cam, exp_time);
    set_acquisition_mode(cam, acq_mode); //acq_mode

    // if the gain is 0 or greater then them is set to off and a value must be set
    if (camera_gain >= 0)
    {
        set_gain_value(cam, camera_gain);
    }

    // print out the camera configuration
    ui->console_te->append("------------------------------------------------------------------");
    ui->console_te->append("Camera Configuration:");
    ui->console_te->show();

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

    /*
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
*/

//    std::cout << "------------------------------------------------------------------" << std::endl;
//    std::cout << "Beginning Acquisition:" << std::endl;
//    std::cout << std::endl << "Press the following keys to perform actions:" << std::endl;
//    //std::cout << "  f - Step the focus motor by 160 steps" << std::endl;
//    //std::cout << "  g - Step the focus motor by -160 steps" << std::endl;
//    //std::cout << "  z - Step the zoom motor by 160 steps" << std::endl;
//    //std::cout << "  x - Step the zoom motor by -160 steps" << std::endl;

//    std::cout << "  s - Save an image" << std::endl;
//    std::cout << "  q - Quit" << std::endl;
//    std::cout << std::endl;

    // start the acquistion if the mode is set to continuous
    if(acq_mode == Spinnaker::AcquisitionModeEnums::AcquisitionMode_Continuous)
        cam->BeginAcquisition();

    //image_window = image_window + "_" + cam_sn[cam_index];

    // set trigger mode and enable
    set_trigger_source(cam, trigger_source, trigger_activation);
    //config_trigger(cam, OFF);
    config_trigger(cam, ON);
    sleep_ms(1000); // blackfy camera needs a 1 second delay after setting the trigger mode to ON

    // grab an initial image to get the padding
    //acquire_image(cam, image);
//    switch (ts)
//    {
//    case 0:
//        cam->BeginAcquisition();
//        status = ctrl.trigger(ctrl_handle, TRIG_ALL);
//        aquire_trigger_image(cam, image);
//        cam->EndAcquisition();
//        break;
//    case 1:
//        aquire_software_trigger_image(cam, image);
//        break;
//    }

//    x_padding = (uint32_t)image->GetXPadding();
//    y_padding = (uint32_t)image->GetYPadding();

}

void capture_gui::on_z_stop_valueChanged(int arg1)
{

}

void capture_gui::on_px_format_currentIndexChanged(int index)
{
//    switch(ui->px_format->currentIndex())
    switch(index)
    {
    case 0:
        pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_BGR8;
        break;

    case 1:
        pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_Mono12;
        break;
    }
}

void capture_gui::on_toolButton_clicked()
{
    QFileDialog dialog(this);

    dialog.setFileMode(QFileDialog::Directory);

    save_location = QFileDialog::getExistingDirectory(0, ("Select Output Folder"), QDir::currentPath());

    ui->save_location->setText(save_location);
    //ui->save_location->show();


}

void capture_gui::update_zoom_position()
{
    // set the current step to the minimum
    bool status = ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, true);
    status &= ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_range[0]);
    status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);

}

template <typename T>
void capture_gui::generate_range(T start, T stop, T step, std::vector<T>& range)
{
    range.clear();

    T s = start;
    if (step > 0)
    {
        while (s <= stop)
        {
            range.push_back(s);
            s += step;
        }
    }
    else if (step < 0)
    {
        while (s >= stop)
        {
            range.push_back(s);
            s += step;
        }
    }
    else
    {
        range.push_back(start);
    }

}   // end of generate_range

void capture_gui::on_z_start_valueChanged(int arg1)
{
    int32_t start = (int32_t)ui->z_start->value();
    int32_t step = (int32_t)ui->z_step->value();
    int32_t stop = (int32_t)ui->z_stop->value();

    // generate the step ranges
    generate_range(start, stop, step, zoom_range);

    update_zoom_position();

}
