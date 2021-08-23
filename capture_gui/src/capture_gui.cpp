#include "capture_gui.h"
#include "./ui_capture_gui.h"


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
uint32_t cam_index;
std::vector<std::string> cam_sn;
Spinnaker::ImagePtr image;
Spinnaker::CameraPtr cam;
Spinnaker::CameraList cam_list;
Spinnaker::PixelFormatEnums pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_BGR8;
Spinnaker::GainAutoEnums gain_mode = Spinnaker::GainAutoEnums::GainAuto_Off;
//Spinnaker::ExposureAutoEnums exp_mode = Spinnaker::ExposureAutoEnums::ExposureAuto_Once;
Spinnaker::ExposureAutoEnums exp_mode = Spinnaker::ExposureAutoEnums::ExposureAuto_Off;
Spinnaker::AdcBitDepthEnums bit_depth = Spinnaker::AdcBitDepthEnums::AdcBitDepth_Bit12;
//Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_Continuous;
//Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_MultiFrame;
Spinnaker::AcquisitionModeEnums acq_mode = Spinnaker::AcquisitionModeEnums::AcquisitionMode_SingleFrame;
Spinnaker::TriggerSourceEnums trigger_source;
Spinnaker::TriggerActivationEnums trigger_activation = Spinnaker::TriggerActivation_RisingEdge;
Spinnaker::SystemPtr cam_system;

// OpenCV
cv::Mat cv_image;
cv::Size img_size;
std::string image_window = "Image";
std::vector<int> compression_params;


// ----------------------------------------------------------------------------
capture_gui::capture_gui(QWidget *parent)
    : QMainWindow(parent)
    , ui(new Ui::capture_gui)
{
    ui->setupUi(this);

    uint32_t idx;
    uint32_t ftdi_device_count = 0;

    cam_system = Spinnaker::System::GetInstance();

    // camera variables
    uint32_t num_cams;

    // ui->statusbar->hide();

    // connect the focus slider and focus spinner
    // connect(ui->focus_slider, SIGNAL(valueChanged(int)), this, SLOT(on_focus_slider_change()));

    // zoom slots
    // returnPressed()
    QIntValidator zoom_val(min_zoom_steps, max_zoom_steps, this);
    ui->z_start->setValidator(&zoom_val);
    connect(ui->z_start, SIGNAL(editingFinished()), this, SLOT(z_start_edit_complete()));
    ui->z_step->setValidator(&zoom_val);
    connect(ui->z_step, SIGNAL(editingFinished()), this, SLOT(z_step_edit_complete()));
    ui->z_stop->setValidator(&zoom_val);
    connect(ui->z_stop, SIGNAL(editingFinished()), this, SLOT(z_stop_edit_complete()));

    // focus slots
    QIntValidator focus_val(min_focus_steps, max_focus_steps, this);
    ui->f_start->setValidator(&focus_val);
    connect(ui->f_start, SIGNAL(editingFinished()), this, SLOT(f_start_edit_complete()));
    ui->f_step->setValidator(&focus_val);
    connect(ui->f_step, SIGNAL(editingFinished()), this, SLOT(f_step_edit_complete()));
    ui->f_stop->setValidator(&focus_val);
    connect(ui->f_stop, SIGNAL(editingFinished()), this, SLOT(f_stop_edit_complete()));

    // camera slots
    QIntValidator x_off(0, 2048, this);
    ui->x_offset->setValidator(&x_off);
    connect(ui->x_offset, SIGNAL(returnPressed()), this, SLOT(x_offset_edit_complete()));
    ui->width->setValidator(&x_off);
    connect(ui->width, SIGNAL(returnPressed()), this, SLOT(width_edit_complete()));

    QIntValidator y_off(0, 1536, this);
    ui->y_offset->setValidator(&y_off);
    connect(ui->y_offset, SIGNAL(returnPressed()), this, SLOT(y_offset_edit_complete()));
    ui->height->setValidator(&y_off);
    connect(ui->height, SIGNAL(returnPressed()), this, SLOT(height_edit_complete()));

    ui->gain->setValidator( new QDoubleValidator(0, 8, 4, this) );
    connect(ui->gain, SIGNAL(returnPressed()), this, SLOT(gain_edit_complete()));

    ui->exposure->setValidator( new QDoubleValidator(0, 50000, 1, this) );
    connect(ui->exposure, SIGNAL(returnPressed()), this, SLOT(exposure_edit_complete()));

    ui->num_caps->setValidator(new QIntValidator(1,1000, this));

    x_offset = (uint64_t)ui->x_offset->text().toInt();
    y_offset = (uint64_t)ui->y_offset->text().toInt();
    height = (uint64_t)ui->height->text().toInt();
    width = (uint64_t)ui->width->text().toInt();

    camera_gain = ui->gain->text().toDouble();
    exp_time = ui->exposure->text().toDouble();

    switch(ui->px_format->currentIndex())
    {
    case 0:
        pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_BGR8;
        break;
    case 1:
        pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_Mono12;
        break;
    }


//    timer = new QTimer(this);
//    connect(timer, SIGNAL(timeout()), this, SLOT(update_image()));
//    timer->start(20);

//    cv::namedWindow(image_window, cv::WindowFlags::WINDOW_NORMAL);


    compression_params.push_back(cv::IMWRITE_PNG_COMPRESSION);
    compression_params.push_back(4);

    // ----------------------------------------------------------------------------
    // FTDI section for finding attached devices and filling in the combo box
    ui->ftdi_cb->clear();

    ftdi_device_count = get_device_list(ftdi_devices);

    for (idx = 0; idx < ftdi_device_count; ++idx)
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
    cam_list = cam_system->GetCameras();

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
        cam_system->ReleaseInstance();

        ui->console_te->append("No Camera found...");
        ui->console_te->show();
//        data_log_stream << "No Camera found... Exiting!" << std::endl;
        //std::cin.ignore();
        //return -1;
    }

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

    if(ctrl_connected == false)
    {
        uint32_t controller_device_num = ui->ftdi_cb->currentIndex();

        msgBox.setText("Rotate the focus and the zoom lens to the zero position.  Press OK when complete...");
        msgBox.exec();

        ui->console_te->setText("Connecting to Controller...");
        qApp->processEvents();

        ftdi_devices[controller_device_num].baud_rate = 250000;
        while ((ctrl_handle == NULL) && (connect_count < 10))
        {
            ctrl_handle = open_com_port(ftdi_devices[controller_device_num], read_timeout, write_timeout);
            QThread::msleep(50);
            ++connect_count;
        }

        if (ctrl_handle == NULL)
        {
            ui->console_te->append("No Controller found...");
        }

        ctrl.tx = data_packet(DRIVER_CONNECT);

        // send connection request packet and get response back
        ctrl.send_packet(ctrl_handle, ctrl.tx);
        status = ctrl.receive_packet(ctrl_handle, 6, ctrl.rx);

        if (status == false)
        {
            ui->console_te->append("No Controller found...");
        }

        // get the controller information and display
        ctrl.set_driver_info(ctrl.rx);
        ui->console_te->append("-----------------------------------------------------------------------------");
        ss << ctrl;
        ui->console_te->append(QString::fromStdString(ss.str()));
        ss.str(std::string());
        ss.clear();

        QThread::msleep(50);
        qApp->processEvents();

        //-----------------------------------------------------------------------------
        // ping the motors to get the model number and firmware version
        status = ctrl.ping_motor(ctrl_handle, FOCUS_MOTOR_ID, focus_motor);

        ui->console_te->append("-----------------------------------------------------------------------------");
        ui->console_te->append("Focus Motor Information: ");

        if (status)
        {
            ss << focus_motor;
            ui->console_te->append(QString::fromStdString(ss.str()));
            ss.str(std::string());
            ss.clear();
        }
        else
        {
            ui->console_te->append("  Error getting focus motor info");
        }
//        ui->console_te->show();
        qApp->processEvents();

        // configure the homing offsets for the focus motor
        // set the offset to zero
        status = ctrl.set_offset(ctrl_handle, FOCUS_MOTOR_ID);

        // get the current motor positions
        status = ctrl.get_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);

        ui->console_te->append("  Current Step:     " + QString::number(focus_step));
        QThread::msleep(50);

        //-----------------------------------------------------------------------------
        status = ctrl.ping_motor(ctrl_handle, ZOOM_MOTOR_ID, zoom_motor);

        ui->console_te->append("\n-----------------------------------------------------------------------------");
        ui->console_te->append("Zoom Motor Information: ");

        if (status)
        {
            ss << zoom_motor;
            ui->console_te->append(QString::fromStdString(ss.str()));
            ss.str(std::string());
            ss.clear();
        }
        else
        {
            ui->console_te->append("  Error getting zoom motor info");
        }
//        ui->console_te->show();
        qApp->processEvents();

        // configure the homing offsets for the zoom motor
        // set the offset to zero
        status = ctrl.set_offset(ctrl_handle, ZOOM_MOTOR_ID);

        // get the current motor positions
        status = ctrl.get_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_step);

        ui->console_te->append("  Current Step:     " + QString::number(zoom_step));
        //ui->console_te->append("-----------------------------------------------------------------------------\n");

        QThread::msleep(50);

        // configure the trigger according to the inputs
        // channel 1
        status = ctrl.config_channel(ctrl_handle, CONFIG_T1, tc_ch1);

        // channel 2
        status = ctrl.config_channel(ctrl_handle, CONFIG_T2, tc_ch2);

        //-----------------------------------------------------------------------------
        // get the current trigger configurations and display the information
        status = ctrl.get_trigger_info(ctrl_handle, t1_info, t2_info);

        ui->console_te->append("\n-----------------------------------------------------------------------------");
        ui->console_te->append("Trigger Information: ");
        ss << t1_info << std::endl;
        //ui->console_te->append(QString::fromStdString(ss.str()) + "\n");
        //ss.clear();

        ss << t2_info;
        ui->console_te->append(QString::fromStdString(ss.str()));
        ss.str(std::string());
        ss.clear();
        ui->console_te->append("-----------------------------------------------------------------------------");
        //ui->console_te->show();
        qApp->processEvents();
        QThread::msleep(50);

        //-----------------------------------------------------------------------------
        // read in the position PID config file and set the PID values for each motor
        status = read_pid_config(pid_config_filename, (uint32_t)(ctrl.ctrl_info.serial_number - 1), pid_values);

        if (status == false)
        {
            ui->console_te->append("The pid_config.txt file does not have enough entries based on supplied serial number.  Using default values.");
            //ui->console_te->show();

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
        //ui->console_te->show();
        QThread::msleep(50);

        status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_range[0]);
        status &= ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_range[0]);

        if (!status)
        {
            ui->console_te->append("Error setting motor positions.");
        }
//        ui->console_te->show();
        qApp->processEvents();

        // disable the motors
        status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);
        status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);

        ui->ftdi_connect_btn->setText("Disconnect");
        ctrl_connected = true;
    }
    else
    {

        ui->console_te->append("\nSetting motors back to zero...");
        status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, true);
        status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, 0);
        status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);
        QThread::msleep(50);

        status = ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, true);
        status = ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, 0);
        status = ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);
        QThread::msleep(50);

        // close the motor driver port first
        ui->console_te->append("\nClosing the Controller port...");
        qApp->processEvents();
        close_com_port(ctrl_handle);

        ui->ftdi_connect_btn->setText("Connect");
        ctrl_connected = false;
    }


}

void capture_gui::on_cam_connect_btn_clicked()
{
    std::stringstream ss;

    if(cam_connected == false)
    {
        // get the selected camera
        cam = cam_list.GetByIndex(ui->cam_cb->currentIndex());

        // print out some information about the camera
        ui->console_te->append("------------------------------------------------------------------");
        ss << cam;
        ui->console_te->append(QString::fromStdString(ss.str()));
        ss.str(std::string());
        ss.clear();
        qApp->processEvents();

        //data_log_stream << cam;

        // initialize the camera
        init_camera(cam);
        cam_connected = true;
        //cam->TriggerActivation.SetValue(Spinnaker::TriggerActivationEnums::TriggerActivation_RisingEdge);

        get_temperature(cam, camera_temp);
        ui->console_te->append("  Camera Temp:       " + QString::number(camera_temp) + "\n");
        qApp->processEvents();

        // config the image size
        //get_image_size(cam, height, width, y_offset, x_offset);
        //std::cout << "Image Size (h x w): " << height << " x " << width << ", [" << x_offset << ", " << y_offset << "]" << std::endl;

        width = (uint64_t)ui->width->text().toInt();
        height = (uint64_t)ui->height->text().toInt();
        x_offset = (uint64_t)ui->x_offset->text().toInt();
        y_offset = (uint64_t)ui->y_offset->text().toInt();

        camera_gain = ui->gain->text().toDouble();

        switch(ui->px_format->currentIndex())
        {
        case 0:
            pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_BGR8;
            break;

        case 1:
            pixel_format = Spinnaker::PixelFormatEnums::PixelFormat_Mono12;
            break;
        }

        // configure the camera
        set_image_size(cam, height, width, y_offset, x_offset);
        set_pixel_format(cam, pixel_format);
        set_gain_mode(cam, gain_mode);
        set_gain_value(cam, camera_gain);
        set_exposure_mode(cam, exp_mode);
        set_exposure_time(cam, exp_time);
        set_acquisition_mode(cam, acq_mode); //acq_mode

        // print out the camera configuration
    //    ui->console_te->append("------------------------------------------------------------------");
    //    ui->console_te->append("Camera Configuration:");


        //get_image_size(cam, height, width, y_offset, x_offset);

        // pixel format
        //get_pixel_format(cam, pixel_format);
        //get_gain_value(cam, camera_gain);

        //set_gain_mode(cam, gain_mode);

        // exposure
    //    double tmp_exp_time;
    //    get_exposure_time(cam, tmp_exp_time);

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

        image_timer = new QTimer(this);
        connect(image_timer, SIGNAL(timeout()), this, SLOT(update_image()));
        image_timer->start(16);

        cv::namedWindow(image_window, cv::WindowFlags::WINDOW_NORMAL);

        cam_connected = true;
    }
    else
    {

        ui->console_te->append("\nClosing Camera...");

        // disconnect the timer signal
        disconnect(image_timer, SIGNAL(timeout()), 0, 0);

        // de-initialize the camera
        cam->DeInit();

        // Release reference to the camera
        cam = nullptr;

        // Clear camera list before releasing system
        cam_list.Clear();

        // Release system
        cam_system->ReleaseInstance();

        cv::destroyAllWindows();
        cam_connected = false;

    }

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

//    QString save_location = QFileDialog::getExistingDirectory(0, ("Select Output Folder"), ("../" + QDir::currentPath()));
    QString save_location = QFileDialog::getExistingDirectory(0, ("Select Output Folder"), ("../"));

    ui->save_location->setText(save_location);

    output_save_location = path_check(save_location.toStdString());

}

void capture_gui::update_zoom_position()
{
    int32_t position = 0;

    // set the current step to the minimum
    bool status = ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, true);
    status &= ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_range[0]);
    status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);

    status &= ctrl.get_position(ctrl_handle, ZOOM_MOTOR_ID, position);
    ui->console_te->append("zoom motor: " + QString::number(position));
}

void capture_gui::update_focus_position()
{
    int32_t position = 0;

    // set the current step to the minimum
    bool status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, true);
    status &= ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_range[0]);
    status &= ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);

    status &= ctrl.get_position(ctrl_handle, FOCUS_MOTOR_ID, position);
    ui->console_te->append("focus motor: " + QString::number(position));
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

void capture_gui::z_start_edit_complete()
{
    int32_t start = (int32_t)ui->z_start->text().toInt();
    int32_t step = (int32_t)ui->z_step->text().toInt();
    int32_t stop = (int32_t)ui->z_stop->text().toInt();

    // generate the step ranges
    generate_range(start, stop, step, zoom_range);

    if(ctrl_connected == true)
        update_zoom_position();

}

void capture_gui::z_step_edit_complete()
{
    int32_t start = (int32_t)ui->z_start->text().toInt();
    int32_t step = (int32_t)ui->z_step->text().toInt();
    int32_t stop = (int32_t)ui->z_stop->text().toInt();

    // generate the step ranges
    generate_range(start, stop, step, zoom_range);

}

void capture_gui::z_stop_edit_complete()
{
    int32_t start = (int32_t)ui->z_start->text().toInt();
    int32_t step = (int32_t)ui->z_step->text().toInt();
    int32_t stop = (int32_t)ui->z_stop->text().toInt();

    // generate the step ranges
    generate_range(start, stop, step, zoom_range);

}


//void capture_gui::on_f_start_valueChanged(int arg1)
void capture_gui::f_start_edit_complete()
{
    int32_t start = (int32_t)ui->f_start->text().toInt();
    int32_t step = (int32_t)ui->f_step->text().toInt();
    int32_t stop = (int32_t)ui->f_stop->text().toInt();

    // generate the step ranges
    generate_range(start, stop, step, focus_range);

    if(ctrl_connected == true)
        update_focus_position();
}

void capture_gui::f_step_edit_complete()
{
    int32_t start = (int32_t)ui->f_start->text().toInt();
    int32_t step = (int32_t)ui->f_step->text().toInt();
    int32_t stop = (int32_t)ui->f_stop->text().toInt();

    // generate the step ranges
    generate_range(start, stop, step, focus_range);

}

void capture_gui::f_stop_edit_complete()
{
    int32_t start = (int32_t)ui->f_start->text().toInt();
    int32_t step = (int32_t)ui->f_step->text().toInt();
    int32_t stop = (int32_t)ui->f_stop->text().toInt();

    // generate the step ranges
    generate_range(start, stop, step, focus_range);

}

void capture_gui::x_offset_edit_complete()
{
    x_offset = (uint64_t)ui->x_offset->text().toInt();

    if(cam_connected == true)
        set_image_size(cam, height, width, y_offset, x_offset);
}

void capture_gui::y_offset_edit_complete()
{
    y_offset = (uint64_t)ui->y_offset->text().toInt();

    if(cam_connected == true)
        set_image_size(cam, height, width, y_offset, x_offset);
}

void capture_gui::height_edit_complete()
{
    height = (uint64_t)ui->height->text().toInt();

    if(cam_connected == true)
        set_image_size(cam, height, width, y_offset, x_offset);
}

void capture_gui::width_edit_complete()
{
    width = (uint64_t)ui->width->text().toInt();

    if(cam_connected == true)
        set_image_size(cam, height, width, y_offset, x_offset);
}

void capture_gui::gain_edit_complete()
{
    camera_gain = ui->gain->text().toDouble();

    if(cam_connected == true)
        set_gain_value(cam, camera_gain);

}

void capture_gui::exposure_edit_complete()
{
    exp_time = ui->exposure->text().toDouble();

    if(cam_connected == true)
        set_exposure_time(cam, exp_time);

}


void capture_gui::update_image()
{
    cv::RNG rng(time(NULL));
    cv_image = cv::Mat(200,200, CV_8UC3, cv::Scalar(rng.uniform(0, 256), rng.uniform(0, 256), rng.uniform(0, 256)));

    cv::imshow(image_window, cv_image);
    cv::waitKey(1);

}

void capture_gui::on_start_capture_clicked()
{
    uint32_t focus_idx, zoom_idx, img_idx;
    bool status;
    std::string focus_str, zoom_str, exposure_str;
    std::string image_str;
    std::string image_header = "image_";
    std::string image_capture_name = "image_";
    std::string img_save_folder;
    std::string sdate, stime;

    double tmp_exp_time;


    if(ctrl_connected == false || cam_connected == false)
        return;

    uint32_t cap_num = (uint32_t)ui->num_caps->text().toInt();

    // disable the gui items
    ui->controller_gb->setEnabled(false);
    ui->camera_gb->setEnabled(false);

    int32_t dir_status = mkdir(output_save_location);
    if (dir_status != 0)
        std::cout << "Error creating folder: " << dir_status << std::endl;

    // start the data logging by creating the file
    get_current_time(sdate, stime);
    img_save_folder = output_save_location + sdate + "_" + stime + "/";

    int32_t stat = mkdir(img_save_folder);
    if (stat != 0 && stat != (int32_t)ERROR_ALREADY_EXISTS)
    {
         ui->console_te->append("Error creating directory:");
    }

    get_current_time(sdate, stime);
    std::string log_filename = "camera_capture_log_" + sdate + "_" + stime + ".txt";

    ui->console_te->append("Log File: " + QString::fromStdString(output_save_location + log_filename)  + "\n");
    data_log_stream.open((output_save_location + log_filename), ios::out | ios::app);

    // Add the date and time to the start of the log file
    data_log_stream << "#------------------------------------------------------------------" << std::endl;
    data_log_stream << "Version: 1.0    Date: " << sdate << "    Time: " << stime << std::endl << std::endl;
    data_log_stream << "#------------------------------------------------------------------" << std::endl;

//    data_log_stream << "Input Parameters:" << std::endl;
//    data_log_stream << "Image Size (h x w):  " << height << " x " << width << std::endl;
//    data_log_stream << "Image Offset (x, y): " << x_offset << ", " << y_offset << std::endl;
//    data_log_stream << "Focus Step Range:    " << focus_str << std::endl;
//    data_log_stream << "Zoom Step Range:     " << zoom_str << std::endl;
//    data_log_stream << "Exposure Time Range: " << exposure_str << std::endl;
//    data_log_stream << "Camera Gain:         " << camera_gain << std::endl;
//    data_log_stream << "Number of Captures:  " << cap_num << std::endl;

    data_log_stream << "Save location: " << img_save_folder << std::endl;
    ui->console_te->append("------------------------------------------------------------------");

    // enable the motors
    status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, true);
    status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, true);

    sleep_ms(10);

    focus_step = 0;
    zoom_step = 0;

    // set the camera settings
    set_image_size(cam, height, width, y_offset, x_offset);
    set_pixel_format(cam, pixel_format);
    //set_gain_mode(cam, gain_mode);
    set_gain_value(cam, camera_gain);
    //set_exposure_mode(cam, exp_mode);
    set_exposure_time(cam, exp_time);
    //set_acquisition_mode(cam, acq_mode); //acq_mode

    // get all of the camera settings
    get_image_size(cam, height, width, y_offset, x_offset);

    // pixel format
    get_pixel_format(cam, pixel_format);
    get_gain_value(cam, camera_gain);

    // exposure
    get_exposure_mode(cam, exp_mode);
    get_exposure_time(cam, exp_time);
    get_acquisition_mode(cam, acq_mode);

    // dispaly the capture parameters
    ui->console_te->append("Image Size (h x w):       " + QString::number(height) + " x " + QString::number(width));
    ui->console_te->append("Image Offset (x, y):      " + QString::number(x_offset) + ", " + QString::number(y_offset));
    ui->console_te->append("Pixel Format:             " + QString::fromStdString(cam->PixelFormat.GetCurrentEntry()->GetSymbolic().c_str()));
    ui->console_te->append("ADC Bit Depth:            " + QString::fromStdString(cam->AdcBitDepth.GetCurrentEntry()->GetSymbolic().c_str()));
    ui->console_te->append("Gain Mode/Value:          " + QString::fromStdString(cam->GainAuto.GetCurrentEntry()->GetSymbolic().c_str()) + " / " + QString::number(camera_gain));
    ui->console_te->append("Exposure Mode/Value (ms): " + QString::fromStdString(cam->ExposureAuto.GetCurrentEntry()->GetSymbolic().c_str()) +  " / " + QString::number(exp_time/1000.0));
    ui->console_te->append("Acquistion Mode:          " + QString::fromStdString(cam->AcquisitionMode.GetCurrentEntry()->GetSymbolic().c_str()));
    ui->console_te->append("Number of Captures:       " + QString::number(cap_num));
    ui->console_te->append("Trigger Source:           " + QString::fromStdString(cam->TriggerSource.GetCurrentEntry()->GetSymbolic().c_str()));
    ui->console_te->append("Min/Max Gain:             " + QString::number(cam->Gain.GetMin()) + " / " + QString::number(cam->Gain.GetMax()));
    ui->console_te->append("Min/Max Exposure (ms):    " + QString::number(cam->ExposureTime.GetMin()/1000.0) + " / " + QString::number((uint64_t)cam->ExposureTime.GetMax()/1000.0) + "\n");
    ui->console_te->show();

    // save the capture parameters to a file
    data_log_stream << "-----------------------------------------------------------------------------" << std::endl;
    data_log_stream << ctrl << std::endl;

    data_log_stream << "-----------------------------------------------------------------------------" << std::endl << std::endl;
    data_log_stream << "Focus Motor Information: " << std::endl;
    data_log_stream << focus_motor;
    data_log_stream << "  Current Step:     " << focus_step << std::endl;

    data_log_stream << "-----------------------------------------------------------------------------" << std::endl;
    data_log_stream << "Zoom Motor Information: " << std::endl;
    data_log_stream << zoom_motor;
    data_log_stream << "  Current Step:     " << zoom_step << std::endl;

    data_log_stream << "#----------------------------------------------------------------------------" << std::endl;
    data_log_stream << "Trigger Information: " << std::endl;
    data_log_stream << t1_info << std::endl;
    data_log_stream << t2_info;
    data_log_stream << "#----------------------------------------------------------------------------" << std::endl << std::endl;


    data_log_stream << "-----------------------------------------------------------------------------" << std::endl;
    data_log_stream << "Image Size (h x w):       " << height << " x " << width << std::endl;
    data_log_stream << "Image Offset (x, y):      " << x_offset << ", " << y_offset << std::endl;
    data_log_stream << "Pixel Format:             " << cam->PixelFormat.GetCurrentEntry()->GetSymbolic() << std::endl;
    data_log_stream << "ADC Bit Depth:            " << cam->AdcBitDepth.GetCurrentEntry()->GetSymbolic() << std::endl;
    data_log_stream << "Gain Mode/Value:          " << cam->GainAuto.GetCurrentEntry()->GetSymbolic() << " / " << camera_gain << std::endl;
    data_log_stream << "Exposure Mode/Value (ms): " << cam->ExposureAuto.GetCurrentEntry()->GetSymbolic() << " / " << exp_time/1000.0 << std::endl;
    data_log_stream << "Acquistion Mode:          " << cam->AcquisitionMode.GetCurrentEntry()->GetSymbolic() << std::endl;
    data_log_stream << "Number of Captures:       " << cap_num << std::endl;
    data_log_stream << "Trigger Source:           " << cam->TriggerSource.GetCurrentEntry()->GetSymbolic() << std::endl;
    data_log_stream << "Min/Max Gain:             " << cam->Gain.GetMin() << " / " << cam->Gain.GetMax() << std::endl;
    data_log_stream << "Min/Max Exposure (ms):    " << cam->ExposureTime.GetMin()/1000.0 << " / " << (uint64_t)cam->ExposureTime.GetMax()/1000.0 << std::endl;
    data_log_stream << std::endl << "#------------------------------------------------------------------" << std::endl;

    // disconnect the timer for displaying the camera images
    disconnect(image_timer, SIGNAL(timeout()), 0, 0);


    // set the trigger


    // loop through the zoom and focus settings
    for (zoom_idx = 0; zoom_idx < zoom_range.size(); ++zoom_idx)
    {
        // set the zoom motor value to each value in focus_range
        status = ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_range[zoom_idx]);
        status = ctrl.get_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_step);

        zoom_str = num2str(zoom_step, "z%05d_");

        for (focus_idx = 0; focus_idx < focus_range.size(); ++focus_idx)
        {
            // set the focus motor value to each value in focus_range
            status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_range[focus_idx]);
            status = ctrl.get_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);

            focus_str = num2str(focus_step, "f%05d_");
            sleep_ms(5);

            for (img_idx = 0; img_idx < cap_num; ++img_idx)
            {

                // grab an image: either from the continuous, single or triggered
//                switch (ts)
//                {
//                case 0:
//                    cam->BeginAcquisition();
//                    status = ctrl.trigger(ctrl_handle, TRIG_ALL);
//                    aquire_trigger_image(cam, image);
//                    cam->EndAcquisition();
//                    break;
//                case 1:
                    aquire_software_trigger_image(cam, image);
//                    break;
//                }

                get_exposure_time(cam, tmp_exp_time);
                exposure_str = num2str(tmp_exp_time, "e%05.0f_");

                image_capture_name = image_header + zoom_str + focus_str + exposure_str + num2str(img_idx, "i%02d") + ".png";

                cv_image = cv::Mat(height + y_padding, width + x_padding, CV_8UC3, image->GetData(), image->GetStride());

                cv::namedWindow(image_window, cv::WindowFlags::WINDOW_NORMAL);
                cv::imshow(image_window, cv_image);
                cv::waitKey(1);

                // save the image
                ui->console_te->append("saving: " + QString::fromStdString(img_save_folder) + QString::fromStdString(image_capture_name));
                data_log_stream << image_capture_name << std::endl;

                cv::imwrite((img_save_folder + image_capture_name), cv_image, compression_params);
                //std::cout << image_capture_name << "," << num2str(tmp_exp_time, "%2.2f") << std::endl;
                //sleep_ms(100);

            }   // end of img_idx loop



//            set_exposure_time(cam, exp_time);

        }   // end of focus_idx loop

    }   // end of zoom_idx loop

    // set the focus step to the first focus_range setting
    status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_range[0]);
    status = ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_range[0]);

    // disable the motors
    status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);
    status &= ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);

    ui->console_te->append("------------------------------------------------------------------");
    data_log_stream << "#------------------------------------------------------------------" << std::endl;


    // re-enable the gui controlls
    ui->controller_gb->setEnabled(true);
    ui->camera_gb->setEnabled(true);

    // reconnect the timer for displaying camera images
    connect(image_timer, SIGNAL(timeout()), this, SLOT(update_image()));


}

void capture_gui::closeEvent (QCloseEvent *event)
{

    bool status;

    // set the focus and zoom steps to zero
    focus_step = 0;
    zoom_step = 0;

    if(ctrl_connected == true)
    {
        ui->console_te->append("\nSetting motors back to zero...");
        ui->console_te->show();

        status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, true);
        status = ctrl.set_position(ctrl_handle, FOCUS_MOTOR_ID, focus_step);
        status = ctrl.enable_motor(ctrl_handle, FOCUS_MOTOR_ID, false);

        status = ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, true);
        status = ctrl.set_position(ctrl_handle, ZOOM_MOTOR_ID, zoom_step);
        status = ctrl.enable_motor(ctrl_handle, ZOOM_MOTOR_ID, false);

        // close the motor driver port first
        ui->console_te->append("\nClosing the Controller port...");
        ui->console_te->show();

        close_com_port(ctrl_handle);
        ctrl_connected = false;
    }

    if(cam_connected == true)
    {
        // close out the camera
        ui->console_te->append("\nClosing Camera...");
        ui->console_te->show();

        // de-initialize the camera
        cam->DeInit();

        // Release reference to the camera
        cam = nullptr;

        // Clear camera list before releasing system
        cam_list.Clear();

        // Release system
        cam_system->ReleaseInstance();
        cam_connected = false;
    }

    cv::destroyAllWindows();

    data_log_stream.close();

    ui->console_te->append("\nProgram Complete!");
    ui->console_te->show();
}


