#ifndef _CAPTURE_GUI_H_
#define _CAPTURE_GUI_H_

#include <cstdio>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <sstream>

#include "Spinnaker.h"
#include "SpinGenApi/SpinnakerGenApi.h"

// FTDI Driver Includes
#include "ftd2xx_functions.h"

#include <QMainWindow>
#include <QMessageBox>
#include <QFileDialog>
#include <QTimer>
#include <QCloseEvent>
#include <QThread>

QT_BEGIN_NAMESPACE
namespace Ui { class capture_gui; }
QT_END_NAMESPACE

class capture_gui : public QMainWindow
{
    Q_OBJECT

public:
    capture_gui(QWidget *parent = nullptr);
    ~capture_gui();


public slots:

    void update_zoom_position();
    void update_focus_position();

private slots:
    void on_ftdi_connect_btn_clicked();

    void on_cam_connect_btn_clicked();


    void on_px_format_currentIndexChanged(int index);

    void on_toolButton_clicked();

    void z_start_edit_complete();
    void z_step_edit_complete();
    void z_stop_edit_complete();

    void f_start_edit_complete();
    void f_step_edit_complete();
    void f_stop_edit_complete();

    void x_offset_edit_complete();
    void y_offset_edit_complete();
    void height_edit_complete();
    void width_edit_complete();
    void gain_edit_complete();
    void exposure_edit_complete();

    void update_image();

    void on_start_capture_clicked();

private:
    Ui::capture_gui *ui;

    FT_HANDLE ctrl_handle = NULL;

    // motor variables
    std::string pid_config_filename = "pid_config.txt";
    std::vector<int32_t> focus_range, zoom_range;
    int32_t focus_step = 0;
    int32_t zoom_step = 0;
    bool mtr_moving = false;
    std::vector<uint16_t> pid_values;

    // trigger variables
    std::vector<uint8_t> tc_ch1;
    std::vector<uint8_t> tc_ch2;
    uint8_t t1_polarity, t2_polarity;
    uint32_t t1_offset, t2_offset;
    uint32_t t1_length, t2_length;

    bool ctrl_connected = false;

    // camera variables
    uint32_t cam_index;
    uint32_t num_cams;
    uint64_t width, height, x_offset, y_offset;
    uint32_t x_padding, y_padding;
    uint32_t ts = 0;
    //uint32_t sharpness;
    double camera_temp;
    double camera_gain;
    double frame_rate;
    double exp_time;

    bool cam_connected = false;

//    QString save_location;
    std::string output_save_location;

    QTimer *image_timer;

    template <typename T>
    void generate_range(T start, T stop, T step, std::vector<T>& range);

    void closeEvent (QCloseEvent *event);

};
#endif // _CAPTURE_GUI_H_
