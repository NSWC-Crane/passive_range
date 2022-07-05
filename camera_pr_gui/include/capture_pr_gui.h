#ifndef _CAPTURE_PR_GUI_H_
#define _CAPTURE_PR_GUI_H_

#include <cstdio>
#include <cstdint>
#include <ctime>
#include <iostream>
#include <sstream>

// OpenCV Includes
#include <opencv2/core.hpp>

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
namespace Ui { class capture_pr_gui; }
QT_END_NAMESPACE

class capture_pr_gui : public QMainWindow
{
    Q_OBJECT

public:
    capture_pr_gui(QWidget *parent = nullptr);
    ~capture_pr_gui();


public slots:

    void update_zoom_position();
    void update_focus_position();

private slots:
    void on_ftdi_connect_btn_clicked();

    void on_cam_connect_btn_clicked();


    void on_px_format_currentIndexChanged(int index);

    void on_toolButton_clicked();
    //void save_location_update();

    void zoom_edit_complete();
    void zoom_fp_complete();

    void focus_edit_complete();
    void focus_fp_complete();

    void image_size_edit_complete();

    void gain_edit_complete();
    void exposure_edit_complete();

    void update_image();

    void on_start_capture_clicked();

//    void on_stop_capture_clicked();

    void on_auto_gain_stateChanged(int arg1);

    void on_auto_exp_stateChanged(int arg1);

    void on_motor_position_clicked();

private:
    Ui::capture_pr_gui *ui;

    FT_HANDLE ctrl_handle = NULL;

    // motor variables
    std::string pid_config_filename = "pid_config.txt";
    //std::vector<int32_t> focus_range, zoom_range;
    int32_t focus_fp1, focus_fp2, zoom_fp1, zoom_fp2;
    int32_t focus_idx, zoom_idx;
//    int32_t focus_step = 0;
//    int32_t zoom_step = 0;
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
    uint64_t img_w, img_h;  //, x_offset, y_offset;
    uint32_t x_padding, y_padding;
    uint32_t ts = 0;
    unsigned int num_classes;

    double camera_temp;
    double camera_gain;
    double frame_rate;
    double exp_time;

    const uint32_t max_height = 1536;
    const uint32_t max_width = 2048;

    uint32_t range_w = 64;
    uint32_t range_h = 64;

    bool cam_connected = false;

    cv::Mat img_f1, img_f2;
    cv::Mat dm;
    cv::Rect crop_rect;

    //QString save_location;
    //std::string output_save_location;

    bool stop_capture = false;

    QTimer *image_timer;

    template <typename T>
    void generate_range(T start, T stop, T step, std::vector<T>& range);

    void closeEvent (QCloseEvent *event);

};
#endif // _CAPTURE_PR_GUI_H_
