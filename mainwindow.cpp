#include "mainwindow.h"
#include "ui_mainwindow.h"

#define TERM_MIN 0.0
#define TERM_MAX 60.0

const string PathSeparator = QString(QDir::separator()).toStdString();
/*
#ifdef _WIN32
                            "\\";
#else
                            "/";
#endif
*/
const string IMG_FORMAT = ".png";
const string ORG_FOL = "org" + PathSeparator;
const string PRED_FOL = "pred" + PathSeparator;

const string VIDEO_TEXT = "Video File";
const string CAMERA_TEXT = "Relatime Camera";

// Database file
static const char* nrtDBName = "neuroe.db";

QVector<QColor> COLOR_VECTOR;

extern int PROB_IDX, PRED_IDX, CAM_IDX, ANO_IDX;

int COLOR_COL = 0, NAME_COL = 1, PROB_THRES_COL = 5, WIDTH_COL = 2, HEIGHT_COL = 4, AND_OR_COL = 3;
cv::Scalar red(0, 0, 255);
cv::Scalar green(0, 255, 0);
cv::Scalar blue(255, 0, 0);
cv::Scalar white(255, 255, 255);
cv::Scalar black(0, 0, 0);

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Neuro-E");

    // Text Size
    QSize availableSize = QGuiApplication::screens().at(0)->availableSize();
    int s_width = availableSize.width();
    int s_height = availableSize.height();
    /* For 2020 KOREA MAT
    if (s_width > 2000) {
        QFont m_font = ui->lab_show_class->font();
        m_font.setPointSize(m_font.pointSize()*2);
        ui->lab_show_class->setFont(m_font);
        ui->lab_show_inf->setFont(m_font);
        m_font = ui->edit_show_class->font();
        m_font.setPointSize(m_font.pointSize()*3);
        ui->edit_show_class->setFont(m_font);
        ui->edit_show_inf->setFont(m_font);
        m_font = ui->btn_viewfilter->font();
        m_font.setPointSize(m_font.pointSize()*2);
        ui->btn_viewfilter->setFont(m_font);

        m_font = ui->com_cam_select->font();
        m_font.setPointSize(m_font.pointSize()*3);
        ui->com_cam_select->setFont(m_font);
        ui->btn_cam_select->setFont(m_font);
        ui->rad_cam_rtmode->setFont(m_font);
        ui->rad_cam_savemode->setFont(m_font);
        ui->edit_cam_save_term->setFont(m_font);
        ui->lab_cam_save_term->setFont(m_font);
        ui->chb_show_prediction->setFont(m_font);

        m_font = ui->lab_model_topbar->font();
        m_font.setPointSize(m_font.pointSize()*3);
        ui->lab_model_topbar->setFont(m_font);
        m_font = ui->btn_select_model->font();
        m_font.setPointSize(m_font.pointSize()*3);
        ui->btn_select_model->setFont(m_font);
        ui->cbx_select_fp16->setFont(m_font);
        ui->btn_select_gpu->setFont(m_font);

        m_font = ui->lab_pro_device->font();
        m_font.setPointSize(m_font.pointSize()*3);
        ui->lab_pro_device->setFont(m_font);
        ui->lab_pro_name->setFont(m_font);
        ui->lab_pro_type->setFont(m_font);
        ui->lab_pro_trainingtype->setFont(m_font);
        ui->lab_pro_search->setFont(m_font);
        ui->lab_pro_inference->setFont(m_font);
        ui->lab_device->setFont(m_font);
        ui->lab_model_name->setFont(m_font);
        ui->lab_model_type->setFont(m_font);
        ui->lab_model_trainingtype->setFont(m_font);
        ui->lab_model_search->setFont(m_font);
        ui->lab_model_inference->setFont(m_font);
        ui->lab_class_list->setFont(m_font);

        m_font = ui->tableWidget_class->font();
        m_font.setPointSize(m_font.pointSize()*2);
        ui->tableWidget_class->setFont(m_font);
    }
    */

    // TopBar Style

    ui->btn_topbar_menu->setIcon(QIcon(":/icons/menu_tab.png"));
    ui->btn_topbar_menu->setToolTip("Menu");
    ui->btn_topbar_menu->setIconSize(QSize(ui->btn_topbar_menu->width()*0.5, ui->btn_topbar_menu->height()*0.5));
    ui->TopBar->setStyleSheet("background-color: rgb(54, 93, 157)");
    ui->edit_topbar_title->setStyleSheet("QLineEdit { border: none; color: rgb(255, 255, 255); }");

    // Show Style
    ui->btn_show_prediction->setIcon(QIcon(":/icons/show.png"));
    ui->btn_show_prediction->setToolTip("Show Prediction");
    ui->btn_show_prediction->setIconSize(QSize(ui->btn_show_prediction->width()*0.5, ui->btn_show_prediction->height()*0.5));
    ui->btn_view_eval_stats->setIcon(QIcon(":/icons/table.png"));
    ui->btn_view_eval_stats->setToolTip("Show Result Table");
    ui->btn_display_eval_results->setIconSize(QSize(ui->btn_display_eval_results->width()*0.5, ui->btn_display_eval_results->height()*0.5));
    ui->btn_display_eval_results->setIcon(QIcon(":/icons/save.png"));
    ui->btn_display_eval_results->setToolTip("Save Result Table");
    ui->btn_display_eval_results->setIconSize(QSize(ui->btn_display_eval_results->width()*0.5, ui->btn_display_eval_results->height()*0.5));
    ui->lab_result_info->setText("There is no table to show.");
    ui->gridLayout->removeWidget(ui->Show_Res_Table);
    ui->gridLayout->addWidget(ui->Show_Res_Table, 0, 0, Qt::AlignRight);
    ui->tableWidget_result->setEditTriggers(QAbstractItemView::NoEditTriggers);
    ui->Show_Res_Table->hide();
    ui->Show_Res_Table->setStyleSheet("background-color: rgb(255, 255, 255)");
    ui->lab_show_res->setAlignment(Qt::AlignCenter);

    // CAM Icon
    ui->btn_cam_mode->setIcon(QIcon(":/icons/cam_tab.png"));
    ui->btn_cam_mode->setToolTip("CAM Mode");
    ui->btn_cam_mode->setIconSize(QSize(ui->btn_cam_mode->width()*1.2, ui->btn_cam_mode->height()*1.2));
    ui->btn_cam_play->setIcon(QIcon(":/icons/cam_play.png"));
    ui->btn_cam_play->setIconSize(QSize(ui->btn_cam_play->height()*0.45, ui->btn_cam_play->height()*0.45));
    ui->btn_cam_pause->setIcon(QIcon(":/icons/cam_pause.png"));
    ui->btn_cam_pause->setIconSize(QSize(ui->btn_cam_pause->height()*0.45, ui->btn_cam_pause->height()*0.45));
    ui->btn_cam_stop->setIcon(QIcon(":/icons/cam_stop.png"));
    ui->btn_cam_stop->setIconSize(QSize(ui->btn_cam_stop->height()*0.45, ui->btn_cam_stop->height()*0.45));
    setCamControlEnabled(false);

    // CAM Select ComboBox
    camType.append(QString::fromStdString(CAMERA_TEXT));
    camType.append(QString::fromStdString(VIDEO_TEXT));
    ui->com_cam_input_select->addItems(camType);
    if(ui->com_cam_input_select->currentText().toStdString() == VIDEO_TEXT){
        ui->btn_cam_select->setText("Select Video File");
    }
    else if(ui->com_cam_input_select->currentText().toStdString() == CAMERA_TEXT){
        ui->btn_cam_select->setText("Select Camera");
    }

    // CAM Save Style
    ui->rad_cam_rtmode->setChecked(true);

    // IMG Mode Style
    setImgShowTimeEditEnable(false);

    // IMG Icon
    ui->btn_img_mode->setIcon(QIcon(":/icons/img_tab.png"));
    ui->btn_img_mode->setToolTip("IMG Mode");
    ui->btn_img_mode->setIconSize(ui->btn_img_mode->size());

    ui->edit_img_name->setReadOnly(true);
    ui->edit_set_show_time->setValidator(new QDoubleValidator(TERM_MIN, TERM_MAX, 1, this));

//    ui->spb_img_cur_idx->setReadOnly(true);
    ui->btn_img_play->setIcon(QIcon(":/icons/cam_play.png"));
    ui->btn_img_play->setIconSize(QSize(ui->btn_img_play->height()*0.45, ui->btn_img_play->height()*0.45));
    ui->btn_img_pause->setIcon(QIcon(":/icons/cam_pause.png"));
    ui->btn_img_pause->setIconSize(QSize(ui->btn_img_pause->height()*0.45, ui->btn_img_pause->height()*0.45));
    ui->btn_img_pause->setEnabled(false);
    ui->btn_img_stop->setIcon(QIcon(":/icons/cam_stop.png"));
    ui->btn_img_stop->setIconSize(QSize(ui->btn_img_stop->height()*0.45, ui->btn_img_stop->height()*0.45));
    ui->btn_img_result->setIcon(QIcon(":/icons/img_res.png"));
    ui->btn_img_result->setIconSize(QSize(ui->btn_img_result->height()*0.8, ui->btn_img_result->height()*0.8));
    ui->btn_img_result->setEnabled(false);

    // Model Property Style
    ui->cbx_select_fp16->setToolTip("Slower when creating Executor, but Faster when predicting Img");
    ui->cbx_select_fp16->setWhatsThis("Slower when creating Executor, but Faster when predicting Img");
    ui->lab_model_status->setAlignment(Qt::AlignCenter);
    setModelInfo(false, "");

    QSqlError err = m_db->InitialDBSetup();
    if(err.type() != QSqlError::NoError){
        qDebug() << "Initial DB Setup failed: " << err;
        m_db.reset();
    }
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::center_and_resize()
{
    QSize availableSize = QGuiApplication::screens().at(0)->availableSize();
    int width = availableSize.width() * 0.8;
    int height = availableSize.height() * 0.8;
    QSize newSize(width, height);

    setGeometry(
        QStyle::alignedRect(
            Qt::LeftToRight,
            Qt::AlignCenter,
            newSize,
            QGuiApplication::screens().at(0)->geometry()
        )
    );
}

void MainWindow::paintEvent(QPaintEvent *paintEvent)
{
    QPainter painter(this);

    // Border Paint
    QList<QWidget*> specWidgets;
    specWidgets.append(ui->Show);
    specWidgets.append(ui->Info_Mode);
    specWidgets.append(ui->Model_Proper_Class);

    int MARGIN = 1;
    painter.setPen(QColor(0, 0, 0, 100));
    for (const QWidget *borderWidget : specWidgets) {
        QRect rect(borderWidget->mapTo(ui->centralWidget, QPoint(-MARGIN,-MARGIN)),
                    borderWidget->size() + QSize(MARGIN*2, MARGIN*2));
        painter.fillRect(rect, QColor(255,255,255));
        painter.drawRoundedRect(rect, 5, 5);
    }

    QMainWindow::paintEvent(paintEvent);
}

void MainWindow::setCamSelectEnabled(bool flag) {
    ui->com_cam_input_select->setEnabled(flag);
    ui->btn_cam_select->setEnabled(flag);
}

void MainWindow::setCamControlEnabled(bool flag) {
    ui->btn_cam_play->setEnabled(flag);
    ui->btn_cam_pause->setEnabled(flag);
    ui->btn_cam_stop->setEnabled(flag);
}

void MainWindow::setCamSaveEditEnabled(bool flag) {
    ui->rad_cam_rtmode->setEnabled(flag);
    ui->rad_cam_autosave->setEnabled(flag);
    ui->rad_cam_mansave->setEnabled(flag);
//    ui->rad_cam_savemode->setEnabled(flag);
//    ui->edit_cam_save_term->setEnabled(flag);
//    ui->edit_cam_save->setEnabled(flag);
//    ui->btn_cam_save->setEnabled(flag);
//    if (flag)
//        ui->edit_cam_save->setText("");
}

void MainWindow::setCamSaveChangeEnabled(bool flag) {  // Can be Changed Part when Paused
    ui->rad_cam_rtmode->setEnabled(flag);
//    ui->rad_cam_savemode->setEnabled(flag);
//    ui->edit_cam_save_term->setEnabled(flag);
//    if (flag) {
//        if (ui->edit_cam_save->text() == NULL) {
//            ui->edit_cam_save->setEnabled(flag);
//            ui->btn_cam_save->setEnabled(flag);
//        }
//    }
//    else {
//        ui->edit_cam_save->setEnabled(flag);
//        ui->btn_cam_save->setEnabled(flag);
//    }
}

bool MainWindow::checkCanSave() {
    QMessageBox *err_msg = new QMessageBox;
    err_msg->setFixedSize(600, 400);
//    if (ui->edit_cam_save->text() == NULL) {
//        err_msg->critical(0,"Error", "Please Choose Folder to be Saved.");
//        on_btn_cam_save_clicked();
//        return false;
//    }
//    if (ui->edit_cam_save_term->text() == NULL) {
//        err_msg->critical(0,"Error", "Please Input How Often(sec) will you Save Images.");
//        ui->edit_cam_save_term->setFocus();
//        return false;
//    }

    // check if the save term is set
    // check if the save path is set
    delete err_msg;
    return true;
}

/*** IMG Setting Method (ISM) ***/

void MainWindow::setImgShowTimeEditEnable(bool flag) {
    ui->edit_set_show_time->setEnabled(flag);
    ui->lab_set_show_time->setEnabled(flag);
}

void MainWindow::setImgInference(bool flag) {
    ui->btn_img_input->setEnabled(!flag);
    ui->edit_img_input->setEnabled(!flag);
    ui->btn_img_output->setEnabled(!flag);
    ui->edit_img_output->setEnabled(!flag);
    ui->cbx_set_show_time->setEnabled(!flag);
    setImgShowTimeEditEnable(!flag);

    ui->spb_img_cur_idx->setReadOnly(flag);
    ui->btn_img_pause->setEnabled(flag);
    ui->btn_img_stop->setEnabled(flag);
    if (!flag)
        setImgResultShow(false);
}

void MainWindow::setImgResultShow(bool flag) {
    // setImageResultShow(true) -> set the img index spinbox to Write Available, image result buttoned Enabled
    ui->spb_img_cur_idx->setReadOnly(!flag);
    ui->btn_img_result->setEnabled(flag);
}

/*** Butttons that locate above result image ***/

void MainWindow::on_btn_show_prediction_clicked()
{
    if (!show_pred_flag) {  // Unshow -> Show
        show_pred_flag = true;
        ui->btn_show_prediction->setIcon(QIcon(":/icons/show.png"));
    }
    else {                  // Show -> Unshow
        show_pred_flag = false;
        ui->btn_show_prediction->setIcon(QIcon(":/icons/unshow.png"));
    }

    if (m_timer.use_count() > 0) {  // Inferencing
        if (!mode_flag) {   // CAM Mode
            qDebug() << "CAM Mode) Show Prediction Icon Clicked";
        }
        else {              // IMG Mode
            qDebug() << "IMG Mode) Show Prediction Icon Clicked";
            //showResult();

            int cur_inf_img_idx = ui->spb_img_cur_idx->value();
            if (cur_inf_img_idx < 1 || cur_inf_img_idx > inf_img_list.size())
                return;
            std::string cur_inf_img_path = inf_img_list[cur_inf_img_idx-1].toStdString();
            int pre_idx = ui->edit_img_input->text().size() + 1;
            std::string img_name = cur_inf_img_path.substr(pre_idx);
            std::string save_path = m_save_info.save_path;
            std::string org_img_path = save_path + PathSeparator + ORG_FOL + img_name;
            std::string pred_img_path = save_path + PathSeparator + PRED_FOL + img_name;
            qDebug() << QString::fromStdString(org_img_path) << QString::fromStdString(pred_img_path);

            cv::Mat ORG_IMG = cv::imread(org_img_path, cv::IMREAD_COLOR);
            cv::Mat PRED_IMG = cv::imread(pred_img_path, cv::IMREAD_COLOR);
            if (ORG_IMG.empty() || PRED_IMG.empty())
                return;

            cv::cvtColor(ORG_IMG, ORG_IMG, COLOR_RGB2BGR);
            cv::cvtColor(PRED_IMG, PRED_IMG, COLOR_RGB2BGR);
            QImage m_qimage;
            if (!show_pred_flag)
                m_qimage = QImage((const unsigned char*) (ORG_IMG.data), ORG_IMG.cols, ORG_IMG.rows, ORG_IMG.step, QImage::Format_RGB888);
            else
                m_qimage = QImage((const unsigned char*) (PRED_IMG.data), PRED_IMG.cols, PRED_IMG.rows, PRED_IMG.step, QImage::Format_RGB888);
            QPixmap m_qpixmap = QPixmap::fromImage(m_qimage);
            ui->lab_show_res->setPixmap(m_qpixmap.scaled(ui->lab_show_res->width(), ui->lab_show_res->height(), Qt::KeepAspectRatio));
            qDebug() << "IMG Mode) IMG Change";
        }
    }
}


void MainWindow::on_btn_show_img_result_clicked()
{
    if (show_result_table) {    // Show -> Unshow
        show_result_table = false;
        ui->btn_view_eval_stats->setIcon(QIcon(":/icons/table.png"));
        ui->btn_view_eval_stats->setDefault(false);
        ui->btn_view_eval_stats->setToolTip("Show Result Table");

        ui->Show_Res_Table->hide();

        ui->lab_show_res->setSizePolicy(QSizePolicy::Preferred, QSizePolicy::Preferred);
        ui->lab_show_res->setSizeIncrement(ui->Show->width(), ui->Show->height());
    }
    else {                      // Unshow -> Show
        show_result_table = true;
        ui->btn_view_eval_stats->setIcon(QIcon(":/icons/untable.png"));
        ui->btn_view_eval_stats->setDefault(true);
        ui->btn_view_eval_stats->setToolTip("Unshow Result Table");

        ui->gridLayout->removeWidget(ui->Show_Res_Table);
        ui->Show_Res_Table->setMinimumWidth((int)(ui->Show->width()/2));
        ui->gridLayout->addWidget(ui->Show_Res_Table, 0, 0, Qt::AlignRight);
        ui->Show_Res_Table->setSizeIncrement((int)(ui->Show->width()/2), ui->Show->height());
        ui->Show_Res_Table->show();
    }
}

void MainWindow::setTabelColumn(bool flag) {
    if (!flag) {
        ui->lab_result_info->setText("There is no table to show.");
        ui->tableWidget_result->clear();
        ui->tableWidget_result->setRowCount(0);
        ui->tableWidget_result->setColumnCount(0);
        return;
    }

    ui->tableWidget_result->clear();
    if (get_model_status() == nrt::STATUS_SUCCESS) {
        QString model_type = get_model_type();
        int COLOR_COL = 0, NAME_COL = 1;
        int START_POINT;
        if (model_type == "Classification" || model_type == "Anomaly")
            START_POINT = 0;
        else if (model_type == "Segmentation" || model_type == "Detection")
            START_POINT = 1;

        // Set Horizental Header
        ui->tableWidget_result->horizontalHeader()->show();
        QStringList horHeaderLabels;
        //QStringList horHeaderLabels = QStringList() << "Image";

        int num_classes;
        if (model_type == "Classification") {
            num_classes = get_model_class_num();
            ui->lab_result_info->setText("* CLA Info: (%) - Probabilty of each class");
            ui->tableWidget_result->setColumnCount(num_classes - START_POINT + 1);
            horHeaderLabels.append(QString("Class"));
            for (int cla_idx = START_POINT; cla_idx < num_classes; cla_idx++)
                horHeaderLabels.append((get_model_class_name(cla_idx) + QString("(%)")));
        }
        else if (model_type == "Segmentation") {
            num_classes = get_model_class_num();
            ui->lab_result_info->setText("* SEG Info: (#) - Number of blobs, (Pixel) - % of pixel area");
            ui->tableWidget_result->setColumnCount((num_classes - START_POINT) * 2);
            for (int cla_idx = START_POINT; cla_idx < num_classes; cla_idx++)
                horHeaderLabels.append((get_model_class_name(cla_idx) + QString("(#)")));
            for (int cla_idx = START_POINT; cla_idx < num_classes; cla_idx++)
                horHeaderLabels.append((get_model_class_name(cla_idx) + QString("(Pixel)")));
        }
        else if (model_type == "Detection") {
            num_classes = get_model_class_num();
            ui->lab_result_info->setText("* DET Info: (#) - Number of Objects, (%) - Probability Mean of each class");
            ui->tableWidget_result->setColumnCount((num_classes - START_POINT) * 2);
            for (int cla_idx = START_POINT; cla_idx < num_classes; cla_idx++)
                horHeaderLabels.append((get_model_class_name(cla_idx) + QString("(#)")));
            for (int cla_idx = START_POINT; cla_idx < num_classes; cla_idx++)
                horHeaderLabels.append((get_model_class_name(cla_idx) + QString("(%)")));
        }
        else if (model_type == "OCR") {
            ui->lab_result_info->setText("* OCR Info: Characters were appeared with left to right aligned.");
            ui->tableWidget_result->setColumnCount(1);
            horHeaderLabels.append(QString("Characters"));
        }
        else if (model_type == "Anomaly") {
            ui->lab_result_info->setText("* ANO Info: Do not support.");
        }
        else {
            qDebug() << "Set class table error! (Model Type Error)";
            return;
        }
        ui->tableWidget_result->setHorizontalHeaderLabels(horHeaderLabels);
        ui->tableWidget_result->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
        connect(ui->tableWidget_result, SIGNAL(cellClicked(int,int)), this, SLOT(resultItemClicked(int,int)));
        connect(ui->tableWidget_result->verticalHeader(), SIGNAL(sectionClicked(int)), this, SLOT(resultVerClicked(int)));
    }
}

void MainWindow::resultItemClicked(int row, int col) {
    resultVerClicked(row);
}

void MainWindow::resultVerClicked(int row) {
    if (mode_flag) {    // IMG Mode
        ui->spb_img_cur_idx->setValue(row+1);
        return;
    }

    QString cur_img_name = ui->tableWidget_result->verticalHeaderItem(row)->text();
//    QString output_fol = (!mode_flag ? ui->edit_cam_save->text() : ui->edit_img_output->text());
    QString output_fol = (!mode_flag ? "HAHA" : ui->edit_img_output->text());
    std::string cur_img_path = output_fol.toStdString() + PathSeparator + PRED_FOL + cur_img_name.toStdString();
    cv::Mat cur_img = cv::imread(cur_img_path, cv::IMREAD_COLOR);
    if (cur_img.empty())
        return;
    cv::cvtColor(cur_img, cur_img, COLOR_RGB2BGR);
    QImage m_qimage = QImage((const unsigned char*) (cur_img.data), cur_img.cols, cur_img.rows, cur_img.step, QImage::Format_RGB888);
    QPixmap m_qpixmap = QPixmap::fromImage(m_qimage);
    ui->lab_show_res->setPixmap(m_qpixmap.scaled(ui->lab_show_res->width(), ui->lab_show_res->height(), Qt::KeepAspectRatio));
}

//void MainWindow::on_btn_save_img_result_clicked() {
//    QMessageBox *err_msg = new QMessageBox;
//    err_msg->setFixedSize(600, 400);
//    if (get_model_status() == nrt::STATUS_SUCCESS) {
//        if ((!mode_flag && cam_save_flag && !ui->edit_cam_save->text().isNull()) || (mode_flag && !ui->edit_img_output->text().isNull())) {
//            std::unique_lock<std::mutex> lock(m_save_info._mutex);
//            m_save_info.save_csv_flag = true;
//            return;
//        }
//        else {
//            err_msg->critical(0,"Error", "There is no output folder path.");
//            return;
//        }
//    }
//    else {
//        err_msg->critical(0,"Error", "There are no model and result table.");
//        return;
//    }
//}

/*** Model Info Setting Method ***/

void MainWindow::setModelInfo(bool flag, QString model_name) {
    // Model Info를 지우고 Please Select Model 을 표시하고 싶을 때
    if (flag == false) {
        // 0: Model Info Page, 1: Model Status Page
        if ( ui->Model_Proper->currentIndex() == 0 ) {  // Info Page -> Status Page
            ui->Model_Proper->setCurrentWidget(ui->Model_Status_Page);
            ui->lab_model_status->setText("Please Select Model.");
        }
        return;
    }

    // Model Info를 표시하고 싶을 때 -> model 상태 확인
    else if (get_model_status() == nrt::STATUS_SUCCESS) {
        if( ui->Model_Proper->currentIndex() == 1 ) { // Status Page -> Info Page
            ui->Model_Proper->setCurrentWidget(ui->Model_Info_Page);
        }

        ui->lab_device->setText(get_gpu_name());
        // Model Name Issue
        ui->lab_model_name->setText(model_name);
        ui->lab_model_type ->setText(get_model_type());
        ui->lab_model_trainingtype->setText(get_model_training_type());
        ui->lab_model_search->setText(get_model_search_level());

        // Fast가 아닐 때만 ui에 inference speed 표시
        if (get_model_search_level() != QString("Fast")) {
            ui->Proper_Inference->show();
            ui->lab_model_inference->setText(get_model_inference_level());
        }
        else
            ui->Proper_Inference->hide();

        // Class Table
        setClassTable(flag);
    }

    class_table_availbale = true;
}

void MainWindow::setClassTable(bool flag) {
    // tableWidget_class: 오른쪽 하단 class 보여주는 table
    disconnect(ui->tableWidget_class, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(tableItemClicked(int,int)));
    disconnect(ui->tableWidget_class, SIGNAL(cellChanged(int,int)), this, SLOT(tableItemChanged(int,int)));

    ui->tableWidget_class->clear();
    ui->tableWidget_class->horizontalHeader()->hide();

    if (!flag)
        return;

    QString model_type = get_model_type();
    int COLOR_COL = 0, NAME_COL = 1;
    int START_POINT;
    if (model_type == "Classification" || model_type == "Anomaly")
        START_POINT = 0;
    else if (model_type == "Segmentation" || model_type == "Detection")
        START_POINT = 1;

    // Set Horizental Header
    ui->tableWidget_class->verticalHeader()->hide();
    ui->tableWidget_class->horizontalHeader()->show();
    if (model_type != "OCR")
        ui->edit_show_class->setFont(QFont("Ubuntu", 23, 1, false));
    QStringList horHeaderLabels;
    if (model_type == "Classification") {
        ui->tableWidget_class->setRowCount(get_model_class_num() - START_POINT);
        ui->tableWidget_class->setColumnCount(3);
        horHeaderLabels = QStringList() << "Color" << "Name" << "Prob. Thres.";
        ui->tableWidget_class->setHorizontalHeaderLabels(horHeaderLabels);
    }
    else if (model_type == "Segmentation") {
        ui->tableWidget_class->setRowCount(get_model_class_num() - START_POINT);
        ui->tableWidget_class->setColumnCount(6);
        horHeaderLabels = QStringList() << "Color" << "Name" << "Width" << "AND/OR" << "Height" << "Prob. Thres.";
        ui->tableWidget_class->setHorizontalHeaderLabels(horHeaderLabels);
    }
    else if (model_type == "Detection") {
        ui->edit_show_class->setFont(QFont("Ubuntu", 14, 2, false));
        ui->tableWidget_class->setRowCount(get_model_class_num() - START_POINT);
        ui->tableWidget_class->setColumnCount(6);
        horHeaderLabels = QStringList() << "Color" << "Name" << "Width" << "AND/OR" << "Height" << "Prob. Thres";
        ui->tableWidget_class->setHorizontalHeaderLabels(horHeaderLabels);
    }
    else if (model_type == "OCR") {
    }
    else if (model_type == "Anomaly") {
        ui->tableWidget_class->setRowCount(get_model_class_num() - START_POINT);
        ui->tableWidget_class->setColumnCount(3);
        horHeaderLabels = QStringList() << "Color" << "Name" << "Sensitivity";
        ui->tableWidget_class->setHorizontalHeaderLabels(horHeaderLabels);
    }
    else {
        qDebug() << "Set class table error! (Model Type Error)";
        return;
    }
    ui->tableWidget_class->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

    // Set Color Item (Column = 0)
    COLOR_VECTOR.clear();
    if (model_type == "Classification" || model_type == "Segmentation") {
        int row_cnt = ui->tableWidget_class->rowCount();
        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(0, 255);
        dis(gen);

        ui->tableWidget_class->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        for (int i = 0; i < row_cnt; i++) {
            QWidget* cWidget = new QWidget();
            QLabel* lab_color = new QLabel();
            QColor new_color = QColor(dis(gen), dis(gen), dis(gen));
            if (i == 0)
                new_color = QColor(255, 0, 0);
            QVariant cVariant = new_color;
            QString cString = cVariant.toString();
            lab_color->setStyleSheet("background-color:"+cString+";");
            lab_color->setAlignment(Qt::AlignTop | Qt::AlignLeft);
            lab_color->setContentsMargins(0,0,0,0);
            lab_color->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
            QHBoxLayout* cLayout = new QHBoxLayout(cWidget);
            cLayout->addWidget(lab_color);
            cLayout->setAlignment(Qt::AlignCenter);
            cLayout->setContentsMargins(30, 2, 30, 2);
            cWidget->setLayout(cLayout);
            ui->tableWidget_class->setCellWidget(i, COLOR_COL, cWidget);
            COLOR_VECTOR.append(new_color);
        }
    }
    else if (model_type == "Detection") {int row_cnt = ui->tableWidget_class->rowCount();
        ui->tableWidget_class->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Fixed);
        for (int i = 0; i < row_cnt; i++) {
            QWidget* cWidget = new QWidget();
            QLabel* lab_color = new QLabel();
            QColor new_color = QColor(0, 0, 0);
            if (i == 0) // green spray
                new_color = QColor(0, 204, 0);
            else if (i == 1) // purple
                new_color = QColor(153, 0, 255);
            else if (i == 2) // pill
                new_color = QColor(255, 0, 0);
            else if (i == 3) // white
                new_color = QColor(220, 220, 220);
            else if (i == 4) // pink
                new_color = QColor(255, 51, 153);
            else if (i == 5) // tissue
                new_color = QColor(0, 0, 0);
            else if (i == 6) // blue
                new_color = QColor(0, 0, 255);
            QVariant cVariant = new_color;
            QString cString = cVariant.toString();
            lab_color->setStyleSheet("background-color:"+cString+";");
            lab_color->setAlignment(Qt::AlignTop | Qt::AlignLeft);
            lab_color->setContentsMargins(0,0,0,0);
            lab_color->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
            QHBoxLayout* cLayout = new QHBoxLayout(cWidget);
            cLayout->addWidget(lab_color);
            cLayout->setAlignment(Qt::AlignCenter);
            cLayout->setContentsMargins(30, 2, 30, 2);
            cWidget->setLayout(cLayout);
            ui->tableWidget_class->setCellWidget(i, COLOR_COL, cWidget);
            COLOR_VECTOR.append(new_color);
        }
    }
    else if (model_type == "Anomaly") {
        for (int i = 0; i < 2; i++) {
            QWidget* cWidget = new QWidget();
            QLabel* lab_color = new QLabel();
            QColor new_color = ((i == 0) ? QColor(6, 101, 210) : QColor(244, 67, 54));
            QVariant cVariant = new_color;
            QString cString = cVariant.toString();
            lab_color->setStyleSheet("background-color:"+cString+";");
            lab_color->setAlignment(Qt::AlignTop | Qt::AlignLeft);
            lab_color->setContentsMargins(0,0,0,0);
            lab_color->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
            QHBoxLayout* cLayout = new QHBoxLayout(cWidget);
            cLayout->addWidget(lab_color);
            cLayout->setAlignment(Qt::AlignCenter);
            cLayout->setContentsMargins(30, 2, 30, 2);
            cWidget->setLayout(cLayout);
            ui->tableWidget_class->setCellWidget(i, COLOR_COL, cWidget);
            COLOR_VECTOR.append(new_color);
        }
    }
    // Set OCR Char Classes
    else if (model_type == "OCR") {
        int OCR_COL_SIZE = 10, cur_start_row = 0, row, col;
        ui->tableWidget_class->setRowCount(7);
        ui->tableWidget_class->setColumnCount(OCR_COL_SIZE);
        for (int i = 0; i <= 9; i++) {
            row = i / OCR_COL_SIZE;
            col = i % OCR_COL_SIZE;
            QTableWidgetItem *newItem = new QTableWidgetItem(0);
            newItem->setText(QString::number(i));
            newItem->setTextAlignment(Qt::AlignCenter);
            newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
            ui->tableWidget_class->setItem(cur_start_row+row, col, newItem);
        }
        cur_start_row += (9 / OCR_COL_SIZE) + 1;
        char alp = 'A';
        for (int i = 0; i < 26; i++) {
            row = i / OCR_COL_SIZE;
            col = i % OCR_COL_SIZE;
            QTableWidgetItem *newItem = new QTableWidgetItem(0);
            newItem->setText(QString(alp));
            newItem->setTextAlignment(Qt::AlignCenter);
            newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
            ui->tableWidget_class->setItem(cur_start_row+row, col, newItem);
            alp++;
        }
        cur_start_row += (26 / OCR_COL_SIZE) + 1;
        alp = 'a';
        for (int i = 0; i < 26; i++) {
            row = i / OCR_COL_SIZE;
            col = i % OCR_COL_SIZE;
            QTableWidgetItem *newItem = new QTableWidgetItem(0);
            newItem->setText(QString(alp));
            newItem->setTextAlignment(Qt::AlignCenter);
            newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
            ui->tableWidget_class->setItem(cur_start_row+row, col, newItem);
            alp++;
        }
        ui->tableWidget_class->setShowGrid(true);
        ui->tableWidget_class->resizeColumnsToContents();
        ui->tableWidget_class->horizontalHeader()->hide();
        ui->tableWidget_class->verticalHeader()->setSectionResizeMode(QHeaderView::Stretch);
//        ui->tableWidget_class->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    }

    // Set Name (Column = 1)
    if (model_type == "Classification" || model_type == "Segmentation" || model_type == "Detection" || model_type == "Anomaly") {
        for (int i = START_POINT; i < get_model_class_num(); i++) {
            QTableWidgetItem *newItem = new QTableWidgetItem(0);
            newItem->setText(get_model_class_name(i));
            newItem->setTextAlignment(Qt::AlignCenter);
            ui->tableWidget_class->setItem(i - START_POINT, NAME_COL, newItem);
        }
    }
    else if (model_type == "OCR") {
    }

    /*
    Set Size Threshold [Width, And/Or, Height] (Segmentation, Detection).
        - Classification, OCR, Anomaly models do not have size thresholds.
        - ui -> tableWidget_class
        - Columns: SIZE_THRES_WIDTH_COL, SIZE_THRES_CONJ_COL, SIZE_THRES_HEIGHT_COL
    */
    if (model_type == "Segmentation" || model_type == "Detection") {
        /*
        get_model_size_threshold()
            if size threshold was set during model training
            returns NDBuffer -> dims[0]: class index, dims[1]: threshold valus for class

            else
            returns empty NDBuffer
        */
        nrt::NDBuffer size_threshold_buf = get_model_size_threshold();
        nrt::Shape size_thres_shape = size_threshold_buf.get_shape();
        // size threshold가 있는 모델이면 해당 값으로 treshold 값을 ui에 적음.
        if (!size_threshold_buf.empty()) {
            // size_thres_shape.dims[0] : number of classes that has size threshold values
            for (int cls_idx = 0; cls_idx < size_thres_shape.dims[0]; cls_idx++) {
                int height = *size_threshold_buf.get_at_ptr<int>(cls_idx, 0);
                int width = *size_threshold_buf.get_at_ptr<int>(cls_idx, 1);
                int conjunction = *size_threshold_buf.get_at_ptr<int>(cls_idx, 2);

                for (int col = WIDTH_COL; col <= HEIGHT_COL; col++) {
                    if (col != AND_OR_COL) {
                        QTableWidgetItem *newItem = new QTableWidgetItem(0);
                        newItem->setText(QString::number(((col != WIDTH_COL) ? width : height)));
                        newItem->setTextAlignment(Qt::AlignCenter);
                        ui->tableWidget_class->setItem(cls_idx, col, newItem);
                    }
                    else {
                        /*
                        QWidget* cWidget = new QWidget();
                        QComboBox *condCB = new QComboBox;
                        condCB->setEditable(true);
                        condCB->lineEdit()->setReadOnly(true);
                        condCB->lineEdit()->setAlignment(Qt::AlignCenter);
                        condCB->addItem("and");
                        condCB->addItem("or");
                        for (int i = 0 ; i < condCB->count() ; ++i)
                            condCB->setItemData(i, Qt::AlignCenter, Qt::TextAlignmentRole);
                        condCB->setCurrentIndex((conjunction ? 1 : 0));
                        QHBoxLayout* cLayout = new QHBoxLayout(cWidget);
                        cLayout->addWidget(condCB);
                        cLayout->setAlignment(Qt::AlignCenter);
                        cLayout->setContentsMargins(10, 2, 10, 2);
                        cWidget->setLayout(cLayout);
                        ui->tableWidget_class->setCellWidget(i, col, cWidget);
                        */
                        QTableWidgetItem *newItem = new QTableWidgetItem(0);
                        newItem->setText((conjunction == 0 ? "and" : "or"));
                        newItem->setTextAlignment(Qt::AlignCenter);
                        newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
                        ui->tableWidget_class->setItem(cls_idx, col, newItem);
                    }
                }
            }
        }

        // size threshold가 없는 모델이면 0으로 값 초기화.
        else {
            qDebug() << "No Size Threshold";
            int row_cnt = ui->tableWidget_class->rowCount();
            for (int i = 0; i < row_cnt; i++) {
                for (int col = 2; col <= 4; col++) {
                    if (col != 3) {
                        QTableWidgetItem *newItem = new QTableWidgetItem(0);
                        newItem->setText(QString::number(0));
                        newItem->setTextAlignment(Qt::AlignCenter);
                        ui->tableWidget_class->setItem(i, col, newItem);
                    }
                    else {
                        /*
                        QWidget* cWidget = new QWidget();
                        QComboBox *condCB = new QComboBox;
                        condCB->setEditable(true);
                        condCB->lineEdit()->setReadOnly(true);
                        condCB->lineEdit()->setAlignment(Qt::AlignCenter);
                        condCB->addItem("and");
                        condCB->addItem("or");
                        for (int i = 0 ; i < condCB->count() ; ++i)
                            condCB->setItemData(i, Qt::AlignCenter, Qt::TextAlignmentRole);
                        QHBoxLayout* cLayout = new QHBoxLayout(cWidget);
                        cLayout->addWidget(condCB);
                        cLayout->setAlignment(Qt::AlignCenter);
                        cLayout->setContentsMargins(10, 2, 10, 2);
                        cWidget->setLayout(cLayout);
                        ui->tableWidget_class->setCellWidget(i, col, cWidget);
                        */
                        QTableWidgetItem *newItem = new QTableWidgetItem(0);
                        newItem->setText("and");
                        newItem->setTextAlignment(Qt::AlignCenter);
                        newItem->setFlags(newItem->flags() & ~Qt::ItemIsEditable);
                        ui->tableWidget_class->setItem(i, col, newItem);
                    }
                }
            }
        }
    }

    // Set Probability Threshold (CLA) (Column = 2)
    if (model_type == "Classification") {
        nrt::NDBuffer prob_thres = get_model_prob_threshold();
        nrt::Shape prob_thres_shape = prob_thres.get_shape();
        if (!prob_thres.empty()) {
            qDebug() << "CLA Probability Threshold";
            for (int i = 0; i < prob_thres_shape.dims[0]; i++) {
                float thres = *prob_thres.get_at_ptr<float>(i);
                QTableWidgetItem *newItem = new QTableWidgetItem(0);
                newItem->setText(QString::number(thres) + QString("%"));
                newItem->setTextAlignment(Qt::AlignCenter);
                ui->tableWidget_class->setItem(i, 2, newItem);
            }
        }
        else {
            qDebug() << "No Probability Threshold";
            int row_cnt = ui->tableWidget_class->rowCount();
            for (int i = 0; i < row_cnt; i++) {
                QTableWidgetItem *newItem = new QTableWidgetItem(0);
                newItem->setText(QString::number(0) + QString("%"));
                newItem->setTextAlignment(Qt::AlignCenter);
                ui->tableWidget_class->setItem(i, 2, newItem);
            }
        }
    }
    // Set Prob Threshold (SEG, DET) (Column = 5)
    if (model_type == "Segmentation" || model_type == "Detection") {
        int row_cnt = ui->tableWidget_class->rowCount();

        /*
        for (int i = 0; i < row_cnt; i++) {
            QWidget *pWidget = new QWidget();

            QDoubleSpinBox *probSB = new QDoubleSpinBox;
            probSB->setRange(0.00, 100.00);
            probSB->setValue(95.00);
            probSB->setSuffix("%");
            probSB->setDecimals(2);

            QHBoxLayout* pLayout = new QHBoxLayout(pWidget);
            pLayout->addWidget(probSB);
            pLayout->setAlignment(Qt::AlignCenter);
            pLayout->setContentsMargins(10, 2, 10, 2);

            pWidget->setLayout(pLayout);

            ui->tableWidget_class->setCellWidget(i, 5, pWidget);
        }
        */

        for (int i = 0; i < row_cnt; i++) {
            QTableWidgetItem *newItem = new QTableWidgetItem(0);
            newItem->setText(QString::number(0) + QString("%"));
            newItem->setTextAlignment(Qt::AlignCenter);
            ui->tableWidget_class->setItem(i, 5, newItem);
        }
    }

    // Set Sensitivity (ANO) (Column = 2)

    // connect(sender, SIGNAL(destroyed(QObject*)), this, SLOT(objectDestroyed(QObject*)));
    connect(ui->tableWidget_class, SIGNAL(cellClicked(int,int)), this, SLOT(tableItemClicked(int,int)));
    connect(ui->tableWidget_class, SIGNAL(cellDoubleClicked(int,int)), this, SLOT(tableItemClicked(int,int)));
    connect(ui->tableWidget_class, SIGNAL(cellChanged(int,int)), this, SLOT(tableItemChanged(int,int)));
}

void MainWindow::tableItemClicked(int row, int col) {
    on_btn_cam_pause_clicked();
    on_btn_img_pause_clicked();
    QString model_type = get_model_type();
    // Color Column = 0
    if (model_type != "OCR" && col == 0) {
        QColor new_color = QColorDialog::getColor(Qt::red, this);
        if (new_color.isValid()) {
            QWidget* cWidget = new QWidget();
            QLabel* lab_color = new QLabel();
            QVariant cVariant = new_color;
            QString cString = cVariant.toString();
            lab_color->setStyleSheet("background-color:"+cString+";");
            lab_color->setAlignment(Qt::AlignTop | Qt::AlignLeft);
            lab_color->setContentsMargins(0,0,0,0);
            lab_color->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
            QHBoxLayout* cLayout = new QHBoxLayout(cWidget);
            cLayout->addWidget(lab_color);
            cLayout->setAlignment(Qt::AlignCenter);
            cLayout->setContentsMargins(30, 2, 30, 2);
            cWidget->setLayout(cLayout);
            ui->tableWidget_class->setCellWidget(row, 0, cWidget);
            COLOR_VECTOR[row] = new_color;
        }
    }
    // And/Or (SEG, DET Column = 3)
    if ((model_type == "Segmentation" || model_type == "Detection") && col == 3) {
        QString cur_txt = ui->tableWidget_class->item(row, col)->text();
        ui->tableWidget_class->item(row, col)->setText((cur_txt == "and" ? "or" : "and"));
    }
}

void MainWindow::tableItemChanged(int row, int col) {
    // Detection Prob Edit
    QString model_type = get_model_type();
    if (((model_type == "Segmentation" || model_type == "Detection") && (col == 5)) || (model_type == "Classification" && (col == 2))) {
        QString input = ui->tableWidget_class->item(row, col)->text();
        if (input.at(input.length()-1) != QChar('%'))
            ui->tableWidget_class->item(row, col)->setText(input + QString("%"));
    }
}

/* Loop for Save Images */
void MainWindow::save_worker() {
    while (1) {
        std::unique_lock<std::mutex> lock(m_save_info._mutex);
        // Save Work
        Mat_With_Name org_mwn, pred_mwn;
        std::string save_path = m_save_info.save_path;
        std::string org_path, pred_path;
        vector<std::string> new_row;

        if (!m_save_info.org_buffer.empty()) {
            org_mwn = m_save_info.org_buffer.front();
            m_save_info.org_buffer.pop();

            org_path = save_path + PathSeparator + ORG_FOL + org_mwn.name;

            std::string::size_type pos = 0;
            while (pos != std::string::npos) {
                pos = org_mwn.name.find(PathSeparator, pos+1);
                QString new_fol_path = QString::fromStdString(save_path + PathSeparator + ORG_FOL +org_mwn.name.substr(0, pos+1));
                if (!QDir(new_fol_path).exists()) {
                    qDebug() << "Save Thread) Make Folder: [" << new_fol_path << "]";
                    QDir().mkdir(new_fol_path);
                }
            }

            cv::cvtColor(org_mwn.image, org_mwn.image, COLOR_RGB2BGR);
            cv::imwrite(org_path, org_mwn.image);
            qDebug() << "Save Thread)" << "[Save Complete]" << QString::fromStdString(org_path);
        }

        if (!m_save_info.pred_buffer.empty()) {
            pred_mwn = m_save_info.pred_buffer.front();
            m_save_info.pred_buffer.pop();
            pred_path = save_path + PathSeparator + PRED_FOL + pred_mwn.name;
            std::string::size_type pos = 0;
            while (pos != std::string::npos) {
                pos = pred_mwn.name.find(PathSeparator, pos+1);
                QString new_fol_path = QString::fromStdString(save_path + PathSeparator + PRED_FOL + pred_mwn.name.substr(0, pos+1));
                if (!QDir(new_fol_path).exists()) {
                    qDebug() << "Save Thread) Make Folder: [" << new_fol_path << "]";
                    QDir().mkdir(new_fol_path);
                }
            }

            cv::cvtColor(pred_mwn.image, pred_mwn.image, COLOR_RGB2BGR);
            cv::imwrite(pred_path, pred_mwn.image);
            qDebug() << "Save Thread)" << "[Save Complete]" << QString::fromStdString(pred_path);
        }

        if (!m_save_info.row_buffer.empty()) {
            new_row = m_save_info.row_buffer.front();
            m_save_info.row_buffer.pop();
            qDebug() << "Save Thread)" << "[New Table Row]" << QString::fromStdString(new_row[0]);
            ui->tableWidget_result->insertRow(ui->tableWidget_result->rowCount());
            qDebug() << ui->tableWidget_result->rowCount();
            int cur_row_idx = ui->tableWidget_result->rowCount() - 1;
            qDebug() << ui->tableWidget_result->rowCount() - 1;
            ui->tableWidget_result->setVerticalHeaderItem(cur_row_idx, new QTableWidgetItem(QString::fromStdString(new_row[0])));
            for (int i = 0; i < ui->tableWidget_result->columnCount(); i++) {
                std::string cur_item_str;
                if ((i+1) < new_row.size())
                    cur_item_str = new_row[i+1];
                else
                    cur_item_str = " ";
                ui->tableWidget_result->setItem(cur_row_idx, i, new QTableWidgetItem(QString::fromStdString(cur_item_str)));
            }
        }

        if (m_save_info.save_csv_flag) {
            qDebug() << "Save Thread) CSV Come";
            QString time_format = "yyMMdd_HHmm";
            QDateTime now = QDateTime::currentDateTime();
            QString time_str = now.toString(time_format);
            //QString file_name = (!mode_flag ? ui->edit_cam_save->text() : ui->edit_img_output->text()) + QString::fromStdString(PathSeparator) + get_model_name() + QString("_") + time_str + QString(".csv");
            QStringList m_n_split = ui->lab_model_name->text().split(QLatin1Char('.'));
            QString model_name("");
            for (int m_n_i = 0; m_n_i < m_n_split.size()-1; m_n_i++)
                model_name += m_n_split[m_n_i];
//            QString file_name = (!mode_flag ? ui->edit_cam_save->text() : ui->edit_img_output->text()) + QString::fromStdString(PathSeparator) + model_name + QString("_") + time_str + QString(".csv");
            QString file_name = "temp_name.csv";
            qDebug() << file_name;

            QFile m_csv(file_name);
            if (m_csv.open(QFile::WriteOnly | QFile::Truncate)) {
                QTextStream data(&m_csv);
                QStringList strList;
                strList << "\" \"";
                for (int c = 0; c < ui->tableWidget_result->columnCount(); c++) {
                    strList << "\""+ ui->tableWidget_result->horizontalHeaderItem(c)->data(Qt::DisplayRole).toString() +"\"";
                    //qDebug() << ui->tableWidget_result->horizontalHeaderItem(c)->data(Qt::DisplayRole).toString();
                }
                data << strList.join(";") + "\n";

                for (int r = 0; r < ui->tableWidget_result->rowCount(); r++) {
                    strList.clear();
                    strList << "\""+ ui->tableWidget_result->verticalHeaderItem(r)->text()+"\"";
                    for (int c = 0; c < ui->tableWidget_result->columnCount(); c++) {
                        strList << "\""+ ui->tableWidget_result->item(r, c)->text() +"\"";
                    }
                    data << strList.join(";") + "\n";
                }
                m_csv.close();
            }

            m_save_info.save_csv_flag = false;
            qDebug() << "Save Thread) [CSV Save Complete]" << file_name;
        }
    }
}

void MainWindow::setSaveStautus()
{
    cur_save_flag = true;
    qDebug() << "CAM Mode) Request to save";
}

void MainWindow::claSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row){
    nrt::Status status;

    nrt::NDBuffer output_cam;
    nrt::NDBuffer output_prob;
    nrt::NDBuffer output_pred;
    nrt::NDBuffer thresholded_pred;

    if (CAM_IDX != -1) {
        nrt::NDBuffer cam_colormap;
        output_cam = outputs.get_at(CAM_IDX);
        status = nrt::convert_to_colormap(output_cam, cam_colormap);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "convert_to_colormap failed. :" << QString(nrt::get_last_error_msg());
        }

        nrt::Shape color_map_shape = cam_colormap.get_shape();
        for (int j = 0; j < color_map_shape.dims[0]; j++) {
            cv::Mat cam_colormap_mat(
                    color_map_shape.dims[1],
                    color_map_shape.dims[2],
                    CV_8UC3,
                    (void*)(cam_colormap.get_data_ptr<char>() + (color_map_shape.dims[1] * color_map_shape.dims[2] * color_map_shape.dims[3]) * j));

            cv::resize(cam_colormap_mat, cam_colormap_mat, cv::Size(PRED_IMG.cols, PRED_IMG.rows), 0, 0, cv::INTER_LINEAR);
            cv::addWeighted(cam_colormap_mat, 0.4, PRED_IMG, 0.6, 0.0, PRED_IMG);
        }
    }

    vector<float> cls_prob_vec(get_model_class_num(), 0);
    if(PROB_IDX != -1) {
        output_prob = outputs.get_at(PROB_IDX);
        nrt::Shape output_prob_shape = output_prob.get_shape();

        for(int i=0; i < output_prob_shape.dims[0]; i++) {
            qDebug() << "probability value for each class : ";
            for (int j=0; j < output_prob_shape.dims[1]; j++) {
                float prob = *output_prob.get_at_ptr<float>(i, j);
                cls_prob_vec[j] = prob;
                qDebug() << get_model_class_name(j) << " - " << prob;
            }
        }
    }

    std::string pred_cls = "";
    if(PRED_IDX != -1) {
        output_pred = outputs.get_at(PRED_IDX);
        nrt::Shape output_pred_shape = output_pred.get_shape();

        int pred_cla_idx;
        for (int i = 0; i < output_pred_shape.dims[0]; i++) {
            pred_cla_idx = *output_pred.get_at_ptr<int>(i);
            qDebug() << "Prediction class index(Not thresholded): " << QString::number(pred_cla_idx);
        }
        QString pred_cla_name = get_model_class_name(pred_cla_idx);

        int num_idx = ui->tableWidget_class->item(pred_cla_idx, NAME_COL+1)->text().size() - 1;
        float cur_prob_thres = ui->tableWidget_class->item(pred_cla_idx, NAME_COL+1)->text().left(num_idx).toFloat();
        qDebug() << ui->tableWidget_class->item(pred_cla_idx, NAME_COL+1)->text().left(num_idx) << cur_prob_thres;

        if ((cls_prob_vec[pred_cla_idx] * 100) < cur_prob_thres)
            pred_cla_name = QString("Unknown");

        ui->edit_show_class->setText(pred_cla_name);

        pred_cls = pred_cla_name.toStdString();
    }

    // csv new row
    new_row.push_back(pred_cls);

    for (int cls_idx = 0; cls_idx < cls_prob_vec.size(); cls_idx++) {
        float cur_rate = cls_prob_vec[cls_idx] * 100;
        if (cur_rate == 0) {
            new_row.push_back(std::string("0 %"));
            continue;
        }
        std::ostringstream out;
        out.precision(2);
        out << std::fixed << cur_rate;
        new_row.push_back((out.str() + std::string(" %")));
    }
}

void MainWindow::segSetResults(nrt::NDBuffer merged_pred_output, cv::Mat &PRED_IMG, vector<std::string> &new_row){
    vector<int> seg_blob_cnt(get_model_class_num()-1);
    vector<float> seg_pixel_rate(get_model_class_num()-1, 0);
    vector<long long> seg_pixel_cnt(get_model_class_num()-1, 0);
    long long total_pixel;

    if (!merged_pred_output.empty()) {
        nrt::Status status;

        // Threshold by size
        nrt::NDBuffer bounding_rects;
        nrt::NDBuffer size_threshold_buf = get_model_size_threshold();

        if (size_threshold_buf.empty()) {
            size_threshold_buf = nrt::NDBuffer::zeros(nrt::Shape(get_model_class_num(), 3), nrt::DTYPE_INT32);
            int* thres_ptr = size_threshold_buf.get_at_ptr<int>();

            for (int i = 0; i < ui->tableWidget_class->rowCount(); i++) {
                if(ui->tableWidget_class->item(i, HEIGHT_COL) && ui->tableWidget_class->item(i, WIDTH_COL) && ui->tableWidget_class->item(i, AND_OR_COL)) {
                    thres_ptr[3*(i+1) + 0] = ui->tableWidget_class->item(i, HEIGHT_COL)->text().toInt();
                    thres_ptr[3*(i+1) + 1] = ui->tableWidget_class->item(i, WIDTH_COL)->text().toInt();
                    thres_ptr[3*(i+1) + 2] = (ui->tableWidget_class->item(i, AND_OR_COL)->text() == "and" ? 0 : 1);
                    //qDebug() << QString::number(thres_ptr[3*(i+1) + 0]) << QString::number(thres_ptr[3*(i+1) + 1]) << QString::number(thres_ptr[3*(i+1) + 2]);
                }
            }
        }
        status = nrt::pred_map_threshold_by_size(merged_pred_output, bounding_rects, size_threshold_buf, get_model_class_num());
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "pred_map_threshold_by_size failed.  : " << QString(nrt::get_last_error_msg());
        }

        /*
        nrt::NDBuffer prob_threshold = get_model_prob_threshold();
        if(prob_threshold.empty()){
            prob_threshold = nrt::NDBuffer::zeros(nrt::Shape(get_model_class_num()), nrt::DTYPE_FLOAT32);

            float* prob_thres_ptr = prob_threshold.get_at_ptr<float>();
            for(int i = 0; i < get_model_class_num(); i++) {
                prob_thres_ptr[i] = 0.95f;
                qDebug() << get_model_class_name(i) << " prob thres: " << QString::number(prob_thres_ptr[i]);
            }
        }
        */

        // Class Name in Result Image
        nrt::Shape bounding_rects_shape = bounding_rects.get_shape();
        for (int j = 0; j < bounding_rects_shape.dims[0]; j++) {
            int* output_rect_ptr = bounding_rects.get_at_ptr<int>(j);
            int image_batch_index = output_rect_ptr[0];
            int rect_x = output_rect_ptr[1];
            int rect_y = output_rect_ptr[2];
            int rect_h = output_rect_ptr[3];
            int rect_w = output_rect_ptr[4];
            int rect_class_index = output_rect_ptr[5];

            if (rect_class_index < 1 || rect_class_index > get_model_class_num()){
                continue;
            }

            seg_blob_cnt[rect_class_index-1] += 1;

            int r, g, b;

            COLOR_VECTOR[rect_class_index-1].getRgb(&r, &g, &b);

            cv::Scalar class_color_scalar = cv::Scalar(b, g, r);

            const char* classname = "";
            if(ui->tableWidget_class->item(rect_class_index-1, NAME_COL))
                classname = ui->tableWidget_class->item(rect_class_index-1, NAME_COL)->text().toStdString().c_str();

            if (classname) {
                //cv::putText(PRED_IMG, classname, cv::Point(rect_x, rect_y), FONT_HERSHEY_SIMPLEX, 1, white, 7);
                cv::putText(PRED_IMG, classname, cv::Point(rect_x, rect_y), FONT_HERSHEY_SIMPLEX, 1, class_color_scalar, 4);
            }
        }

        vector<bool> exist_class(get_model_class_num());
        // Class Color Pixel in Result Image;
        unsigned char* output_ptr = merged_pred_output.get_at_ptr<unsigned char>(0);
        int img_h = PRED_IMG.rows;
        int img_w = PRED_IMG.cols;
        total_pixel = img_h * img_w;
        int cur_ofs, cur_class;
        double alp_src = 0.5, alp_mask = 0.5;
        auto shape = merged_pred_output.get_shape();
        for (int h = 0; h < img_h; h++) {
            cur_ofs = h * img_w;
            for (int w = 0; w < img_w; w++) {
                cur_class = output_ptr[cur_ofs + w];
                if (cur_class < 1 || cur_class > get_model_class_num()){
                    continue;
                }
                exist_class[cur_class-1] = true;
                seg_pixel_cnt[cur_class-1] += 1;

                QColor cur_class_color = COLOR_VECTOR[cur_class-1];
                cv::Vec3b pix = PRED_IMG.at<cv::Vec3b>(h, w);
                pix[2] = (int)(((double)pix[2] * alp_src) + ((double)cur_class_color.red() * alp_mask));
                pix[1] = (int)(((double)pix[2] * alp_src) + ((double)cur_class_color.green() * alp_mask));
                pix[0] = (int)(((double)pix[2] * alp_src) + ((double)cur_class_color.blue() * alp_mask));
                PRED_IMG.at<cv::Vec3b>(h, w) = pix;
            }
        }
        for (int cla_idx = 0; cla_idx < seg_pixel_cnt.size(); cla_idx++)
            seg_pixel_rate[cla_idx] = (float)seg_pixel_cnt[cla_idx] / (float)total_pixel;

        // Class Name in edit_show_class
        QString exist_class_string("");
        for (int i = 0; i < ui->tableWidget_class->rowCount(); i++) {
            if (exist_class[i]) {
                QString classname = ui->tableWidget_class->item(i, NAME_COL)->text();
                if (exist_class_string != QString(""))
                    exist_class_string += ", ";
                exist_class_string += classname;
            }
        }
        if (exist_class_string == QString(""))
            exist_class_string = QString("None");
        ui->edit_show_class->setText((QString(" ") + exist_class_string));
    }

    for (int cla_idx = 0; cla_idx < seg_blob_cnt.size(); cla_idx++) {
        new_row.push_back(to_string(seg_blob_cnt[cla_idx]));
    }

    for (int cla_idx = 0; cla_idx < seg_pixel_rate.size(); cla_idx++) {
        float cur_rate = seg_pixel_rate[cla_idx] * 100;
        if (cur_rate == 0) {
            new_row.push_back(std::string("0 %"));
            continue;
        }
        std::ostringstream out;
        out.precision(2);
        out << std::fixed << cur_rate;
        new_row.push_back((out.str() + std::string(" %")));
    }
}

void MainWindow::detSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row){
    vector<int> det_box_cnt(get_model_class_num()-1, 0);
    vector<float> det_box_prob(get_model_class_num()-1, 0);
    vector<bool> exist_class(get_model_class_num());

    if ((PRED_IDX != -1) && (PROB_IDX != -1)) {
        nrt::NDBuffer output_boxes = outputs.get_at(PRED_IDX);
        nrt::NDBuffer output_prob = outputs.get_at(PROB_IDX);
        nrt::Shape output_boxes_shape = output_boxes.get_shape();

        nrt::Shape input_image_shape = get_model_input_shape(0);
        int input_h = PRED_IMG.rows;
        int input_w = PRED_IMG.cols;
        double h_ratio = (double)input_image_shape.dims[0] / input_h;
        double w_ratio = (double)input_image_shape.dims[1] / input_w;

        // Color Box in Result Image
        const int number_of_boxes = output_boxes_shape.dims[0];
        const int* bcyxhw_ptr = output_boxes.get_data_ptr<int>();
        const float* prob_ptr = output_prob.get_data_ptr<float>();

        for (int box_idx = 0; box_idx < number_of_boxes; ++box_idx) {
            BoundingBox bbox = convert_to_bounding_box(bcyxhw_ptr + box_idx * 6, h_ratio, w_ratio);
            const float* probs = prob_ptr + box_idx * (get_model_class_num() + 1);

            if (bbox.class_number < 1 || bbox.class_number > get_model_class_num()){
                qDebug() << "Detection Box class is either background or out of range.";
                continue;
            }

            // Treshold by size
            int h_thres, w_thres, a_thres;
            if(ui->tableWidget_class->item(bbox.class_number-1, HEIGHT_COL))
                h_thres = ui->tableWidget_class->item(bbox.class_number-1, HEIGHT_COL)->text().toInt();
            else return;
            if(ui->tableWidget_class->item(bbox.class_number-1, WIDTH_COL))
                w_thres = ui->tableWidget_class->item(bbox.class_number-1, WIDTH_COL)->text().toInt();
            else return;
            if(ui->tableWidget_class->item(bbox.class_number-1, AND_OR_COL))
                a_thres = (ui->tableWidget_class->item(bbox.class_number-1, AND_OR_COL)->text() == "and" ? 0 : 1);
            else return;

            if (a_thres == 0) { // AND
                if ((bbox.box_height < h_thres) && (bbox.box_width < w_thres))
                    continue;
            }
            else {              // OR
                if ((bbox.box_height < h_thres) || (bbox.box_width < w_thres))
                    continue;
            }

            // Threshold by probability
            float cur_prob = probs[bbox.class_number+1] * 100;
            string cur_prob_thres_str = "";
            if(ui->tableWidget_class->item(bbox.class_number-1, PROB_THRES_COL))
                cur_prob_thres_str = ui->tableWidget_class->item(bbox.class_number-1, PROB_THRES_COL)->text().toStdString();
            else return;

            cur_prob_thres_str = cur_prob_thres_str.substr(0, cur_prob_thres_str.length()-1);
            double cur_prob_thres = QString::fromStdString(cur_prob_thres_str).toDouble();
            if (cur_prob < cur_prob_thres){
                continue;
            }

            exist_class[bbox.class_number-1] = true;
            det_box_cnt[bbox.class_number-1] += 1;
            det_box_prob[bbox.class_number-1] += cur_prob;

            int r, g, b;
            COLOR_VECTOR[bbox.class_number-1].getRgb(&r, &g, &b);
            cv::Scalar class_color_scalar = cv::Scalar(b, g, r);
            cv::rectangle(PRED_IMG,
                          cv::Point(bbox.box_center_X - bbox.box_width/2, bbox.box_center_Y - bbox.box_height/2),
                          cv::Point(bbox.box_center_X + bbox.box_width/2 + (bbox.box_width % 2), bbox.box_center_Y + bbox.box_height / 2 + (bbox.box_height % 2)),
                          class_color_scalar,
                          2
                          );
            string classname = "";
            if(ui->tableWidget_class->item(bbox.class_number-1, NAME_COL))
                classname = ui->tableWidget_class->item(bbox.class_number-1, NAME_COL)->text().toStdString();
            else return;

            std::ostringstream out;
            out.precision(2);
            out << std::fixed << cur_prob;
            string classprob = string("(") + out.str() + string("%)");
            classname += classprob;
            const char* class_string = classname.c_str();

            if (class_string) {
                cv::putText(PRED_IMG, class_string, cv::Point(bbox.box_center_X - bbox.box_width / 2, bbox.box_center_Y - bbox.box_height / 2), FONT_HERSHEY_SIMPLEX, 1, white, 7);
                cv::putText(PRED_IMG, class_string, cv::Point(bbox.box_center_X - bbox.box_width / 2, bbox.box_center_Y - bbox.box_height / 2), FONT_HERSHEY_SIMPLEX, 1, class_color_scalar, 4);
            }
        }

        // Class Name in edit_show_class
        QString exist_class_string("");
        int exist_class_cnt = 0;
        for (int i = 0; i < ui->tableWidget_class->rowCount(); i++) {
            if (exist_class[i]) {
                exist_class_cnt++;
                QString classname = ui->tableWidget_class->item(i, NAME_COL)->text();
                if (exist_class_string != QString("")) {
                    if (exist_class_cnt == 5)
                        exist_class_string += ",\n";
                    else
                        exist_class_string += ", ";
                }
                exist_class_string += classname;
            }
        }
        if (exist_class_string == QString(""))
            exist_class_string = QString("None");
        ui->edit_show_class->setText((QString(" ") + exist_class_string));

        for (int cla_idx = 0; cla_idx < det_box_cnt.size(); cla_idx++) {
            new_row.push_back(to_string(det_box_cnt[cla_idx]));
        }
        for (int cla_idx = 0; cla_idx < det_box_prob.size(); cla_idx++) {
            float prob_sum = det_box_prob[cla_idx];
            if (prob_sum == 0) {
                new_row.push_back(std::string("0 %"));
                continue;
            }
            float cur_cla_prob = prob_sum / (float)det_box_cnt[cla_idx];

            std::ostringstream out;
            out.precision(2);
            out << std::fixed << cur_cla_prob;
            new_row.push_back((out.str() + std::string(" %")));
        }
    }
}

void MainWindow::ocrSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row) {
    if ((PRED_IDX != -1)) {
        nrt::Shape input_image_shape = get_model_input_shape(0);
        int input_h = PRED_IMG.rows;
        int input_w = PRED_IMG.cols;
        double h_ratio = (double)input_image_shape.dims[0] / input_h;
        double w_ratio = (double)input_image_shape.dims[1] / input_w;

        nrt::NDBuffer output_boxes;
        nrt::Shape output_pred_shape;

        output_boxes = outputs.get_at(PRED_IDX);
        output_pred_shape = output_boxes.get_shape();

        const int number_of_boxes = output_pred_shape.dims[0];
        const int* bcyxhw_ptr = output_boxes.get_data_ptr<int>();

        // Rotation apply needed!

        vector<BoundingBox> exist_bbox;
        for (int box_idx = 0; box_idx < number_of_boxes; ++box_idx) {
            BoundingBox bbox = convert_to_bounding_box(bcyxhw_ptr + box_idx * 6, h_ratio, w_ratio);
            exist_bbox.push_back(bbox);

            cv::rectangle(PRED_IMG,
                          cv::Point(bbox.box_center_X - bbox.box_width / 2, bbox.box_center_Y - bbox.box_height / 2),
                          cv::Point(bbox.box_center_X + bbox.box_width / 2 + (bbox.box_width & 1), bbox.box_center_Y + bbox.box_height / 2 + (bbox.box_height & 1)),
                          red,
                          2
                          );

            if(bbox.class_number < 1 || bbox.class_number >  get_model_class_num()){
                qDebug() << "OCR Box class is either background or out of range.";
                continue;
            }
            const char* classname = get_model_class_name(bbox.class_number).toStdString().c_str();
            if (classname) {
                cv::Size org_size = cv::getTextSize(std::string(classname), cv::FONT_HERSHEY_SIMPLEX, 1.0, 4, 0);
                double font_scale = (double)bbox.box_height / (double)org_size.height;
                int scale_gap = (int)((font_scale - 1) * 50);
                // cv::putText(PRED_IMG, classname, cv::Point(bbox.box_center_X - bbox.box_width / 2, bbox.box_center_Y - bbox.box_height / 2), 0, 1, cv::Scalar(0, 0, 255), 3);
                cv::putText(PRED_IMG, classname, cv::Point(bbox.box_center_X - (bbox.box_width / 2), bbox.box_center_Y + bbox.box_height + scale_gap), FONT_HERSHEY_SIMPLEX, font_scale, white, 7, 8, false);
                cv::putText(PRED_IMG, classname, cv::Point(bbox.box_center_X - (bbox.box_width / 2), bbox.box_center_Y + bbox.box_height + scale_gap), FONT_HERSHEY_SIMPLEX, font_scale, red, 4, 8, false);
            }
        }
        sort(exist_bbox.begin(), exist_bbox.end(), bbox_cmp);

        // Set Color in Class List
        // ui->tableWidget_class->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        for (int r = 0; r < ui->tableWidget_class->rowCount(); r++) {
            int colCount = ui->tableWidget_class->columnCount();
            if ((r == 3) || (r == 6))
                colCount = 6;
            for (int c = 0; c < colCount; c++) {
                ui->tableWidget_class->item(r, c)->setBackgroundColor(QColor(255, 255, 255));
                ui->tableWidget_class->item(r, c)->setTextColor(QColor(0, 0, 0));
            }
        }
        for (int i = 0; i < exist_bbox.size(); i++) {
            const char* classname = get_model_class_name(exist_bbox[i].class_number).toStdString().c_str();
            if (classname) {
                const char classchar = classname[0];
                int row = 0, col = 0;
                if ((classchar >= '0') && (classchar <= '9')) {
                    row = 0;
                    col = classchar - '0';
                }
                else if ((classchar >= 'A') && (classchar <= 'Z')) {
                    int order = classchar - 'A';
                    row = 1 + (int)(order / 10);
                    col = order % 10;
                }
                else if ((classchar >= 'a') && (classchar <= 'z')) {
                    int order = classchar - 'a';
                    row = 4 + (int)(order / 10);
                    col = order % 10;
                }
                else {
                }
                ui->tableWidget_class->item(row, col)->setBackgroundColor(QColor(255, 0, 0));
                ui->tableWidget_class->item(row, col)->setTextColor(QColor(255, 255, 255));
            }
        }

        // OCR String in edit_show_class
        QString ocr_string("");
        for (int i = 0; i < exist_bbox.size(); i++) {
            QString classname = get_model_class_name(exist_bbox[i].class_number);
            ocr_string += classname;
        }
        ui->edit_show_class->setText(ocr_string);
        new_row.push_back(ocr_string.toStdString());
    }
}

void MainWindow::anomSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row) {

}

void MainWindow::showResult() {
    cv::Mat ORG_IMG, PRED_IMG;
    QString cur_inf_img_path; // current inference image path
    int pre_idx;

    // save csv table
    vector<std::string> new_row;
    QString cur_img_name;

    int cur_mode = ui->Mode_Setting_Stack->currentIndex();

    if (cur_mode == 0) {  // CAM Mode
        if (ui->com_cam_input_select->currentText().toStdString() == CAMERA_TEXT) {
            // Realtime Camera Mode
            if (!m_usbCam->isExist()) {
                if(m_timer)
                    m_timer->stop();
                return;
            }

            m_usbCam->setResult(); // m_cap >> m_frame
            if (m_usbCam->m_frame.empty()) {
                if(m_timer)
                    m_timer->stop();
                return;
            }

            ORG_IMG = m_usbCam->m_frame.clone();
        }

        // Video Input Mode
        else if (ui->com_cam_input_select->currentText().toStdString() == VIDEO_TEXT){
            if(!m_videoInputCap.isOpened()){
                qDebug() << "Video capture is not opened.";
                return;
            }
            m_videoInputCap >> ORG_IMG;

            if(ORG_IMG.empty()){
                qDebug() << "Video is over!";
                QMessageBox::information(this, "Notification", "The video is over.");
                m_videoInputCap.release();
                video_mode_flag = false;
                on_btn_cam_pause_clicked();
                return;
            }

        }

        QString time_format = "yyMMdd_HHmmss_zzz";
        QDateTime now = QDateTime::currentDateTime();
        std::string time_str = now.toString(time_format).toStdString();
        cur_img_name = QString::fromStdString(time_str + IMG_FORMAT);
    }

    else if (cur_mode == 1) {
        int cur_inf_img_idx = ui->spb_img_cur_idx->value(); // cur_inf_img_idx: 현재 inference하고 있는 이미지의 index
        if (cur_inf_img_idx < 0 || cur_inf_img_idx >= inf_img_list.size()) {
            if(m_timer)
                m_timer->stop();
            img_inf_ing_flag = false;
            return;
        }

        if (!(macro_flag && !macro_cam_flag))
            ui->spb_img_cur_idx->setValue(cur_inf_img_idx+1);

        if (cur_inf_img_idx == 0 && macro_cam_flag)
            ui->spb_img_cur_idx->setRange(1, ui->spb_img_cur_idx->maximum());

        cur_inf_img_path = inf_img_list[cur_inf_img_idx];
        cur_img_name = cur_inf_img_path.split(QDir::separator()).last();
        qDebug() << "[" << cur_inf_img_idx << "] " << inf_img_list[cur_inf_img_idx];

        ORG_IMG = cv::imread(cur_inf_img_path.toStdString(), cv::IMREAD_COLOR);
        qDebug() << "Show Result) Imread";

        if (ORG_IMG.empty()) {
            if(m_timer)
                m_timer->stop();
            return;
        }
        if (cur_inf_img_idx == (inf_img_list.size()-1)) {
            /*** FOR AI EXPO ***/
            if (macro_flag) {
                std::random_shuffle(inf_img_list.begin(),inf_img_list.end());
                ui->spb_img_cur_idx->setRange(0, inf_img_list.size());
                ui->spb_img_cur_idx->setValue(0);
            }
            else {
                if(m_timer)
                    m_timer->stop();
                setImgResultShow(true); // inference result button enabled & iagme index spinbox write enabled
                img_inf_ing_flag = false;
            }
            /*******************/
        }
    }

    new_row.push_back(cur_img_name.toStdString());

    qDebug() << "Inference Image";
    PRED_IMG = ORG_IMG.clone();

    nrt::NDBufferList outputs;
    nrt::NDBuffer merged_pred_output; //Segmentation
//    nrt::NDBuffer merged_prob_output; //Segmentation

    std::chrono::duration<double, std::milli> inf_time;
    qDebug() << "Show Result) Execute";
    QString model_type = get_model_type();
    if(get_model_status() == nrt::STATUS_SUCCESS && get_executor_status() == nrt::STATUS_SUCCESS && class_table_availbale){
        if(model_type == "Segmentation"){
//            if(video_mode_flag && ORG_IMG.rows>1000 && ORG_IMG.cols>1000){
//                cv::resize(ORG_IMG, ORG_IMG, cv::Size(), 0.5, 0.5);
//            }

            nrt::NDBuffer img_buffer = get_img_buffer(ORG_IMG);
            merged_pred_output = seg_execute(img_buffer, inf_time);
            ui->edit_show_inf->setText(QString::number(inf_time.count(), 'f', 3));
        }
        else if(model_type == "Classification" || model_type == "Detection" || model_type == "OCR" || model_type == "Anomaly") {
            nrt::NDBuffer resized_img_buffer = get_img_buffer(ORG_IMG);

            auto start = std::chrono::high_resolution_clock::now();
            outputs = execute(resized_img_buffer);
            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double, std::milli> inf_time = end - start;
            ui->edit_show_inf->setText(QString::number(inf_time.count(), 'f', 3));
        }
    }
    else {
        qDebug() << "Model and executor is not loaded.";
    }

    if( get_model_status() == nrt::STATUS_SUCCESS && get_executor_status() == nrt::STATUS_SUCCESS){
        qDebug() << QString("Model Type:") << model_type;

        if (model_type == "Classification") {
            claSetResults(outputs, PRED_IMG, new_row);
        }
        else if (model_type == "Segmentation") {
            segSetResults(merged_pred_output, PRED_IMG, new_row);
        }
        else if (model_type == "Detection") {
            detSetResults(outputs, PRED_IMG, new_row);
        }
        else if (model_type == "OCR") {
            ocrSetResults(outputs, PRED_IMG, new_row);
        }
        else if (model_type == "Anomaly") {
            anomSetResults(outputs, PRED_IMG, new_row);
        }
    }

    cv::cvtColor(ORG_IMG, ORG_IMG, COLOR_RGB2BGR);
    cv::cvtColor(PRED_IMG, PRED_IMG, COLOR_RGB2BGR);

    Mat_With_Name org_mwn, pred_mwn;
    org_mwn.image = ORG_IMG.clone();
    pred_mwn.image = PRED_IMG.clone();

    if (cur_mode == 0) {        // CAM Mode
        if (cam_autosave_flag) {
//            cur_save_flag = false;
            qDebug() << "CAM Mode) Push Mat to buffer";
            std::unique_lock<std::mutex> lock(m_save_info._mutex);

            QString time_format = "yyMMdd_HHmmss_zzz";
            QDateTime now = QDateTime::currentDateTime();
            std::string time_str = now.toString(time_format).toStdString();
            org_mwn.name = time_str + IMG_FORMAT;
            pred_mwn.name = time_str + "_pred" + IMG_FORMAT;

            m_save_info.org_buffer.push(org_mwn);
            m_save_info.pred_buffer.push(pred_mwn);
            m_save_info.row_buffer.push(new_row);
        }
    }
    else if (cur_mode == 1) {   // IMG Mode
        qDebug() << "IMG Mode) Push Mat to buffer";
        std::unique_lock<std::mutex> lock(m_save_info._mutex);

//        int pre_idx = ui->edit_img_input->text().size() + 1;
//        std::string img_name = cur_inf_img_path.substr(pre_idx);
        ui->edit_img_name->setText(cur_img_name);
        org_mwn.name = cur_img_name.toStdString();
        pred_mwn.name = cur_img_name.toStdString();

        m_save_info.org_buffer.push(org_mwn);
        m_save_info.pred_buffer.push(pred_mwn);
        m_save_info.row_buffer.push(new_row);
    }

    // Show Result Task
    QImage m_qimage;
    if (macro_flag) {
        if (!macro_cam_flag) {
            m_qimage = QImage((const unsigned char*) (ORG_IMG.data), ORG_IMG.cols, ORG_IMG.rows, ORG_IMG.step, QImage::Format_RGB888);
            macro_cam_flag = true;
        }
        else {
            m_qimage = QImage((const unsigned char*) (PRED_IMG.data), PRED_IMG.cols, PRED_IMG.rows, PRED_IMG.step, QImage::Format_RGB888);
            macro_cam_flag = false;
        }
    }
    else {
        if (!show_pred_flag)
            m_qimage = QImage((const unsigned char*) (ORG_IMG.data), ORG_IMG.cols, ORG_IMG.rows, ORG_IMG.step, QImage::Format_RGB888);
        else
            m_qimage = QImage((const unsigned char*) (PRED_IMG.data), PRED_IMG.cols, PRED_IMG.rows, PRED_IMG.step, QImage::Format_RGB888);
    }

    QPixmap m_qpixmap = QPixmap::fromImage(m_qimage);
    ui->lab_show_res->setPixmap(m_qpixmap.scaled(ui->lab_show_res->width(), ui->lab_show_res->height(), Qt::KeepAspectRatio));
}

void MainWindow::on_rad_cam_rtmode_clicked()
{
//    ui->edit_cam_save_term->setEnabled(false);
//    ui->lab_cam_save_term->setEnabled(false);
    ui->rad_cam_autosave->setChecked(false);
    ui->rad_cam_mansave->setChecked(false);
    cam_autosave_flag = false;
    cam_mansave_flag = false;
}

void MainWindow::on_rad_cam_autosave_clicked()
{
//    ui->edit_cam_save_term->setEnabled(true);
//    ui->lab_cam_save_term->setEnabled(true);
    ui->rad_cam_mansave->setChecked(false);
    ui->rad_cam_rtmode->setChecked(false);
    cam_autosave_flag = true;
    cam_mansave_flag = false;

}

void MainWindow::on_rad_cam_mansave_clicked(){
    ui->rad_cam_autosave->setChecked(false);
    ui->rad_cam_rtmode->setChecked(false);
    cam_mansave_flag = true;
    cam_autosave_flag = false;
}

void MainWindow::on_chb_show_prediction_clicked()
{
    /*
    bool status = ui->chb_show_prediction->isChecked();
    if (status) {
        qDebug() << "CAM Mode) Show Prediction";
        show_pred_flag = true;
    }
    else {
        qDebug() << "CAM Mode) Don't Show Prediction";
        show_pred_flag = false;
    }
    */
}

void MainWindow::on_btn_cam_mode_clicked()
{
    mode_flag = false; // false: Cam, true: Img
    ui->btn_cam_mode->setDefault(true);
    ui->btn_img_mode->setDefault(false);
    //on_btn_cam_stop_clicked();
    //on_btn_img_stop_clicked();

    ui->Mode_Setting_Stack->setCurrentIndex(0); // 0: Cam, 1: Img
}

void MainWindow::on_btn_img_mode_clicked()
{
    // 이전에 선택한 input path, output path가 있으면 다시 로드
    if(m_input_path.use_count() > 0){
        qDebug() << "There is previous input path";
        ui->edit_img_input->setText(*m_input_path);
    }
    if(m_output_path.use_count() > 0){
        qDebug() << "There is previous output path";
        ui->edit_img_output->setText(*m_output_path);
    }

    mode_flag = true;
    ui->btn_cam_mode->setDefault(false);
    ui->btn_img_mode->setDefault(true);
    //on_btn_cam_stop_clicked();
    //on_btn_img_stop_clicked();

    ui->Mode_Setting_Stack->setCurrentIndex(1);
}


void MainWindow::on_btn_cam_select_clicked()
{
    if(m_timer.use_count() > 0)
        m_timer.reset();
    if(m_usbCam.use_count() > 0)
        m_usbCam.reset();

    // USB CAM
    if (ui->com_cam_input_select->currentText().toStdString() == CAMERA_TEXT) {
        m_usbCam = std::make_shared<UsbCam>();
        m_usbCam->selectCam();
        if (!m_usbCam->isExist()) {
            if(cam_mode_flag)
                cam_mode_flag = false;
            return;
        }
        cam_mode_flag = true;

        showResult();
        setCamControlEnabled(true);
        on_btn_cam_play_clicked();
        ui->edit_show_inf->setText("");
    }

    // Video Input
    else if (ui->com_cam_input_select->currentText() == "Video"){
        if(m_videoInputCap.isOpened()){
            m_videoInputCap.release();
        }

        // VideoCapture(const String& filename, int apiPreference = CAP_ANY);
        QString video_filename = QFileDialog::getOpenFileName(this,
                                                              tr("Select Input Video"),
                                                              QDir::homePath(), tr(" (*.avi, *.mp4)"));
        if(video_filename.isEmpty()){
            QMessageBox::information(this, "Notification", "No video was selected");
            return;
        }

        m_videoInputCap.open(video_filename.toStdString(), cv::CAP_ANY);
        m_videoInputCap.set(cv::CAP_PROP_FPS, 30);

        if(!m_videoInputCap.isOpened()){
            qDebug() << "Failed to open the video file.";
            if(video_mode_flag)
                video_mode_flag = false;
            return;
        }
        video_mode_flag = true;

        showResult();
        setCamControlEnabled(true);
        on_btn_cam_play_clicked();
    }
}


void MainWindow::on_btn_cam_save_clicked()
{
    QString savePath = QFileDialog::getExistingDirectory(this, tr("Select Save Folder"), QDir::homePath());
    if (savePath.isNull())
        return;
    QString org_fol_path, pred_fol_path;
    org_fol_path = savePath + QString::fromStdString(PathSeparator) + QString::fromStdString(ORG_FOL);
    pred_fol_path = savePath + QString::fromStdString(PathSeparator) + QString::fromStdString(PRED_FOL);
    if (!QDir(org_fol_path).exists())
        QDir().mkdir(org_fol_path);
    if (!QDir(pred_fol_path).exists())
        QDir().mkdir(pred_fol_path);
//    ui->edit_cam_save->setText(savePath);
}

void MainWindow::on_btn_cam_play_clicked()
{
    qDebug() << "come cam play";

    // Camera Mode
    if (cam_mode_flag && m_usbCam.use_count() <= 0) {
        qDebug() << "Cam mode but camera is not open.";
        return;
    }

    // Video Input mode
    if(video_mode_flag && !m_videoInputCap.isOpened()){
        qDebug() << "Video mode but capture is not open.";
        return;
    }

    if (m_timer.use_count() <= 0) { // First Connect with showResult()
        qDebug() << "CAM Mode) Play";

        if(ui->rad_cam_rtmode->isChecked()){
            if(m_save_timer.use_count() > 0){
                m_save_timer.reset();
            }
        }

        setCamSelectEnabled(false);
        setCamSaveEditEnabled(false);

        if (m_timer.use_count() == 0)
            m_timer = make_shared<QTimer>(this);

        connect(m_timer.get(), SIGNAL(timeout()), this, SLOT(showResult()));

        m_timer->setInterval(0);
        m_timer->start();

        if(m_usbCam.use_count() >0 )
            m_usbCam->playCam();

        setTabelColumn(true);
    }
    else {          // Already Connected
        qDebug() << "CAM Mode) Replay";
        /*if (cam_save_flag) {
            if (!checkCanSave())
                return;
            double save_term = ui->edit_cam_save_term->text().toDouble();
            string save_path = ui->edit_cam_save->text().toUtf8().constData();
            qDebug() << "CAM Mode) Save Term:" << save_term << "  |  Save Path:" << QString::fromStdString(save_path);
            {
                std::unique_lock<std::mutex> lock(m_save_info._mutex);
                m_save_info.save_path = save_path;
            }

            string org_fol_path, pred_fol_path;
            org_fol_path = save_path + PathSeparator + ORG_FOL;
            pred_fol_path = save_path + PathSeparator + PRED_FOL;
            if (mkdir(org_fol_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
                if (errno == EEXIST) {
                    qDebug() << "CAM Mode) Orgin Folder Already Exist";
                }
                else {
                    qDebug() << "CAM Mode) Orgin Folder Create Error";
                }
            }
            if (mkdir(pred_fol_path.c_str(), S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH) == -1) {
                if (errno == EEXIST) {
                    qDebug() << "CAM Mode) Predict Folder Already Exist";
                }
                else {
                    qDebug() << "CAM Mode) Predict Folder Create Error";
                }
            }

            if (m_save_timer.use_count() == 0)
                m_save_timer = make_shared<QTimer>(this);
            connect(m_save_timer.get(), SIGNAL(timeout()), this, SLOT(setSaveStautus()));
            m_save_timer->setInterval(save_term * 1000);
            m_save_timer->start();
        }
        else {
            if (m_save_timer.use_count() > 0)
                m_save_timer.reset();
        }*/
        setCamSaveChangeEnabled(false);
        m_timer->setInterval(0);
        m_timer->start();
        if(cam_mode_flag)
            m_usbCam->playCam();
    }
    ui->btn_img_mode->setEnabled(false);
}

void MainWindow::on_btn_cam_pause_clicked()
{
    if (m_timer.use_count() <= 0)
        return;
    if (m_timer->isActive()) {
        qDebug() << "CAM Mode) Pause";
        m_timer->stop();
        if(cam_mode_flag && m_usbCam->isExist())
            m_usbCam->pauseCam();

    }

    if (m_save_timer.use_count() > 0)
        m_save_timer->stop();

    setCamSaveChangeEnabled(true);
}

void MainWindow::on_btn_cam_stop_clicked()
{
    if (m_save_timer.use_count() > 0) {
        qDebug() << "CAM Mode) Delete Thread";
        m_save_timer.reset();
    }

    if (m_timer.use_count() > 0) {
        qDebug() << "CAM Mode) Stop";
        m_timer.reset();
    }

    if (m_usbCam.use_count() > 0) {
        qDebug() << "CAM Mode) Delete CAM";
        m_usbCam.reset();
        cam_mode_flag = false;
    }

    if(m_videoInputCap.isOpened()){
        qDebug() << "VIDEO Mode) Delete Video Capture";
        m_videoInputCap.release();
        video_mode_flag = false;
    }

    setCamSelectEnabled(true);
    setCamControlEnabled(false);
    setCamSaveEditEnabled(true);

    ui->lab_result_info->setText("There is no table to show.");
    setTabelColumn(false);

    ui->lab_show_res->setText("Please Select CAM or IMG folder.");
    ui->edit_show_class->setText("");
    ui->edit_show_inf->setText("");

    ui->btn_img_mode->setEnabled(true);

    if (get_model_status() == nrt::STATUS_SUCCESS && get_model_type() == "OCR") {
        for (int r = 0; r < ui->tableWidget_class->rowCount(); r++) {
            int colCount = ui->tableWidget_class->columnCount();
            if ((r == 3) || (r == 6))
                colCount = 6;
            for (int c = 0; c < colCount; c++) {
                ui->tableWidget_class->item(r, c)->setBackgroundColor(QColor(255, 255, 255));
                ui->tableWidget_class->item(r, c)->setTextColor(QColor(0, 0, 0));
            }
        }
    }
}

void MainWindow::on_btn_select_model_clicked()
{
    if (!get_gpu_status()) {
        on_btn_select_gpu_clicked();
        if (!get_gpu_status())
            return;
    }

//    QString modelPath = selectModelWindow();

    QString modelPath = QFileDialog::getOpenFileName(this, tr("Select Model"), QDir::homePath(), tr("default (*.net)"));

    if (modelPath == "")
        return;

    connect(futureWatcher.get(), SIGNAL(started()), this, SLOT(set_model_started()));
    connect(futureWatcher.get(), SIGNAL(finished()), this, SLOT(set_model_completed()));

    future = QtConcurrent::run(set_model, modelPath, ui->cbx_select_fp16->isChecked());
    futureWatcher->setFuture(future);
}

void MainWindow::set_model_started(){
    if ( ui->Model_Proper->currentIndex() == 0){ // Info Page -> Status Page
        ui->Model_Proper->setCurrentWidget(ui->Model_Status_Page);
    }
    if(class_table_availbale == true){
        class_table_availbale = false;
    }

    ui->lab_model_status->setText("Loading Model...\nIt may take a few seconds...");
    ui->btn_select_model->setEnabled(false);
    ui->btn_select_gpu->setEnabled(false);
    ui->cbx_select_fp16->setEnabled(false);

    // Clear class label and inference time
    ui->edit_show_inf->clear();
    ui->edit_show_class->clear();

    setClassTable(false);
}

void MainWindow::set_model_completed(){
    QString modelPath = futureWatcher->result(); // 결과값 modelPath가 빈 문자열이면 문제가 생긴거고 아니면 잘 된 거

    disconnect(futureWatcher.get(), SIGNAL(started()), this, SLOT(set_model_started()));
    disconnect(futureWatcher.get(), SIGNAL(finished()), this, SLOT(set_model_completed()));

    if(!modelPath.isEmpty() && (get_model_status() == nrt::STATUS_SUCCESS)){
        qDebug() << "NRT) Set Model Completed! :"  << modelPath;

        QString model_name = modelPath.split(QDir::separator()).last();

        //setTabelColumn(true);
        setModelInfo(true, model_name);
    }
    else {
        qDebug() << "NRT) Set Model Failed!";

        QMessageBox messageBox;
        messageBox.critical(0,"Error","Model initialization has falied!");
        messageBox.setFixedSize(500,200);

        ui->lab_model_status->setText("Please Select Model.");
    }

    ui->btn_select_model->setEnabled(true);
    ui->btn_select_gpu->setEnabled(true);
    ui->cbx_select_fp16->setEnabled(true);
}

void MainWindow::on_btn_select_gpu_clicked()
{
    int idx = 0;
    QDialog *selectDlg = new QDialog;
    selectDlg->setWindowTitle("GPU List");
    selectDlg->setFixedSize(400, 300);

    QVBoxLayout *dlgVLayout = new QVBoxLayout;

    QLabel *cHeader = new QLabel("GPU List", selectDlg);
    dlgVLayout->addWidget(cHeader);
    int gpuNum = get_gpu_num();
    QListWidget *gpuList = new QListWidget;
    for (int idx = 0; idx < gpuNum; idx++) {
        QListWidgetItem *gpuItem = new QListWidgetItem;
        gpuItem->setText(QString("GPU ").append(QString::number(idx)));
        gpuList->insertItem(idx, gpuItem);
    }
    dlgVLayout->addWidget(gpuList);

    QHBoxLayout *btns = new QHBoxLayout;
    QPushButton *okBtn = new QPushButton("Select");
    QPushButton *cancelBtn = new QPushButton("Cancel");

    okBtn->connect(okBtn, SIGNAL(clicked()), selectDlg, SLOT(accept()));
    cancelBtn->connect(cancelBtn, SIGNAL(clicked()), selectDlg, SLOT(reject()));

    btns->addStretch();
    btns->addWidget(okBtn);
    btns->addWidget(cancelBtn);
    dlgVLayout->addLayout(btns);

    selectDlg->setLayout(dlgVLayout);
    if (selectDlg->exec() == QDialog::Accepted) {
        idx = gpuList->currentRow();
        set_gpu(idx);
    }
    else
        return;
}

/*** Load IMG Folder ***/

void MainWindow::on_btn_img_input_clicked()
{
    QMessageBox *err_msg = new QMessageBox;
    err_msg->setFixedSize(600, 400);

    QString inputPath = QFileDialog::getExistingDirectory(this, tr("Select Input Folder"), QDir::homePath());
    if (inputPath.isNull())
        return;
    if( m_input_path.use_count() > 0 ) {
        m_input_path.reset();
    }
    m_input_path = std::make_shared<QString>(inputPath);

    QDir input_dir(inputPath);
    QStringList img_filters;
    int inf_img_num;
    img_filters << "*.bmp" << "*.dib"
                << "*.jpg" << "*.jpeg" << "*.jpe" << "*.jp2"
                << "*.png" << "*.webp"
                << "*.pbm" << "*.pgm" << "*.ppm" << "*.pxm" << "*.pnm"
                << "*.tiff" << "*.tif"
                << "*.sr" << "*.ras"
                << "*.hdr" << "*.pic";
    QDirIterator input_it(inputPath, img_filters, QDir::AllEntries | QDir::NoDotAndDotDot, QDirIterator::Subdirectories);
    inf_img_list.clear();
    while (input_it.hasNext())
        inf_img_list.append(input_it.next());

    inf_img_num = inf_img_list.size();
    if (inf_img_num < 1) {
        err_msg->critical(0,"Error", "There are no image files to inference.");
        return;
    }

    ui->edit_img_input->setText(inputPath);
    ui->lab_img_total_num->setText("/ " + QString::number(inf_img_num));
    ui->spb_img_cur_idx->setRange(0, inf_img_num);
    ui->spb_img_cur_idx->setValue(0);

    QCollator collator;
    collator.setNumericMode(true);
    std::sort(inf_img_list.begin(), inf_img_list.end(), collator);

    /*** FOR AI EXPO ***/
    if (inputPath.contains(QString("Expo"))) {
        std::random_shuffle(inf_img_list.begin(),inf_img_list.end());
        macro_flag = true;
    }
    else {
        macro_flag = false;
    }
    /*******************/

    //for (int i = 0; i < inf_img_num; i++)
    //    qDebug() << inf_img_list[i];

//    m_loading_effect.reset();

//    this->setEnabled(true);
//    movie->~QMovie();
//    lbl->~QLabel();
    delete err_msg;
}

void MainWindow::on_btn_img_output_clicked()
{
    QString outputPath = QFileDialog::getExistingDirectory(this, tr("Select Output Folder"), QDir::homePath());
    if (outputPath.isNull())
        return;
    QString org_fol_path, pred_fol_path;
    org_fol_path = outputPath + QString::fromStdString(PathSeparator) + QString::fromStdString(ORG_FOL);
    pred_fol_path = outputPath + QString::fromStdString(PathSeparator) + QString::fromStdString(PRED_FOL);
    if (!QDir(org_fol_path).exists())
        QDir().mkdir(org_fol_path);
    if (!QDir(pred_fol_path).exists())
        QDir().mkdir(pred_fol_path);

    if( m_output_path.use_count() > 0 ) {
        m_output_path.reset();
    }
    m_output_path = std::make_shared<QString>(outputPath);

    ui->edit_img_output->setText(outputPath);
}

void MainWindow::on_cbx_set_show_time_stateChanged(int state)
{
    if (state == Qt::Unchecked) {
        setImgShowTimeEditEnable(false);
    }
    else if (state == Qt::Checked) {
        setImgShowTimeEditEnable(true);
    }
}

void MainWindow::on_btn_img_play_clicked()
{
    QMessageBox *err_msg = new QMessageBox;
    err_msg->setFixedSize(600, 400);
    int img_show_time = 0;
    std::string img_save_path;
    img_inf_ing_flag = true;

    if (m_timer.use_count() <= 0) { // First Connect with showResult()
        if (!m_input_path) {
            err_msg->critical(0,"Error", "Please set input folder.");
            on_btn_img_input_clicked();
            return;
        }
        if (!m_output_path) {
            err_msg->critical(0,"Error", "Please set output folder.");
            on_btn_img_output_clicked();
            return;
        }
        else
            img_save_path = ui->edit_img_output->text().toStdString();

        if (ui->cbx_set_show_time->isChecked()) {
            if (ui->edit_set_show_time->text() == "") {
                err_msg->critical(0,"Error", "Please set show time or unselect set show time.");
                ui->edit_set_show_time->setFocus();
                return;
            }
            img_show_time = (int)(ui->edit_set_show_time->text().toFloat() * 1000);
            if (img_show_time < 0 || img_show_time > 60000) {
                err_msg->critical(0,"Error", "Please set show time greater than 0 sec and less than 60 sec.");
                ui->edit_set_show_time->setFocus();
                return;
            }
        }

        if (get_model_status() != nrt::STATUS_SUCCESS) {
            qDebug() << "NRT) There is no model";
            err_msg->critical(0,"Error", "Please choose model to inference.");
            on_btn_select_model_clicked();
            return;
        }

        if (get_executor_status() != nrt::STATUS_SUCCESS){
            qDebug() << "NRT) There is no executor";
            err_msg->critical(0, "Error", "Please wait for the model executor to be created.");
            return;
        }

        {
            std::unique_lock<std::mutex> lock(m_save_info._mutex);
            m_save_info.save_path = img_save_path;
        }

        // DB save or not



        ui->btn_cam_mode->setEnabled(false);

        qDebug() << "IMG Mode) Play";

        setImgInference(true);

        if (m_timer.use_count() <= 0)
            m_timer = make_shared<QTimer>(this);

        connect(m_timer.get(), SIGNAL(timeout()), this, SLOT(showResult()));

        showResult();

        if (macro_flag)
            m_timer->setInterval((int)(img_show_time/2));
        else
            m_timer->setInterval(img_show_time);

        m_timer->start();

        setTabelColumn(true);
    }

    else {          // Already Connected
        if (!m_timer->isActive()) {
            qDebug() << "IMG Mode) Replay";
            setImgShowTimeEditEnable(false);
            if (ui->cbx_set_show_time->isChecked()) {
                if (ui->edit_set_show_time->text() == "") {
                    err_msg->critical(0,"Error", "Please set show time or unselect set show time.");
                    ui->edit_set_show_time->setFocus();
                    return;
                }
                img_show_time = (int)(ui->edit_set_show_time->text().toFloat() * 1000);
                if (img_show_time < 0 || img_show_time > 60000) {
                    err_msg->critical(0,"Error", "Please set show time greater than 0 sec and less than 60 sec.");
                    ui->edit_set_show_time->setFocus();
                    return;
                }
            }

            m_timer->setInterval(img_show_time);
            m_timer->start();
        }
    }

    delete err_msg;
}

void MainWindow::on_btn_img_pause_clicked()
{
    if (m_timer.use_count() <= 0)
        return;
    img_inf_ing_flag = true;
    if (m_timer->isActive()) {
        qDebug() << "CAM Mode) Pause";
        m_timer->stop();
    }

    setImgShowTimeEditEnable(true);
}

void MainWindow::on_btn_img_stop_clicked()
{
    img_inf_ing_flag = false;
    if (m_timer.use_count() > 0) {
        qDebug() << "IMG Mode) Stop";
        m_timer.reset();
    }

    ui->btn_cam_mode->setEnabled(true);

    setTabelColumn(false);
    ui->lab_result_info->setText("There is no table to show.");
    inf_img_list.clear();

    ui->edit_img_input->setText("");
    ui->edit_img_output->setText("");
    if(m_input_path.use_count() > 0){
        m_input_path.reset();
    }
    if(m_output_path.use_count() > 0){
        m_output_path.reset();
    }

    ui->edit_img_name->setText("");
    ui->cbx_set_show_time->setChecked(false);
    ui->edit_set_show_time->setText("");
    ui->spb_img_cur_idx->setRange(0, 0);
    ui->lab_img_total_num->setText(" /  TOTAL");

    setImgInference(false);
    setImgShowTimeEditEnable(false);
    setImgResultShow(false);

    ui->lab_show_res->setText("Please Select CAM or IMG folder.");
    ui->edit_show_class->setText("");
    ui->edit_show_inf->setText("");

    if (get_model_status() == nrt::STATUS_SUCCESS && get_model_type() == "OCR") {
        for (int r = 0; r < ui->tableWidget_class->rowCount(); r++) {
            int colCount = ui->tableWidget_class->columnCount();
            if ((r == 3) || (r == 6))
                colCount = 6;
            for (int c = 0; c < colCount; c++) {
                ui->tableWidget_class->item(r, c)->setBackgroundColor(QColor(255, 255, 255));
                ui->tableWidget_class->item(r, c)->setTextColor(QColor(0, 0, 0));
            }
        }
    }
}

void MainWindow::on_spb_img_cur_idx_valueChanged(int cur_idx)
{
    if (img_inf_ing_flag)
        return;
    if (cur_idx < 1 || cur_idx > inf_img_list.size()) {
        return;
    }
    QString cur_img_path = inf_img_list[cur_idx-1];
    QStringList path_split = inf_img_list[cur_idx-1].split(QLatin1Char(PathSeparator[0]));
    QString cur_img_name = path_split.at(path_split.size()-1);
    path_split.pop_back();
    std::string new_img_path = (*m_output_path + QString(PathSeparator[0]) + QString::fromStdString(PRED_FOL) + cur_img_name).toStdString();
    cv::Mat cur_img = cv::imread(new_img_path, cv::IMREAD_COLOR);
    if (cur_img.empty())
        return;
    cv::cvtColor(cur_img, cur_img, COLOR_RGB2BGR);
    QImage m_qimage = QImage((const unsigned char*) (cur_img.data), cur_img.cols, cur_img.rows, cur_img.step, QImage::Format_RGB888);
    QPixmap m_qpixmap = QPixmap::fromImage(m_qimage);
    ui->lab_show_res->setPixmap(m_qpixmap.scaled(ui->lab_show_res->width(), ui->lab_show_res->height(), Qt::KeepAspectRatio));
    ui->edit_img_name->setText(cur_img_name);
}

void MainWindow::on_com_cam_input_select_currentTextChanged(const QString &text){
    if(text.toStdString() == VIDEO_TEXT){
        ui->btn_cam_select->setText("Select Video File");
    }
    else if(text.toStdString() == CAMERA_TEXT){
        ui->btn_cam_select->setText("Select Camera");
    }
}
