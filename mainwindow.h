#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include "shared_include.h"
#include <nrtexe.h>
#include <usbcam.h>
#include <predict.h>
#include <database.h>
#include <experimental/filesystem>
namespace fs = std::experimental::filesystem;

struct Mat_With_Name
{
    cv::Mat image;
    std::string name;
};

struct Share_Mat
{
    Share_Mat() : save_csv_flag(false) {}
    std::mutex _mutex;

    std::queue<Mat_With_Name> org_buffer;     // CAM Mode
//    std::queue<Mat_With_Name> cam_buffer;     // CLA
    std::queue<Mat_With_Name> mask_buffer;    // SEG
    std::queue<Mat_With_Name> pred_buffer;    // CLA, SEG, DET, OCR, ANO

    QFile json_file;

    std::queue<vector<std::string>> row_buffer;
    bool save_csv_flag;

    string save_path;
};

namespace Ui {
class MainWindow;
}

class MainWindow : public QMainWindow
{
    Q_OBJECT

public:
    // explicit: 사용자가 원하지 않는 형 변환 방지.
    explicit MainWindow(QWidget *parent = 0);
    ~MainWindow();

    void center_and_resize();
    void save_worker();
    void sqliteDBSetup();
    shared_ptr<UsbCam> m_usbCam;

public slots:
    void showResult();

private slots:
    void paintEvent(QPaintEvent *paintEvent);

    void setCamSelectEnabled(bool flag);
    void setCamControlEnabled(bool flag);
    void setCamSaveEditEnabled(bool flag);
    void setCamSaveChangeEnabled(bool flag);
    bool checkCanSave();
    void setTabelColumn(bool flag);
    void resultItemClicked(int row, int col);
    void resultVerClicked(int row);
//    void on_btn_save_img_result_clicked();
    void setModelInfo(bool flag, QString model_name);
    void setClassTable(bool flag);
    void tableItemClicked(int row, int col);
    void tableItemChanged(int row, int col);
    void setSaveStautus();

    void setImgShowTimeEditEnable(bool flag);
    void setImgInference(bool flag);
    void setImgResultShow(bool flag);

    void on_btn_cam_mode_clicked();
    void on_btn_img_mode_clicked();

    void on_btn_cam_select_clicked();
    void on_btn_cam_save_clicked();

    void on_btn_cam_play_clicked();
    void on_btn_cam_pause_clicked();
    void on_btn_cam_stop_clicked();

    void on_rad_cam_rtmode_clicked();
    void on_rad_cam_savemode_clicked();
    void on_chb_show_prediction_clicked();

    void on_btn_select_model_clicked();
    void set_model_started();
    void set_model_completed();
    void on_btn_select_gpu_clicked();

    void on_btn_img_input_clicked();
    void on_btn_img_output_clicked();
    void on_cbx_set_show_time_stateChanged(int arg1);
    void on_btn_img_play_clicked();
    void on_btn_img_pause_clicked();
    void on_btn_img_stop_clicked();

    void on_btn_show_img_result_clicked();

    void on_spb_img_cur_idx_valueChanged(int arg1);

    void on_btn_show_prediction_clicked();

    void on_com_cam_input_select_currentTextChanged(const QString& text);

    void claSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row);
    void segSetResults(nrt::NDBuffer merged_pred_output, cv::Mat &PRED_IMG, vector<std::string> &new_row);
    void detSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row);
    void ocrSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row);
    void anomSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row);

private:
    Ui::MainWindow *ui;
    QStringList camType;
    shared_ptr<QTimer> m_timer; // executes showResult at given time interval
    shared_ptr<QTimer> m_save_timer = std::make_shared<QTimer>(this); // executer setSaveStatus at given time interval

    // QFutureWatcher & QFuture for executor creating thread
    std::shared_ptr<QFutureWatcher<QString>> futureWatcher = std::make_shared<QFutureWatcher<QString>>(this);
    QFuture<QString> future;

    // Image input and output folder directory.
    std::shared_ptr<QString> m_input_path;
    std::shared_ptr<QString> m_output_path;

    // SQLite database connection
    std::shared_ptr<neuroeDB> m_nrtDB;

    // Video Input Capture Object
    cv::VideoCapture m_videoInputCap;

    bool video_mode_flag = false;
    bool cam_mode_flag = false;

    bool show_result_table = false;
    bool show_pred_flag = true;

    bool mode_flag = false; // false: CAM Mode, true: IMG Mode

    bool cur_save_flag = false;  // true: push to buffer
    bool cam_save_flag = false;  // false: Realtime, true: Save

    QStringList inf_img_list;
    bool img_show_time_flag = false; // true: show each image in setted show time
    bool img_inf_ing_flag = false;

    Share_Mat m_save_info;

    // FOR AI EXPO
    bool macro_flag = false;
    bool macro_cam_flag = false;

    bool class_table_availbale = false;
};

#endif // MAINWINDOW_H
