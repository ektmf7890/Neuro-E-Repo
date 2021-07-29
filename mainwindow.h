#ifndef MAINWINDOW_H
#define MAINWINDOW_H

#include <sqlitedb.h>
#include <nrtexe.h>
#include <usbcam.h>
#include <predict.h>
#include "shared_include.h"

struct Mat_With_Name
{
    cv::Mat image;
    std::string  name;
    int imageId;
};

struct Share_Mat
{
    Share_Mat() : save_csv_flag(false) {}
    std::mutex _mutex;

    std::queue<Mat_With_Name> org_buffer;     // CAM Mode
    std::queue<Mat_With_Name> pred_buffer;    // CLA, SEG, DET, OCR, ANO
    std::queue<vector<std::string>> row_buffer;

    bool save_csv_flag;

    int evaluationSetId;

    QFile json_file;
    QJsonDocument json_doc;
    QJsonObject title;
};


QString set_model_thread(NrtExe* nrt_ptr, QString modelPath, bool fp16_flag);

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

protected:
    bool eventFilter(QObject* obj, QEvent* ev) override;
    void resizeEvent(QResizeEvent* event) override;

public slots:
    void showResult();
    void model_table_item_clicked(int row, int col);
    void model_table_item_changed(int row, int col);

private slots:
    // single mode 전용 set result 함수들.
    void claSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name);
    void segSetResults(nrt::NDBuffer merged_pred_output, nrt::NDBuffer merged_prob_output, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name);
    void detSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name);
    void ocrSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name);
    void anomSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name);

    // ensmble 중 det->cla set result 함수.
    void detClaEnsmbleSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name);
    void segClaEnsembleSetResults(nrt::NDBuffer merged_pred_output, nrt::NDBuffer merged_prob_output, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name);
    void claDetEnsembleSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name);
    void claSegEnsembleSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name);
    void claClaEnsembleSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name);

    void on_btn_cam_mode_clicked();
    void on_btn_video_mode_clicked();
    void on_btn_img_mode_clicked();

    void on_btn_cam_select_clicked();
    void on_btn_video_input_clicked();

    void on_btn_select_single_mode_clicked();
    void set_model_started();

    void on_com_cam_input_select_currentTextChanged(const QString& text);
    void on_btn_delete_dir_clicked();

    void on_com_video_input_currentTextChanged(const QString& text);

    void initJson(int evaluationSetId);

    void setInfMode(int infMode);
    bool is_ready_for_inf(NrtExe*m_nrt_ptr);
    bool select_device_dialog(NrtExe* nrt_ptr);

    void on_com_video_list_currentTextChanged(const QString& text);
    void on_table_video_files_itemSelectionChanged();
    void play_video(int video_idx);

    void on_table_images_itemSelectionChanged();

    void setUiForEnsemble();
    void setUiForSingle();

    void ensemble_model_start();

    void on_btn_model_settings_clicked();

    void verticalResizeTable(QTableView* table_view);

    void on_btn_run_clicked();
    void on_btn_review_clicked();

    void on_btn_single_clicked();
    void on_btn_ensmble_clicked();

    void on_btn_new_images_clicked();

    void on_btn_play_clicked();
    void on_btn_pause_clicked();
    void on_btn_stop_clicked(); // image: inference 종료, CAM: 사용 안함, Video: 현재 영상 종료(inference 중지 X)

    void on_btn_disconnect_camera_clicked(); // CAM mode에서 inference 종료
    void on_btn_video_stop_inf_clicked(); // Video mode에서 inference 종료

    void on_btn_start_inference_clicked();

    void ready_for_inference_check();

    void insert_video_table(QString video_filepath);

    void show_result_image_table(int evaluationSetId);

    void update_pie_chart(int idx, int START_POINT);
    void update_results_table(int idx, int START_POINT);

    void save_settings_dialog();

    void on_btn_full_screen_viewer_clicked();

    void initialize_settings_page();

    bool saveEvaluationJson();

    void directory_updated(QString path);

    void on_com_ensemble_options_currentIndexChanged(int index);

    void on_btn_select_ens_mode_clicked();

    void ens_settings_dialog();

    void set_model_table(int idx);
    void set_model_chart(int idx);

    void on_btn_settings_clicked();

    void on_btn_delete_videos_clicked();
    void on_btn_delete_images_clicked();

    void on_result_images_table_itemSelectionChanged();

    void initialize_review_page();
    void initialize_realtime_page();

    void ensemble_realtime_setting();
    void single_realtime_setting();

private:
    Ui::MainWindow *ui;
    shared_ptr<QTimer> m_timer; // executes showResult at given time interval

    // QFutureWatcher & QFuture for executor creating thread
    std::shared_ptr<QFutureWatcher<QString>> futureWatcher = std::make_shared<QFutureWatcher<QString>>(this);
    std::shared_ptr<QFutureWatcher<QString>> futureWatcherEns = std::make_shared<QFutureWatcher<QString>>(this);
    QFuture<QString> future;
    QFuture<QString> futureEns;

    // cur time
    QTimer* timer;

    // Video 모드 일 경우에 사용할 vedio capure 객체
    cv::VideoCapture m_videoInputCap;

    // 현재 재생되고 있는 video에 대한 정보들
    QString current_video_name;
    QString current_video_length;

    // Video Mode에서 불러온 비디오 경로들
    QStringList video_list = {};

    // brn show prediction 버튼
    bool show_pred_flag = true;

    // SQLite Databse
    std::shared_ptr<sqliteDB> m_db = std::make_shared<sqliteDB>();

    // NrtExe class
    std::shared_ptr<NrtExe> m_nrt;
    std::shared_ptr<NrtExe> m_nrt_ensmble;
    static const int INF_MODE_NONE = -1;
    static const int INF_MODE_SINGLE = 0;
    static const int INF_MODE_ENSEMBLE = 2;
    int inf_mode_status = INF_MODE_NONE;

    // MEDIA_MODE flag: 현재 어떤 media type을 input을 받고 있는가.
    static const int MEDIA_MODE_NONE = -1;
    static const int MEDIA_MODE_CAM = 0;
    static const int MEDIA_MODE_CAM_FOLDER = 1;
    static const int MEDIA_MODE_VIDEO = 2;
    static const int MEDIA_MODE_IMAGE = 3;
    int media_mode = MEDIA_MODE_NONE;

    // CAM FOLDER 모드
    QFileSystemWatcher* file_sys_watcher;
    QMap<QString, QStringList> currentContentsMap;
    QStringList new_img_buff = {};

    // Model1 의 Chart
    QPieSeries* series_model1 = new QPieSeries;
    QChart* chart_model1 = new QChart;
    QVector<int> class_ratio_model1;
    QVector<float> avg_scores_model1;

    // Model2의 Chart
    QPieSeries* series_model2 = new QPieSeries;
    QChart* chart_model2 = new QChart;
    QVector<int> class_ratio_model2;
    QVector<float> avg_scores_model2;

    // Ensemble: Model1의 class들이 각각 Model2를 trigger 하는 것인지 나타내는 map
    QVector<bool> model1_class_trigger_map;

    // Ensemble 중 Seg/Det->Cla 옵션: 이미지 crop size (crop size가 원본 이미지 크기를 넘어가는 경우, 원본 이미지 크기를 crop size로 다시 지정: 각 ensSetResult 함수 참고)
    int crop_h = 128;
    int crop_w = 128;

    // Image 모드: inference할 이미지들의 list
    QStringList inf_image_list = {};
    int cur_inf_img_idx;

    // 모든 media mode에서 공통으로 사용하는 save flag
    QString root_path;
    QString current_save_path; // 날짜를 단위로 yyyy-MM-dd / MM-dd가 기본 current save path이나, 유저가 다른 경로 혹은 sub경로를 지정할 수 있음
    Share_Mat m_save_info;
    int currentModelId;
    int ensembleModelId;

    // Save를 할 때, 관련 flag들
    bool save_flag = false;
    int frame_interval = 1; // video 모드에서는 모든 frame이 아니라, N번째 frame만 저장하도록 지정하여 중간 결과를 확인하는 용도로 사용할 수 있음.
    int inferenced_images = 0; // 지금까지 inference 된 이미지들의 개수

    // Show를 위한 flag들
    int update_result_table_rate;
    bool random_shuffle_flag = false;
    bool repeat_flag = false;
    double show_time;
    bool result_show_flag;

    // 이미지를 저장할 때, 이미지 명에 포함할 정보들
    QString src_prefix = ""; // CAM, IMAGE, 비디오 이름 등 사용자가 이미지 명 앞에 붙이고 싶은 prefix
    bool class_include_flag = false; // class 정보를 이미지 명에 포함?

    // Detection에서 Actual column을 사용할 지를 나타내는 flag
    bool detect_count_flag = false;

    // Model을 선택하고 Executor 까지 생성 완료되었는 지 나타내는 flag
    bool model_ready_for_inf = false;
    bool inference_flag = false;

    QDateTime inf_start_time;

    // 전체화면 뷰어 버튼
    QPushButton* btn_full_screen_viewer;
    QDialog* full_screen_dialog = new QDialog;
    QLabel* lab_full_screen = new QLabel("Image");

    // show prediction 버튼
    QPushButton* btn_show_prediction;
    QMetaObject::Connection btn_show_pred_connection;

    // loader.gif
    QMovie* movie;
    QMetaObject::Connection movie_connection;

    // result item vector
    int cur_result_img_idx = -1;
    QVector<cv::Mat> org_images;
    QVector<cv::Mat> result_images;
};

#endif // MAINWINDOW_H
