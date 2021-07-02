#include "mainwindow.h"
#include "ui_mainwindow.h"

#define TERM_MIN 0.0
#define TERM_MAX 60.0

const string PathSeparator = QString(QDir::separator()).toStdString();

const string IMG_FORMAT = ".png";
const string ORG_FOL = "org" + PathSeparator;
const string PRED_FOL = "pred" + PathSeparator;

const string VIDEO_FOLDER_TEXT = "Video Folder";
const string SINGLE_VIDEO_TEXT = "Video File";
const string CAMERA_TEXT = "Relatime Camera";

QVector<QColor> COLOR_VECTOR;

int currentModelId = -1, ensmbleModelId;
int ensmble_crop_size = 128;

bool ready_for_inference = false;

int COLOR_COL = 0, NAME_COL = 1, PROB_THRES_COL = 5, WIDTH_COL = 2, HEIGHT_COL = 4, AND_OR_COL = 3;
cv::Scalar red(0, 0, 255);
cv::Scalar green(0, 255, 0);
cv::Scalar blue(255, 0, 0);
cv::Scalar white(255, 255, 255);
cv::Scalar black(0, 0, 0);

QString set_model_thread(NrtExe* nrt_ptr, QString modelPath, bool fp16_flag){
    return nrt_ptr->set_model(modelPath, fp16_flag);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Neuro-E");

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
    camType.append(QString::fromStdString(SINGLE_VIDEO_TEXT));
    camType.append(QString::fromStdString(VIDEO_FOLDER_TEXT));
    camType.append(QString::fromStdString(CAMERA_TEXT));

    ui->com_cam_input_select->addItems(camType);
    if(ui->com_cam_input_select->currentText().toStdString() == SINGLE_VIDEO_TEXT){
        ui->btn_cam_select->setText("Select Video File");
    }
    else if(ui->com_cam_input_select->currentText().toStdString() == CAMERA_TEXT){
        ui->btn_cam_select->setText("Select Camera");
    }
    else if(ui->com_cam_input_select->currentText().toStdString() == VIDEO_FOLDER_TEXT){
        ui->btn_cam_select->setText("Select Video Folder");
    }

    // CAM Save Style
    ui->rad_cam_rtmode->setChecked(true);
    ui->rad_cam_autosave->setToolTip("Auto save option is avavilable only when model is loaded.");

    // IMG Mode Style
//    setImgShowTimeEditEnable(false);

    // IMG Icon
    ui->btn_img_mode->setIcon(QIcon(":/icons/img_tab.png"));
    ui->btn_img_mode->setToolTip("IMG Mode");
    ui->btn_img_mode->setIconSize(ui->btn_img_mode->size());

//    ui->edit_img_name->setReadOnly(true);
    ui->edit_set_show_time->setValidator(new QDoubleValidator(TERM_MIN, TERM_MAX, 1, this));

//    ui->spb_img_cur_idx->setReadOnly(true);
    ui->btn_img_play->setIcon(QIcon(":/icons/cam_play.png"));
    ui->btn_img_play->setIconSize(QSize(ui->btn_img_play->height()*0.45, ui->btn_img_play->height()*0.45));
    ui->btn_img_pause->setIcon(QIcon(":/icons/cam_pause.png"));
    ui->btn_img_pause->setIconSize(QSize(ui->btn_img_pause->height()*0.45, ui->btn_img_pause->height()*0.45));
    ui->btn_img_stop->setIcon(QIcon(":/icons/cam_stop.png"));
    ui->btn_img_stop->setIconSize(QSize(ui->btn_img_stop->height()*0.45, ui->btn_img_stop->height()*0.45));

    ui->btn_img_play->setEnabled(false);
    ui->btn_img_pause->setEnabled(false);
    ui->btn_img_stop->setEnabled(false);

    // Model Property Style
    ui->cbx_select_fp16->setToolTip("Slower when creating Executor, but Faster when predicting Img");
    ui->cbx_select_fp16->setWhatsThis("Slower when creating Executor, but Faster when predicting Img");
    ui->lab_model_status->setAlignment(Qt::AlignCenter);

    QSqlError err = m_db->InitialDBSetup();
    if(err.type() != QSqlError::NoError){
        qDebug() << "Initial DB Setup failed: " << err;
        m_db.reset();
    }

    ui->com_video_list->hide();
    ui->com_video_list->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    ui->inf_numbers->hide();

    ui->com_image_list->hide();
    ui->com_image_list->setSizeAdjustPolicy(QComboBox::AdjustToContents);
    ui->spb_img_cur_idx->hide();
    ui->lab_img_total_num->hide();

    ui->btn_model_settings->hide();
    ui->btn_model_settings->setIcon(QIcon(":/icons/model_settings.png"));

    ui->Mode_Setting_Stack->setCurrentIndex(0);
    ui->Img_Option->setCurrentWidget(ui->SelectImagesetPage);

    ui->rad_img_rtmode->setChecked(true);
    ui->rad_img_save_mode->setToolTip("Save option is only available when model is loaded.");
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

void MainWindow::setCamSaveChangeEnabled(bool flag) {  // Can be Changed Part when Paused
    //ui->rad_cam_rtmode->setEnabled(flag);
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

void MainWindow::setImgInference(bool flag) {
    ui->cbx_set_show_time->setEnabled(!flag);
//    ui->edit_set_show_time->setEnabled(!flag);
//    ui->lab_set_show_time->setEnabled(!flag);

//    ui->spb_img_cur_idx->setReadOnly(flag);
    if (!flag)
        setImgResultShow(false);
}

void MainWindow::setImgResultShow(bool flag) {
    // setImageResultShow(true) -> set the img index spinbox to Write Available, image result buttoned Enabled
    ui->spb_img_cur_idx->setReadOnly(!flag);
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
        if (media_mode == MEDIA_MODE_CAM) {   // CAM Mode
            qDebug() << "CAM Mode) Show Prediction Icon Clicked";
        }
        else {              // IMG Mode
            qDebug() << "IMG Mode) Show Prediction Icon Clicked";
            //showResult();

            /*int cur_inf_img_idx = ui->spb_img_cur_idx->value();
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
            qDebug() << "IMG Mode) IMG Change";*/
        }
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
    if (m_nrt.use_count() > 0 && m_nrt->get_model_status() == nrt::STATUS_SUCCESS) {
        QString model_type = m_nrt->get_model_type();
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
            num_classes = m_nrt->get_model_class_num();
            ui->lab_result_info->setText("* CLA Info: (%) - Probabilty of each class");
            ui->tableWidget_result->setColumnCount(num_classes - START_POINT + 1);
            horHeaderLabels.append(QString("Class"));
            for (int cla_idx = START_POINT; cla_idx < num_classes; cla_idx++)
                horHeaderLabels.append((m_nrt->get_model_class_name(cla_idx) + QString("(%)")));
        }
        else if (model_type == "Segmentation") {
            num_classes = m_nrt->get_model_class_num();
            ui->lab_result_info->setText("* SEG Info: (#) - Number of blobs, (Pixel) - % of pixel area");
            ui->tableWidget_result->setColumnCount((num_classes - START_POINT) * 2);
            for (int cla_idx = START_POINT; cla_idx < num_classes; cla_idx++)
                horHeaderLabels.append((m_nrt->get_model_class_name(cla_idx) + QString("(#)")));
            for (int cla_idx = START_POINT; cla_idx < num_classes; cla_idx++)
                horHeaderLabels.append((m_nrt->get_model_class_name(cla_idx) + QString("(Pixel)")));
        }
        else if (model_type == "Detection") {
            num_classes = m_nrt->get_model_class_num();
            ui->lab_result_info->setText("* DET Info: (#) - Number of Objects, (%) - Probability Mean of each class");
            ui->tableWidget_result->setColumnCount((num_classes - START_POINT) * 2);
            for (int cla_idx = START_POINT; cla_idx < num_classes; cla_idx++)
                horHeaderLabels.append((m_nrt->get_model_class_name(cla_idx) + QString("(#)")));
            for (int cla_idx = START_POINT; cla_idx < num_classes; cla_idx++)
                horHeaderLabels.append((m_nrt->get_model_class_name(cla_idx) + QString("(%)")));
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
    else{
        qDebug() << "setTableColumn) m_nrt is not loaded.";
    }
}

void MainWindow::resultItemClicked(int row, int col) {
    resultVerClicked(row);
}

void MainWindow::resultVerClicked(int row) {
    /*if (media_mode == MEDIA_MODE_IMAGE) {    // IMG Mode
        ui->spb_img_cur_idx->setValue(row+1);
        return;
    }

    QString cur_img_name = ui->tableWidget_result->verticalHeaderItem(row)->text();
//    QString output_fol = (!mode_flag ? ui->edit_cam_save->text() : ui->edit_img_output->text());
    QString output_fol = (media_mode != MEDIA_MODE_IMAGE? "HAHA" : ui->edit_img_output->text());
    std::string cur_img_path = output_fol.toStdString() + PathSeparator + PRED_FOL + cur_img_name.toStdString();
    cv::Mat cur_img = cv::imread(cur_img_path, cv::IMREAD_COLOR);
    if (cur_img.empty())
        return;
    cv::cvtColor(cur_img, cur_img, COLOR_RGB2BGR);
    QImage m_qimage = QImage((const unsigned char*) (cur_img.data), cur_img.cols, cur_img.rows, cur_img.step, QImage::Format_RGB888);
    QPixmap m_qpixmap = QPixmap::fromImage(m_qimage);
    ui->lab_show_res->setPixmap(m_qpixmap.scaled(ui->lab_show_res->width(), ui->lab_show_res->height(), Qt::KeepAspectRatio));*/
}

/* Loop for Save Images */
void MainWindow::save_worker() {
    QSqlDatabase save_thread_db = QSqlDatabase::addDatabase("QSQLITE", "save_thread");
    save_thread_db.setDatabaseName(QString("%1%2").arg(qApp->applicationDirPath()).arg("/neuore.db"));
    save_thread_db.open();
    while (1) {
        std::unique_lock<std::mutex> lock(m_save_info._mutex);

        Mat_With_Name org_mwn, pred_mwn;
        vector<std::string> new_row;

        // insert new row in Images table
        int imageId = -1;
        if(!m_save_info.org_buffer.empty()){
            org_mwn = m_save_info.org_buffer.front();
            m_save_info.org_buffer.pop();

            QUuid u_id = QUuid::createUuid();
            QString org_file_uid = u_id.toString().remove(QChar('{')).remove(QChar('}')) + QString(IMG_FORMAT.c_str());
            QString org_path = imagesDir.absoluteFilePath(org_file_uid);

            QString org_image_name = QString(org_mwn.name.c_str());

            cv::cvtColor(org_mwn.image, org_mwn.image, cv::COLOR_RGB2BGR);
            cv::imwrite(org_path.toStdString(), org_mwn.image);

            qDebug() << "Save thread) Saving image " << org_mwn.name.c_str();

            imageId = m_db->InsertImage(org_path, org_image_name, org_mwn.image.rows, org_mwn.image.cols, m_save_info.imageSetId);
            if(imageId == -1){
                qDebug() << "Save thread) Got invalid ImageId.";
                return;
            }
        }

        // insert new row in ResultItem table
        int resultItemID;
        if(!m_save_info.pred_buffer.empty()){
            pred_mwn = m_save_info.pred_buffer.front();
            m_save_info.pred_buffer.pop();

            QUuid u_id = QUuid::createUuid();
            QString pred_file_uid = u_id.toString().remove(QChar('{')).remove(QChar('}')) + QString(IMG_FORMAT.c_str());
            QString pred_path = resultImagesDir.absoluteFilePath(pred_file_uid);

            QString pred_image_name = QString(pred_mwn.name.c_str());

            cv::cvtColor(pred_mwn.image, pred_mwn.image, cv::COLOR_RGB2BGR);
            cv::imwrite(pred_path.toStdString(), pred_mwn.image);

            qDebug() << "Save thread) Saving image " << pred_mwn.name.c_str();

            // image id가 -1이면 이미지 save 모드에서 org img를 저장하는 과정을 거치지 않은 경우임.
            // mat with name에 저장된 image id 참고 필요.
            if(imageId == -1){
                imageId = pred_mwn.imageId;
            }

            resultItemID = m_db->InsertResultItem(pred_path, imageId, m_save_info.evaluationSetId);
        }

        if(!m_save_info.row_buffer.empty()){
            new_row = m_save_info.row_buffer.front();
            m_save_info.row_buffer.pop();

            QJsonObject& title = m_save_info.title;

            if(inf_mode_status == INF_MODE_SINGLE && m_nrt.use_count() > 0){
                title["numOfItems"] = title["numOfItems"].toInt() + 1;

                QString model_type = m_nrt->get_model_type();
                QJsonArray body_data = title["data"].toArray();
                QJsonObject result_obj;

                qDebug() << "Save thread) Saving evaluation result ";
                if(model_type == "Classification"){
                    QString pred_class = QString::fromStdString(new_row[1]);
                    title[pred_class+"_NumOfImages"] = title[pred_class+"_NumOfImages"].toInt() + 1;

                    result_obj["ResultItemId"] = resultItemID;
                    result_obj["PredClass"] = pred_class;
                    result_obj["InfTime"] = new_row[0].c_str();
                    for(int i=0; i < m_nrt->get_model_class_num(); i++){
                        result_obj[pred_class + "_Prob"] = new_row[i + 2].c_str();
                    }

                    body_data.append(result_obj);
                }
                else if(model_type == "Detection"){
                    result_obj["ResultItemId"] = resultItemID;
                    result_obj["InfTime"] = new_row[0].c_str();
                    QString class_name;
                    for(int i=1; i < m_nrt->get_model_class_num(); i++){
                        class_name = m_nrt->get_model_class_name(i);
                        result_obj[class_name + "_NumOfDetectedBoxes"] = new_row[i].c_str();
                        result_obj[class_name + "_DetectionProb"] = new_row[i + m_nrt->get_model_class_num() - 1].c_str();
                    }

                    body_data.append(result_obj);
                }
                else if(model_type == "Segmentation"){
                    result_obj["ResultItemId"] = resultItemID;
                    result_obj["InfTime"] = new_row[0].c_str();
                    QString class_name;
                    for(int i=1; i < m_nrt->get_model_class_num(); i++){
                        class_name = m_nrt->get_model_class_name(i);
                        result_obj[class_name + "_NumOfBlobs"] = new_row[i].c_str();
                        result_obj[class_name + "_PixelRatio"] = new_row[i + m_nrt->get_model_class_num() -1].c_str();
                    }
                }
                else if(model_type == "OCR"){
                    result_obj["ResultItemId"] = resultItemID;
                    result_obj["InfTime"] = new_row[0].c_str();
                    result_obj["PredictedString"] = new_row[1].c_str();
                    body_data.append(result_obj);
                }
                title["data"] = body_data;
            }
            else if(inf_mode_status == INF_MODE_DET_CLA && m_nrt.use_count() > 0 && m_nrt_ensmble.use_count() > 0){

            }
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

    if (m_nrt->CAM_IDX != -1) {
        nrt::NDBuffer cam_colormap;
        output_cam = outputs.get_at(m_nrt->CAM_IDX);
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

    vector<float> cls_prob_vec(m_nrt->get_model_class_num(), 0);
    if(m_nrt->PROB_IDX != -1) {
        output_prob = outputs.get_at(m_nrt->PROB_IDX);
        nrt::Shape output_prob_shape = output_prob.get_shape();

        for(int i=0; i < output_prob_shape.dims[0]; i++) {
            qDebug() << "probability value for each class : ";
            for (int j=0; j < output_prob_shape.dims[1]; j++) {
                float prob = *output_prob.get_at_ptr<float>(i, j);
                cls_prob_vec[j] = prob;
                qDebug() << m_nrt->get_model_class_name(j) << " - " << prob;
            }
        }
    }

    std::string pred_cls = "";
    if(m_nrt->PRED_IDX != -1) {
        output_pred = outputs.get_at(m_nrt->PRED_IDX);
        nrt::Shape output_pred_shape = output_pred.get_shape();

        int pred_cla_idx;
        for (int i = 0; i < output_pred_shape.dims[0]; i++) {
            pred_cla_idx = *output_pred.get_at_ptr<int>(i);
            qDebug() << "Prediction class index(Not thresholded): " << QString::number(pred_cla_idx);
        }
        QString pred_cla_name = m_nrt->get_model_class_name(pred_cla_idx);

        //float cur_prob_thres = ui->tableWidget_class->item(pred_cla_idx, NAME_COL+1)->text().left(num_idx).toFloat();
        float cur_prob_thres;
        if(pred_cla_idx < m_nrt->prob_threshold.length()){
            cur_prob_thres = m_nrt->prob_threshold[pred_cla_idx];
        }
        else{
            qDebug() << "claSetResults) " << "prob_threshold vector out of range";
            cur_prob_thres = 90;
        }

        if ((cls_prob_vec[pred_cla_idx] * 100) < cur_prob_thres)
            pred_cla_name = QString("Unknown");

        ui->edit_show_class->setText(pred_cla_name);

        pred_cls = pred_cla_name.toStdString();
    }

    new_row.push_back(pred_cls);

    for (int cls_idx = 0; cls_idx < cls_prob_vec.size(); cls_idx++) {
        float cur_rate = cls_prob_vec[cls_idx] * 100;
        if (cur_rate == 0) {
            new_row.push_back(std::string("0"));
            continue;
        }
        std::ostringstream out;
        out.precision(2);
        out << std::fixed << cur_rate;
        new_row.push_back(out.str());
    }
}

void MainWindow::segSetResults(nrt::NDBuffer merged_pred_output, cv::Mat &PRED_IMG, vector<std::string> &new_row){
    vector<int> seg_blob_cnt(m_nrt->get_model_class_num()-1);
    vector<float> seg_pixel_rate(m_nrt->get_model_class_num()-1, 0);
    vector<long long> seg_pixel_cnt(m_nrt->get_model_class_num()-1, 0);
    long long total_pixel;

    if (!merged_pred_output.empty()) {
        nrt::Status status;

        // Threshold by size
        nrt::NDBuffer bounding_rects;
        nrt::NDBuffer size_threshold_buf = m_nrt->get_model_size_threshold();

        if (size_threshold_buf.empty()) {
            size_threshold_buf = nrt::NDBuffer::zeros(nrt::Shape(m_nrt->get_model_class_num(), 3), nrt::DTYPE_INT32);
            int* thres_ptr = size_threshold_buf.get_at_ptr<int>();

            for(int i = 0; i < m_nrt->get_model_class_num(); i++){
              int height = m_nrt->size_threshold[i].first;
              int width = m_nrt->size_threshold[i].second;
              int conjunction = m_nrt->size_thres_conjunction[i] == "AND" ? 0 : 1;
              thres_ptr[3*i + 0] = height;
              thres_ptr[3*i + 1] = width;
              thres_ptr[3*i + 2] = conjunction;
            }
        }
        status = nrt::pred_map_threshold_by_size(merged_pred_output, bounding_rects, size_threshold_buf, m_nrt->get_model_class_num());
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
//            int image_batch_index = output_rect_ptr[0];
            int rect_x = output_rect_ptr[1];
            int rect_y = output_rect_ptr[2];
//            int rect_h = output_rect_ptr[3];
//            int rect_w = output_rect_ptr[4];
            int rect_class_index = output_rect_ptr[5];

            if (rect_class_index < 1 || rect_class_index > m_nrt->get_model_class_num()){
                continue;
            }

            seg_blob_cnt[rect_class_index-1] += 1;

            int r, g, b;

            COLOR_VECTOR[rect_class_index-1].getRgb(&r, &g, &b);

            cv::Scalar class_color_scalar = cv::Scalar(b, g, r);

            QString classname = m_nrt->get_model_class_name(rect_class_index);

            if (!classname.isEmpty()) {
                cv::putText(PRED_IMG, classname.toStdString(), cv::Point(rect_x, rect_y), FONT_HERSHEY_SIMPLEX, 0.6 , class_color_scalar, 1);
            }
        }

        vector<bool> exist_class(m_nrt->get_model_class_num());
        // Class Color Pixel in Result Image;
        unsigned char* output_ptr = merged_pred_output.get_at_ptr<unsigned char>(0);
        int img_h = PRED_IMG.rows;
        int img_w = PRED_IMG.cols;
        total_pixel = img_h * img_w;
        int cur_ofs, cur_class;
        double alp_src = 0.5, alp_mask = 0.5;

        nrt::Shape shape = merged_pred_output.get_shape();
        qDebug() << "merged pred output shape: [";
        for(int i=0; i < shape.num_dim; i++){
            qDebug() << shape.dims[i] << " ";
        }
        qDebug() << "]";

        for (int h = 0; h < img_h; h++) {
            cur_ofs = h * img_w;
            for (int w = 0; w < img_w; w++) {
                cur_class = output_ptr[cur_ofs + w];
                if (cur_class < 1 || cur_class >= m_nrt->get_model_class_num()){
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
        for(int i = 0; i < m_nrt->get_model_class_num(); i++){
            if(exist_class[i]){
                if(exist_class_string != QString("")){
                    exist_class_string += ", ";
                }
                exist_class_string += m_nrt->get_model_class_name(i);
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
            new_row.push_back(std::string("0"));
            continue;
        }
        std::ostringstream out;
        out.precision(2);
        out << std::fixed << cur_rate;
        new_row.push_back(out.str());
    }
}

void MainWindow::detSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row){
    vector<int> det_box_cnt(m_nrt->get_model_class_num()-1, 0);
    vector<float> det_box_prob(m_nrt->get_model_class_num()-1, 0);
    vector<bool> exist_class(m_nrt->get_model_class_num());

    if ((m_nrt->PRED_IDX != -1) && (m_nrt->PROB_IDX != -1)) {
        nrt::NDBuffer output_boxes = outputs.get_at(m_nrt->PRED_IDX);
        nrt::NDBuffer output_prob = outputs.get_at(m_nrt->PROB_IDX);
        nrt::Shape output_boxes_shape = output_boxes.get_shape();

        nrt::Shape input_image_shape = m_nrt->get_model_input_shape(0);
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
            const float* probs = prob_ptr + box_idx * (m_nrt->get_model_class_num() + 1);

            if (bbox.class_number < 1 || bbox.class_number > m_nrt->get_model_class_num()){
                qDebug() << "Detection Box class is either background or out of range.";
                continue;
            }

            // Treshold by size
            int h_thres, w_thres, a_thres;
            h_thres = m_nrt->size_threshold[bbox.class_number].first;
            w_thres = m_nrt->size_threshold[bbox.class_number].second;
            a_thres = m_nrt->size_thres_conjunction[bbox.class_number] == "AND" ? 0 : 1;

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
            float cur_prob_thres = m_nrt->prob_threshold[bbox.class_number];
            if (cur_prob < cur_prob_thres){
                continue;
            }

            exist_class[bbox.class_number] = true;
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
            QString classname = m_nrt->get_model_class_name(bbox.class_number);

            std::ostringstream out;
            out.precision(2);
            out << std::fixed << cur_prob;
            string classprob = string("(") + out.str() + string("%)");
            classname += QString::fromStdString(classprob);

            if (!classname.isEmpty()) {
                cv::putText(PRED_IMG, classname.toLocal8Bit().constData(), cv::Point(bbox.box_center_X - bbox.box_width / 2, bbox.box_center_Y - bbox.box_height / 2), FONT_HERSHEY_SIMPLEX, 1, white, 7);
                cv::putText(PRED_IMG, classname.toLocal8Bit().constData(), cv::Point(bbox.box_center_X - bbox.box_width / 2, bbox.box_center_Y - bbox.box_height / 2), FONT_HERSHEY_SIMPLEX, 1, class_color_scalar, 4);
            }
        }

        // Class Name in edit_show_class
        QString exist_class_string("");
        int exist_class_cnt = 0;
        for(int i = 0; i < m_nrt->get_model_class_num(); i++){
            if(exist_class[i]){
                exist_class_cnt ++;

                if (exist_class_string != QString("")) {
                    if (exist_class_cnt == 5)
                        exist_class_string += ",\n";
                    else
                        exist_class_string += ", ";
                }
                exist_class_string += m_nrt->get_model_class_name(i);
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
                new_row.push_back(std::string("0"));
                continue;
            }
            float cur_cla_prob = prob_sum / (float)det_box_cnt[cla_idx];

            std::ostringstream out;
            out.precision(2);
            out << std::fixed << cur_cla_prob;
            new_row.push_back(out.str());
        }
    }
}

void MainWindow::ocrSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row) {
    if ((m_nrt->PRED_IDX != -1)) {
        nrt::Shape input_image_shape = m_nrt->get_model_input_shape(0);
        int input_h = PRED_IMG.rows;
        int input_w = PRED_IMG.cols;
        double h_ratio = (double)input_image_shape.dims[0] / input_h;
        double w_ratio = (double)input_image_shape.dims[1] / input_w;

        nrt::NDBuffer output_boxes;
        nrt::Shape output_pred_shape;

        output_boxes = outputs.get_at(m_nrt->PRED_IDX);
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

            if(bbox.class_number < 1 || bbox.class_number >  m_nrt->get_model_class_num()){
                qDebug() << "OCR Box class is either background or out of range.";
                continue;
            }
            const char* classname = m_nrt->get_model_class_name(bbox.class_number).toStdString().c_str();
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
        /*for (int r = 0; r < ui->tableWidget_class->rowCount(); r++) {
            int colCount = ui->tableWidget_class->columnCount();
            if ((r == 3) || (r == 6))
                colCount = 6;
            for (int c = 0; c < colCount; c++) {
                ui->tableWidget_class->item(r, c)->setBackgroundColor(QColor(255, 255, 255));
                ui->tableWidget_class->item(r, c)->setTextColor(QColor(0, 0, 0));
            }
        }
        for (int i = 0; i < exist_bbox.size(); i++) {
            const char* classname = m_nrt->get_model_class_name(exist_bbox[i].class_number).toStdString().c_str();
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
            QString classname = m_nrt->get_model_class_name(exist_bbox[i].class_number);
            ocr_string += classname;
        }
        ui->edit_show_class->setText(ocr_string);
        new_row.push_back(ocr_string.toStdString());*/
    }
}

void MainWindow::anomSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row) {

}

void MainWindow::detClaEnsmbleSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row){
    // m_nrt: Detection model, m_nrt_ensmble: Classification model
    if(m_nrt->PRED_IDX != -1 && m_nrt->PROB_IDX != -1 && m_nrt_ensmble->PRED_IDX != -1 && m_nrt_ensmble->PROB_IDX != -1){
        nrt::NDBuffer det_output_boxes = outputs.get_at(m_nrt->PRED_IDX);
        nrt::NDBuffer det_output_prob = outputs.get_at(m_nrt->PROB_IDX);

        nrt::Shape det_output_boxes_shape = det_output_boxes.get_shape();
        const int num_of_boxes = det_output_boxes_shape.dims[0];
        const int* bcyxhw_ptr = det_output_boxes.get_data_ptr<int>();
        const float* prob_ptr = det_output_prob.get_data_ptr<float>();

        nrt::Shape det_input_image_shape = m_nrt->get_model_input_shape(0);
        int det_input_h = PRED_IMG.rows;
        int det_input_w = PRED_IMG.cols;
        double h_ratio = (double)det_input_image_shape.dims[0] / det_input_h;
        double w_ratio = (double)det_input_image_shape.dims[1] / det_input_w;

        if(num_of_boxes == 0){
            ui->edit_show_class->setText("None");
        }

        cv::Mat intact_frame;
        PRED_IMG.copyTo(intact_frame);

        vector<bool> exist_class(m_nrt_ensmble->get_model_class_num(), false);  
        vector<int> num_of_classified_boxes(m_nrt_ensmble->get_model_class_num(), 0);
        vector<float> class_accum_probs(m_nrt_ensmble->get_model_class_num(), 0.0);

        vector<int> num_of_detected_boxes(m_nrt->get_model_class_num(), 0);
        vector<float> det_accum_probs(m_nrt->get_model_class_num(), 0.0);

        for(int box_idx = 0; box_idx < num_of_boxes; box_idx++){
            BoundingBox bbox = convert_to_bounding_box(bcyxhw_ptr + box_idx * 6, h_ratio, w_ratio);

            if (bbox.class_number < 1 || bbox.class_number > m_nrt->get_model_class_num()){
                qDebug() << "Detection Box class is either background or out of range.";
                continue;
            }

            // Treshold by size
            int h_thres, w_thres, a_thres;
            if(bbox.class_number < m_nrt->size_threshold.length() && bbox.class_number < m_nrt->size_thres_conjunction.length()){
                h_thres = m_nrt->size_threshold[bbox.class_number].first;
                w_thres = m_nrt->size_threshold[bbox.class_number].second;
                a_thres = m_nrt->size_thres_conjunction[bbox.class_number] == "AND" ? 0 : 1;

                if (a_thres == 0) { // AND
                    if ((bbox.box_height < h_thres) && (bbox.box_width < w_thres))
                        continue;
                }
                else {              // OR
                    if ((bbox.box_height < h_thres) || (bbox.box_width < w_thres))
                        continue;
                }
            }

            // Threshold by prob
            const float* box_probs = prob_ptr + box_idx*(m_nrt->get_model_class_num() + 1);
            float cur_box_pred_class_prob = box_probs[bbox.class_number+1] * 100;

            if(bbox.class_number < m_nrt->prob_threshold.length()){
                float cur_prob_thres = m_nrt->prob_threshold[bbox.class_number];
                if (cur_box_pred_class_prob < cur_prob_thres){
                    continue;
                }
            }

            if(bbox.class_number < num_of_detected_boxes.size() && bbox.class_number < det_accum_probs.size()){
                num_of_detected_boxes[bbox.class_number] ++;
                det_accum_probs[bbox.class_number] += cur_box_pred_class_prob;
            }

            int x_start = bbox.box_center_X - ensmble_crop_size/2;
            int y_start = bbox.box_center_Y - ensmble_crop_size/2;

            if(x_start < 0) {
                x_start = 0;
            }
            if(x_start + ensmble_crop_size > PRED_IMG.cols){
                x_start = PRED_IMG.cols - ensmble_crop_size;
            }
            if(y_start < 0){
                y_start = 0;
            }
            if(y_start + ensmble_crop_size > PRED_IMG.rows){
                y_start = PRED_IMG.rows - ensmble_crop_size;
            }

            cv::Rect bounds(x_start, y_start, ensmble_crop_size, ensmble_crop_size);
            cv::Mat cropped_cla_img = intact_frame(bounds);
            cv::Mat img_to_infer;
            cv::cvtColor(cropped_cla_img, img_to_infer, cv::COLOR_BGR2RGB);

            nrt::NDBufferList cla_outputs;
            nrt::NDBuffer cla_output_prob;
            nrt::NDBuffer cla_output_pred;

            nrt::NDBuffer resized_img_buff = get_img_buffer(img_to_infer, m_nrt_ensmble.get());
            cla_outputs = m_nrt_ensmble->execute(resized_img_buff);
            cla_output_pred = cla_outputs.get_at(m_nrt_ensmble->PRED_IDX);
            cla_output_prob = cla_outputs.get_at(m_nrt_ensmble->PROB_IDX);

            int pred_cls_idx = -1;
            nrt::Shape output_pred_shape = cla_output_pred.get_shape();
            for (int i = 0; i < output_pred_shape.dims[0]; i++) {
                pred_cls_idx = *cla_output_pred.get_at_ptr<int>(i);
            }

            // Threshold by prob
            QString classname = m_nrt_ensmble->get_model_class_name(pred_cls_idx);
            float pred_cls_prob = *cla_output_prob.get_at_ptr<float>(0, pred_cls_idx) * 100;

            if(pred_cls_idx < m_nrt_ensmble->prob_threshold.length()){
                float cur_cla_prob_thres;
                if(pred_cls_idx < m_nrt_ensmble->prob_threshold.length()){
                    cur_cla_prob_thres = m_nrt_ensmble->prob_threshold[pred_cls_idx];
                }
                else{
                    qDebug() << "claSetResults) " << "prob_threshold vector out of range";
                    cur_cla_prob_thres = 90;
                }

                if (pred_cls_prob < cur_cla_prob_thres){
                    classname = "Unknown";
                }
            }

            qDebug() << "Predicted class: " << classname << ", Detection prob: " << QString::number(cur_box_pred_class_prob) << ", Classification Prob: " << QString::number(pred_cls_prob);

            cv::Scalar class_color_scalar;
            if(classname != "Unknown"){
                if(pred_cls_idx < m_nrt_ensmble->get_model_class_num()){
                    exist_class[pred_cls_idx] = true;
                    num_of_classified_boxes[pred_cls_idx]++;
                    class_accum_probs[pred_cls_idx] += pred_cls_prob;
                }
                if(pred_cls_idx < class_ratio.length()){
                    class_ratio[pred_cls_idx] ++;
                }
                int r, g, b;
                COLOR_VECTOR[pred_cls_idx].getRgb(&r, &g, &b);
                class_color_scalar = cv::Scalar(b, g, r);
            }
            else{
                class_color_scalar = cv::Scalar(0, 0, 0);
            }

            cv::rectangle(PRED_IMG,
                          cv::Point(bbox.box_center_X - bbox.box_width/2, bbox.box_center_Y - bbox.box_height/2),
                          cv::Point(bbox.box_center_X + bbox.box_width/2 + (bbox.box_width % 2), bbox.box_center_Y + bbox.box_height / 2 + (bbox.box_height % 2)),
                          class_color_scalar,
                          1);

            /*TOD 한글 클래스명 때문에 예외 처리*/
            if(classname == QString::fromLocal8Bit("사람")){
                classname = "people";
            }
            else if(classname == QString::fromLocal8Bit("동물")){
                classname = "animal";
            }
            else if(classname == QString::fromLocal8Bit("차량")){
                classname = "car";
            }

            //classname += "(" + QString::number(cur_box_pred_class_prob) + "%, " +QString::number(pred_cls_prob) +"%)";

            if (!classname.isEmpty()) {
                cv::putText(PRED_IMG, classname.toStdString(), cv::Point(bbox.box_center_X - bbox.box_width / 2, bbox.box_center_Y - bbox.box_height / 2), FONT_HERSHEY_SIMPLEX, 1, white, 4);
                cv::putText(PRED_IMG, classname.toStdString(), cv::Point(bbox.box_center_X - bbox.box_width / 2, bbox.box_center_Y - bbox.box_height / 2), FONT_HERSHEY_SIMPLEX, 1, class_color_scalar, 2);
            }
        }

        QString show_class_str;
        for(int i=0; i < m_nrt_ensmble->get_model_class_num(); i++){
            if(exist_class[i]){
                if(show_class_str != ""){
                    show_class_str += ", ";
                }
                show_class_str += m_nrt_ensmble->get_model_class_name(i);
            }
        }
        ui->edit_show_class->setText(QString(" ") + show_class_str);

        // update pie chart
        int total_boxes = 0;
        for(int val : class_ratio){
            total_boxes += val;
        }
        for(int i=0; i < m_nrt_ensmble->get_model_class_num(); i++){
            if(i < series->slices().length() && i < class_ratio.length()){
                float ratio = (float)class_ratio[i] / total_boxes;
                series->slices().at(i)->setValue(ratio);
            }
        }

        // update prob table
        for(int i = 1; i < det_accum_probs.size(); i++){
            float det_avg_prob = num_of_detected_boxes[i] == 0 ? 0 : det_accum_probs[i] / num_of_detected_boxes[i];
            if(ui->table_model1_class_probs->item(i - 1, 1)){
                ui->table_model1_class_probs->item(i - 1, 1)->setText(QString::number(det_avg_prob) + "%");
            }
        }

        for(int i = 0; i < class_accum_probs.size(); i++){
            if(exist_class[i]){
                float cla_avg_prob = num_of_classified_boxes[i] == 0 ? 0 : class_accum_probs[i] / num_of_classified_boxes[i];
                if(ui->table_model2_class_probs->item(i, 2)){
                    ui->table_model2_class_probs->item(i, 2)->setText(QString::number(cla_avg_prob) + "%");
                }
            }
            else{
                if(ui->table_model2_class_probs->item(i, 2)){
                    ui->table_model2_class_probs->item(i, 2)->setText("0%");
                }
            }
        }
    }
}

void MainWindow::showResult() {
    cv::Mat ORG_IMG, PRED_IMG;
    Mat_With_Name org_mwn, pred_mwn;
    QString cur_inf_img_path;

    vector<std::string> new_row;
    QString cur_img_name;

    int cur_mode = ui->Mode_Setting_Stack->currentIndex();

    bool img_mode_end = false;
    if (cur_mode == 0) {
        // Realtime Camera Mode
        if (ui->com_cam_input_select->currentText().toStdString() == CAMERA_TEXT) {
            if (!m_usbCam->isExist()) {
                if(m_timer)
                    m_timer->stop();
                on_btn_cam_stop_clicked();
                return;
            }

            m_usbCam->setResult();
            if (m_usbCam->m_frame.empty()) {
                if(m_timer)
                    m_timer->stop();
                on_btn_cam_stop_clicked();
                return;
            }

            ORG_IMG = m_usbCam->m_frame.clone();
        }

        // Video Input Mode
        else if (ui->com_cam_input_select->currentText().toStdString() == SINGLE_VIDEO_TEXT || ui->com_cam_input_select->currentText().toStdString() == VIDEO_FOLDER_TEXT){
            if(!m_videoInputCap.isOpened()){
                qDebug() << "Video capture is not opened.";
                return;
            }
            m_videoInputCap >> ORG_IMG;

            if(ORG_IMG.empty()){
                qDebug() << "Video is over!";
                QMessageBox::information(this, "Notification", "The video is over.");
                m_videoInputCap.release();

                if(m_timer)
                    m_timer->stop();
                on_btn_cam_stop_clicked();
                return;
            }
        }

        QString time_format = "yyMMdd_HHmmss_zzz";
        QDateTime now = QDateTime::currentDateTime();
        std::string time_str = now.toString(time_format).toStdString();
        cur_img_name = QString::fromStdString(time_str + IMG_FORMAT);
    }

    else if (cur_mode == 1) {
        int cur_inf_img_idx = ui->spb_img_cur_idx->value() - 1;
        org_mwn.imageId = inf_image_id[cur_inf_img_idx];
        pred_mwn.imageId = inf_image_id[cur_inf_img_idx];

        if (cur_inf_img_idx < 0 || cur_inf_img_idx >= inf_image_list.size()) {
            if(m_timer)
                m_timer->stop();
            return;
        }

        if (!(macro_flag && !macro_cam_flag)){
            ui->spb_img_cur_idx->setValue(cur_inf_img_idx + 1);
            ui->com_image_list->setCurrentIndex(cur_inf_img_idx + 1);
        }

//        if (cur_inf_img_idx == 0 && macro_cam_flag)
//            ui->spb_img_cur_idx->setRange(1, ui->spb_img_cur_idx->maximum());

        cur_inf_img_path = inf_image_list[cur_inf_img_idx];
        cur_img_name = cur_inf_img_path.split(QDir::separator()).last();
        qDebug() << "[" << cur_inf_img_idx << "] " << inf_image_list[cur_inf_img_idx];

        ORG_IMG = cv::imread(cur_inf_img_path.toLocal8Bit().constData(), cv::IMREAD_COLOR);
        qDebug() << "Show Result) Imread";

        if (ORG_IMG.empty()) {
            if(m_timer)
                m_timer->stop();
            return;
        }
        if (cur_inf_img_idx == (inf_image_list.size()-1)) {
            if (macro_flag) {
//                std::random_shuffle(inf_image_list.begin(),inf_image_list.end());
//                ui->spb_img_cur_idx->setRange(1, inf_image_list.size());
                ui->spb_img_cur_idx->setValue(1);
            }
            else {
                if(m_timer){
                    m_timer->stop();
                }
                img_mode_end = true;
            }
        }
    }

    PRED_IMG = ORG_IMG.clone();

    nrt::NDBufferList outputs;
    nrt::NDBuffer merged_pred_output;
//    nrt::NDBuffer merged_prob_output;

    std::chrono::duration<double, std::milli> inf_time;
    QString model_type;

    if(ready_for_inference){
        if(m_nrt.use_count() > 0 && is_ready_for_inf(m_nrt.get()))
        model_type = m_nrt->get_model_type();

        if(model_type == "Segmentation"){
            // Non patch mode 이미지 리사이즈 처리
            if(!m_nrt->is_model_patch_mode()){
                nrt::Shape model_input_shape = m_nrt->get_model_input_shape(0);
                int h = model_input_shape.dims[0];
                int w = model_input_shape.dims[1];

                nrt::InterpolationType inter = m_nrt->get_model_interpolty(0);
                if(inter == nrt::INTER_NEAREST){
                    cv::resize(ORG_IMG, ORG_IMG, cv::Size(w, h), 0, 0, cv::INTER_NEAREST);
                    PRED_IMG = ORG_IMG.clone();
                }
                else if(inter == nrt::INTER_LINEAR){
                    cv::resize(ORG_IMG, ORG_IMG, cv::Size(w, h), 0, 0, cv::INTER_LINEAR);
                    PRED_IMG = ORG_IMG.clone();
                }
                else if(inter == nrt::INTER_AREA){
                    cv::resize(ORG_IMG, ORG_IMG, cv::Size(w, h), 0, 0, cv::INTER_AREA);
                    PRED_IMG = ORG_IMG.clone();
                }
                else if(inter == nrt::INTER_CUBIC){
                    cv::resize(ORG_IMG, ORG_IMG, cv::Size(w, h), 0, 0, cv::INTER_CUBIC);
                    PRED_IMG = ORG_IMG.clone();
                }
                else{
                    return;
                }
            }

            nrt::NDBuffer img_buffer = get_img_buffer(ORG_IMG, m_nrt.get());
            merged_pred_output = seg_execute(img_buffer, inf_time, m_nrt.get());
            QString inf_time_str = QString::number(inf_time.count(), 'f', 3);
            ui->edit_show_inf->setText(inf_time_str);
            new_row.push_back(inf_time_str.toStdString());
        }
        else if(model_type == "Classification" || model_type == "Detection" || model_type == "OCR" || model_type == "Anomaly") {
            nrt::NDBuffer resized_img_buffer = get_img_buffer(ORG_IMG, m_nrt.get());

            auto start = std::chrono::high_resolution_clock::now();
            outputs = m_nrt->execute(resized_img_buffer);
            auto end = std::chrono::high_resolution_clock::now();

            std::chrono::duration<double, std::milli> inf_time = end - start;
            QString inf_time_str = QString::number(inf_time.count(), 'f', 3);
            ui->edit_show_inf->setText(inf_time_str);
            new_row.push_back(inf_time_str.toStdString());
        }

        if (inf_mode_status == INF_MODE_SINGLE) {
            if(model_type == "Classification") {
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
        else if(inf_mode_status == INF_MODE_DET_CLA && m_nrt_ensmble.use_count() > 0 && is_ready_for_inf(m_nrt_ensmble.get())){
            detClaEnsmbleSetResults(outputs, PRED_IMG, new_row);
        }
    }
    else{
        qDebug() << "Show Result) Models are not loaded.";
    }

    cv::cvtColor(ORG_IMG, ORG_IMG, COLOR_RGB2BGR);
    cv::cvtColor(PRED_IMG, PRED_IMG, COLOR_RGB2BGR);

    org_mwn.image = ORG_IMG.clone();
    pred_mwn.image = PRED_IMG.clone();

    if (cur_mode == 0) {        // CAM Mode
        if (cam_autosave_flag && cur_save_flag) {
            cur_save_flag = false;
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
    else if (cur_mode == 1 && img_save_flag) {
        qDebug() << "IMG Mode) Push Mat to buffer";
        std::unique_lock<std::mutex> lock(m_save_info._mutex);

        if(!macro_flag){
            pred_mwn.name = cur_img_name.toStdString();

            m_save_info.pred_buffer.push(pred_mwn);
            m_save_info.row_buffer.push(new_row);
        }
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

    if(img_mode_end){
        img_inf_ing_flag = false;
        on_com_image_list_currentIndexChanged(0);

        ui->spb_img_cur_idx->setReadOnly(false);
        ui->com_image_list->setEditable(true);

        ui->cbx_set_show_time->setEnabled(true);
        ui->cbx_set_show_time->setChecked(false);

        ui->btn_img_play->setEnabled(false);
        ui->btn_img_pause->setEnabled(false);

        // save evaluation json file
        {
            std::unique_lock<std::mutex> lock(m_save_info._mutex);

            if(!m_save_info.json_file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
                qDebug()<<"Failed to open file.";
                return;
            }

            m_save_info.json_doc.setObject(m_save_info.title);
            m_save_info.json_file.write(m_save_info.json_doc.toJson());
            m_save_info.json_file.close();
        }

        QMessageBox::information(this, "Notification",
                    "The image inference is completed.\nYou can check the inference results if you chose save mode.",
                    QMessageBox::Ok);
    }
}

void MainWindow::initJson(int evaluationSetId){
    std::unique_lock<std::mutex> lock(m_save_info._mutex);

    QJsonObject& title = m_save_info.title;
    QFile& file = m_save_info.json_file;

    if(inf_mode_status == INF_MODE_SINGLE && m_nrt.use_count() > 0){
        QString suffix = "";
        if(m_nrt->get_model_type() == "Classification"){
            suffix = "_NumOfImages";
        }
        else if(m_nrt->get_model_type() == "Detection" || m_nrt->get_model_type() == "OCR" || m_nrt->get_model_type() == "Anomaly"){
            suffix = "_NumOfDetectedBoxes";
        }
        else if(m_nrt->get_model_type() == "Segmentation"){
            suffix = "_NumOfRegions";
        }
        QString class_name;
        for(int i=0; i < m_nrt->get_model_class_num(); i++){
            class_name = m_nrt->get_model_class_name(i);
            title[class_name + suffix] = 0;
        }
    }
    else if(inf_mode_status == INF_MODE_DET_CLA){

    }

    title["evaluationSetId"] = evaluationSetId;
    title["numOfItems"] = 0;
    title["avgInfTime"] = 0;
    title["data"] = QJsonArray();

    m_save_info.json_doc.setObject(title);

    QString path = m_db->getEvalutionJsonPath(evaluationSetId);
    file.setFileName(path);
}

bool MainWindow::configSaveSettings(){
    // Setting up Cam Save Configuration
    int imageSetId = -1, evaluationSetId;
    bool newImagesetFlag;

    QSqlDatabase db = QSqlDatabase::database("main_thread");
    QSqlTableModel* imageSetTableModel = new QSqlTableModel(this, db);
    imageSetTableModel->setTable("ImageSets");
    imageSetTableModel->select();
    imageSetTableModel->setHeaderData(0, Qt::Horizontal, "Name");
    imageSetTableModel->setHeaderData(1, Qt::Horizontal, "Created On");
    imageSetTableModel->setHeaderData(2, Qt::Horizontal, "Updated On");
    imageSetTableModel->setHeaderData(3, Qt::Horizontal, "Number of Images");

    QTableView* view = new QTableView;
    view->setModel(imageSetTableModel);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->hideColumn(0);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSelectionMode(QAbstractItemView::SingleSelection);

    QDialog dialog(this);

    QVBoxLayout* root_cont = new QVBoxLayout;

    QFormLayout* modelform = new QFormLayout;
    modelform->addRow(new QLabel("Selected Model Info (exit the dialog to re-select model)"));
    modelform->addRow(new QLabel("Model Name: "), new QLabel("Dummy"));
    modelform->addRow(new QLabel("Model Type: "), new QLabel("Dummy"));
    root_cont->addLayout(modelform);
    root_cont->addSpacing(10);

    QGroupBox* groupbox = new QGroupBox;
    QRadioButton* newImageset = new QRadioButton;
    newImageset->setText("Create New Image Set to Save to.");
    newImageset->setChecked(true);
    newImagesetFlag = true;
    QRadioButton* existingImageset = new QRadioButton;
    existingImageset->setText("Select Existing Image Set to Save to.");
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(newImageset);
    vbox->addWidget(existingImageset);
    vbox->addStretch(1);
    groupbox->setLayout(vbox);
    root_cont->addWidget(groupbox);
    root_cont->addWidget(view);

    QDialogButtonBox*btnbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                   Qt::Horizontal, &dialog);
    connect(btnbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(btnbox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    root_cont->addWidget(btnbox);
    dialog.setLayout(root_cont);

    if(dialog.exec() == QDialog::Accepted){
        qDebug() << "Config Save Settings) Accepted";
        if(existingImageset->isChecked()){
            int rowID = view->selectionModel()->currentIndex().row();
            imageSetId = view->model()->index(rowID, 0).data().toInt();
            newImagesetFlag = false;
        }
        else if (newImageset->isChecked()){
            qDebug() << "Config Save Settings) New Image Set selected.";
            newImagesetFlag = true;
        }
    }
    else{
        ui->rad_cam_autosave->setChecked(false);
        qDebug() << "Config Save Settings) Rejected";
        return false;
    }

    ui->rad_cam_rtmode->setChecked(false);
    cam_autosave_flag = true;

    if(newImagesetFlag){
        QString imageSetName;
        if(media_mode == MEDIA_MODE_VIDEO_FILE || media_mode == MEDIA_MODE_VIDEO_FOLDER){
            imageSetName = "VIDEO_" + video_filename + "_ImageSet";
        }
        else if(media_mode == MEDIA_MODE_CAM){
            imageSetName = "CAM_" + QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss") + "_ImageSet";
        }
        else{
            qDebug() << "Config Save Settings) Video mode or Cam mode? Flag is not set.";
            return false;
        }

        bool ok;
        QString userInput = QInputDialog::getText(this, "Image Set Name",
                                                  "Image Set Name", QLineEdit::Normal,
                                                  imageSetName, &ok);
        if(ok && !userInput.isEmpty()){
            imageSetName = userInput;
        }

        imageSetId = m_db->InsertImageSet(imageSetName);
        if(imageSetId == -1){
            qDebug() << "Image set insert failed";
            return false;
        }
    }

    if(inf_mode_status == INF_MODE_SINGLE){
        evaluationSetId = m_db->getEvaluationSetID(imageSetId, currentModelId, -1);

        if(evaluationSetId == -1){
            evaluationSetId = m_db->InsertEvaluationSet(imageSetId, currentModelId, -1);
            if(evaluationSetId == -1){
                qDebug() << "Evaluation set insert failed";
                return false;
            }
            initJson(evaluationSetId);
        }

        else{
            {
                std::unique_lock<std::mutex> lock(m_save_info._mutex);
                QString path = m_db->getEvalutionJsonPath(evaluationSetId);

                QFile& file = m_save_info.json_file;
                file.setFileName(path);

                if(!file.open(QIODevice::ReadWrite)){
                    qDebug() << "Failed to open json file.";
                    return false;
                }

                QByteArray load_data = file.readAll();
                m_save_info.json_doc = QJsonDocument::fromJson(load_data);
                m_save_info.title = m_save_info.json_doc.object();
                file.close();
            }
        }
    }
    else if(inf_mode_status == INF_MODE_DET_CLA){
        evaluationSetId = m_db->getEvaluationSetID(imageSetId, currentModelId, ensmbleModelId);
    }

    {
        std::unique_lock<std::mutex> lock(m_save_info._mutex);
        m_save_info.imageSetId = imageSetId;
        m_save_info.evaluationSetId = evaluationSetId;
    }
    return true;
}

void MainWindow::on_rad_cam_rtmode_clicked()
{
    if(!ui->rad_cam_rtmode->isChecked()){
        ui->rad_cam_rtmode->setChecked(true);
        return;
    }
    ui->rad_cam_autosave->setChecked(false);
    cam_autosave_flag = false;
}

void MainWindow::on_rad_cam_autosave_clicked()
{
    if(!ui->rad_cam_autosave->isChecked()){
        ui->rad_cam_autosave->setChecked(true);
        return;
    }

    // 모델이 로드 되어있지 않으면 선택할 수 없게 막음.
    if(inf_mode_status == INF_MODE_NONE || m_nrt->get_model_status() == -1) {
        ui->rad_cam_autosave->setChecked(false);
        QMessageBox msgbox;
        msgbox.setText("You can not select save mode when model is not loaded.");
        msgbox.exec();
        return;
    }
    if(configSaveSettings()){
        ui->rad_cam_rtmode->setChecked(false);
        cam_autosave_flag = true;
    }
    else{
        ui->rad_cam_autosave->setChecked(false);
        QMessageBox msgbox;
        msgbox.setText("Failed to configure the save settings.");
        msgbox.exec();
        return;
    }
}

void MainWindow::on_btn_cam_mode_clicked()
{
    ui->btn_cam_mode->setDefault(true);
    ui->btn_img_mode->setDefault(false);

    ui->Mode_Setting_Stack->setCurrentIndex(0); // 0: Cam, 1: Img
}

void MainWindow::on_btn_img_mode_clicked()
{
    ui->btn_cam_mode->setDefault(false);
    ui->btn_img_mode->setDefault(true);

    if(ui->com_video_list->isVisible()){
        ui->com_video_list->clear();
        ui->com_video_list->hide();
        video_list.clear();

        if(m_videoInputCap.isOpened()){
            m_videoInputCap.release();
        }
    }
    media_mode = MEDIA_MODE_NONE;
    setCamControlEnabled(false);

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
            media_mode = MEDIA_MODE_NONE;
            return;
        }
        media_mode = MEDIA_MODE_CAM;

        showResult();
        setCamControlEnabled(true);
        on_btn_cam_play_clicked();
        ui->edit_show_inf->setText("");
    }

    // Video Input
    else if (ui->com_cam_input_select->currentText().toStdString() == SINGLE_VIDEO_TEXT){
        if(m_videoInputCap.isOpened()){
            m_videoInputCap.release();
        }

        QString video_filepath = QFileDialog::getOpenFileName(this,
                                                              tr("Select Input Video"),
                                                              QDir::homePath(), tr(" (*.avi *.mp4 *.MOV *.mpg *.mpeg)"));

        if(video_filepath.isEmpty()){
            QMessageBox::information(this, "Notification", "No video was selected");
            media_mode = MEDIA_MODE_NONE;
            return;
        }

        video_filename = video_filepath.split('/').last();

        qDebug() << video_filepath.toStdString().c_str();

        QString extension = video_filename.split('.').last();
        if(extension == "avi" || extension == "mp4" || extension == "MOV"){
            m_videoInputCap.open(video_filepath.toLocal8Bit().constData(), cv::CAP_MSMF);
        }
        else if(extension == "mpg" || extension == "mpeg"){
            m_videoInputCap.open(video_filepath.toLocal8Bit().constData(), cv::CAP_FFMPEG);
        }

        if(!m_videoInputCap.isOpened()){
            QMessageBox::information(this,
                                     "Notification",
                                     "Sorry. The video file is corupted.",
                                     QMessageBox::Ok);
            m_videoInputCap.release();
            media_mode = MEDIA_MODE_NONE;
            return;
        }

        video_list.append(video_filepath);
        ui->com_video_list->setVisible(true);
        ui->com_video_list->addItem(video_filename);
        media_mode = MEDIA_MODE_VIDEO_FILE;

        showResult();
        setCamControlEnabled(true);
        on_btn_cam_play_clicked();
    }

    else if(ui->com_cam_input_select->currentText().toStdString() == VIDEO_FOLDER_TEXT){
        if(m_videoInputCap.isOpened()){
            m_videoInputCap.release();
        }

        QString dir = QFileDialog::getExistingDirectory(this,
                                                        tr("Open Directory"),
                                                        "/home",
                                                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if(dir.isEmpty()){
            QMessageBox::information(this,
                                     "Notification",
                                     "No folder was selected",
                                     QMessageBox::Ok);
            media_mode = MEDIA_MODE_NONE;
            return;
        }

        QDirIterator itr(dir,
                         {"*.mp4", "*.avi", },
                         QDir::Files,
                         QDirIterator::Subdirectories);

        video_list.clear();
        ui->com_video_list->clear();
        ui->com_video_list->setVisible(true);

        QString video_path, video_name;
        while(itr.hasNext()){
            video_path = itr.next();
            video_name = video_path.split("/").last();
            video_list.append(video_path);
            ui->com_video_list->addItem(video_name);
        }

        QString cur_video_path = video_list[ui->com_video_list->currentIndex()];

        m_videoInputCap.open(cur_video_path.toLocal8Bit().constData(), cv::CAP_MSMF);

        if(!m_videoInputCap.isOpened()){
            QMessageBox::information(this,
                                     "Notification",
                                     "Sorry. The video file is corrupted.",
                                     QMessageBox::Ok);
            m_videoInputCap.release();
            media_mode = MEDIA_MODE_NONE;
            return;
        }

        media_mode = MEDIA_MODE_VIDEO_FOLDER;

        showResult();
        setCamControlEnabled(true);
        on_btn_cam_play_clicked();
    }
}

void MainWindow::on_btn_cam_play_clicked()
{
    qDebug() << "come cam play";

    if (media_mode == MEDIA_MODE_CAM && m_usbCam.use_count() <= 0) {
        qDebug() << "Cam mode but camera is not open.";
        return;
    }

    else if(media_mode == MEDIA_MODE_VIDEO_FILE || media_mode == MEDIA_MODE_VIDEO_FOLDER){
        if(!m_videoInputCap.isOpened()){
            on_com_video_list_currentTextChanged(ui->com_video_list->currentText());
        }
    }

    else if(media_mode == MEDIA_MODE_NONE){
        return;
    }

    if (m_timer.use_count() <= 0) { // First Connect with showResult()
        qDebug() << "CAM Mode) Play";

        if(ui->rad_cam_rtmode->isChecked()){
            if(m_save_timer.use_count() > 0){
                m_save_timer.reset();
            }
            ui->rad_cam_autosave->setEnabled(false);
        }
        else if(ui->rad_cam_autosave->isChecked()){
            if (m_save_timer.use_count() == 0)
                m_save_timer = make_shared<QTimer>(this);
            connect(m_save_timer.get(), SIGNAL(timeout()), this, SLOT(setSaveStautus()));
            m_save_timer->setInterval(1000);
            m_save_timer->start();

            ui->rad_cam_rtmode->setEnabled(false);
        }

        setCamSelectEnabled(false);

        if (m_timer.use_count() == 0)
            m_timer = make_shared<QTimer>(this);

        connect(m_timer.get(), SIGNAL(timeout()), this, SLOT(showResult()));

        m_timer->setInterval(0);
        m_timer->start();

        if(m_usbCam.use_count() > 0 )
            m_usbCam->playCam();

        setTabelColumn(true);
    }
    else {          // Already Connected
        qDebug() << "CAM Mode) Replay";

        if(ui->rad_cam_rtmode->isChecked()){
            if (m_save_timer.use_count() > 0)
                m_save_timer.reset();
        }
        else if (ui->rad_cam_autosave->isChecked()) {
            if (m_save_timer.use_count() == 0)
                m_save_timer = make_shared<QTimer>(this);
            connect(m_save_timer.get(), SIGNAL(timeout()), this, SLOT(setSaveStautus()));
            m_save_timer->setInterval(1000);
            m_save_timer->start();
        }

        setCamSaveChangeEnabled(false);
        m_timer->setInterval(0);
        m_timer->start();
    }
    ui->btn_img_mode->setEnabled(false);
    ui->btn_select_single_mode->setEnabled(false);
    ui->btn_select_ensmble_mode->setEnabled(false);

    ui->com_video_list->setEnabled(false);
}

void MainWindow::on_btn_cam_pause_clicked()
{
    if (m_timer.use_count() <= 0)
        return;
    if (m_timer->isActive()) {
        qDebug() << "CAM Mode) Pause";
        m_timer->stop();

    }

    if (m_save_timer.use_count() > 0)
        m_save_timer->stop();
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
        media_mode = MEDIA_MODE_NONE;
        m_usbCam.reset();
    }

    if(m_videoInputCap.isOpened()){
        qDebug() << "VIDEO Mode) Delete Video Capture";
        m_videoInputCap.release();
    }

    setCamSelectEnabled(true);
    if(ui->com_cam_input_select->currentText().toStdString() == CAMERA_TEXT){
        setCamControlEnabled(false);
    }

    ui->lab_result_info->setText("There is no table to show.");
    setTabelColumn(false);

    ui->lab_show_res->setText("Please Select CAM or IMG folder.");
    ui->edit_show_class->setText("");
    ui->edit_show_inf->setText("");

    ui->btn_img_mode->setEnabled(true);

    if (m_nrt.use_count() > 0 && m_nrt->get_model_status() == nrt::STATUS_SUCCESS && m_nrt->get_model_type() == "OCR") {
        /*for (int r = 0; r < ui->tableWidget_class->rowCount(); r++) {
            int colCount = ui->tableWidget_class->columnCount();
            if ((r == 3) || (r == 6))
                colCount = 6;
            for (int c = 0; c < colCount; c++) {
                ui->tableWidget_class->item(r, c)->setBackgroundColor(QColor(255, 255, 255));
                ui->tableWidget_class->item(r, c)->setTextColor(QColor(0, 0, 0));
            }
        }*/
    }

    // Auto save mode -> Save evaluation json file
    if(ui->rad_cam_autosave->isChecked()){
        std::unique_lock<std::mutex> lock(m_save_info._mutex);

        if(!m_save_info.json_file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
            qDebug()<<"Failed to open file.";
            return;
        }

        m_save_info.json_doc.setObject(m_save_info.title);
        m_save_info.json_file.write(m_save_info.json_doc.toJson());
        m_save_info.json_file.close();

        QMessageBox::information(this,
                                 "Notification",
                                 "Evaluation done.\nThe results have been saved.",
                                 QMessageBox::Ok);
    }

    ui->rad_cam_autosave->setEnabled(true);
    ui->rad_cam_rtmode->setEnabled(true);
    ui->btn_select_single_mode->setEnabled(true);
    ui->btn_select_ensmble_mode->setEnabled(true);

    ui->com_video_list->setEnabled(true);
}

/*void MainWindow::on_btn_img_input_clicked()
{
    QMessageBox *err_msg = new QMessageBox;
    err_msg->setFixedSize(600, 400);

    QString inputPath = QFileDialog::getExistingDirectory(this, tr("Select Input Folder"), QDir::homePath());
    if (inputPath.isNull())
        return;
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

    ui->lab_img_total_num->setText("/ " + QString::number(inf_img_num));
    ui->spb_img_cur_idx->setRange(0, inf_img_num);
    ui->spb_img_cur_idx->setValue(0);

    QCollator collator;
    collator.setNumericMode(true);
    std::sort(inf_img_list.begin(), inf_img_list.end(), collator);

    if (inputPath.contains(QString("Expo"))) {
        std::random_shuffle(inf_img_list.begin(),inf_img_list.end());
        macro_flag = true;
    }
    else {
        macro_flag = false;
    }
    //for (int i = 0; i < inf_img_num; i++)
    //    qDebug() << inf_img_list[i];

//    m_loading_effect.reset();

//    this->setEnabled(true);
//    movie->~QMovie();
//    lbl->~QLabel();
    delete err_msg;
}*/

/*void MainWindow::on_btn_img_output_clicked()
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

}*/

void MainWindow::on_cbx_set_show_time_stateChanged(int state)
{
    if (state == Qt::Unchecked) {
        ui->edit_set_show_time->setEnabled(false);
        ui->lab_set_show_time->setEnabled(false);
    }
    else if (state == Qt::Checked) {
        ui->edit_set_show_time->setEnabled(true);
        ui->lab_set_show_time->setEnabled(true);
    }
}

void MainWindow::on_btn_img_play_clicked()
{
    QMessageBox *err_msg = new QMessageBox;
    err_msg->setFixedSize(600, 400);

    int img_show_time = 0;

    if (m_timer.use_count() <= 0) {
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

        if (m_nrt.use_count() <= 0  || !is_ready_for_inf(m_nrt.get())) {
            qDebug() << "NRT) There is no model";
            err_msg->critical(0,"Error", "Please choose model to inference.");
            return;
        }

        if(inf_mode_status == INF_MODE_DET_CLA && m_nrt_ensmble.use_count() > 0 && !is_ready_for_inf(m_nrt_ensmble.get())){
            qDebug() << "NRT) There is no model";
            err_msg->critical(0,"Error", "Please choose model to inference.");
            return;
        }

        ui->btn_cam_mode->setEnabled(false);

        qDebug() << "IMG Mode) Play";

        ui->cbx_set_show_time->setEnabled(false);
        ui->edit_set_show_time->setEnabled(false);
        ui->lab_set_show_time->setEnabled(false);

        ui->com_image_list->setCurrentIndex(0);
        ui->com_image_list->setEditable(false);

        ui->spb_img_cur_idx->setValue(0);
        ui->spb_img_cur_idx->setReadOnly(true);

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
            ui->cbx_set_show_time->setEnabled(false);
            ui->edit_set_show_time->setEnabled(false);
            ui->lab_set_show_time->setEnabled(false);

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

    ui->rad_img_rtmode->setEnabled(false);
    ui->rad_img_save_mode->setEnabled(false);

    img_inf_ing_flag = true;

    delete err_msg;
}

void MainWindow::on_btn_img_pause_clicked()
{
    if (m_timer.use_count() <= 0)
        return;
    img_inf_ing_flag = false;
    if (m_timer->isActive()) {
        qDebug() << "CAM Mode) Pause";
        m_timer->stop();
    }

    ui->cbx_set_show_time->setEnabled(true);
    ui->edit_set_show_time->setEnabled(true);
    ui->lab_set_show_time->setEnabled(true);

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

    inf_image_id.clear();
    inf_image_list.clear();
    ui->com_image_list->clear();
    ui->com_image_list->hide();

    ui->spb_img_cur_idx->hide();
    ui->lab_img_total_num->hide();

    ui->cbx_set_show_time->setChecked(false);
    ui->edit_set_show_time->setText("");

    ui->lab_show_res->setText("Please Select CAM or IMG folder.");
    ui->edit_show_class->setText("");
    ui->edit_show_inf->setText("");

    ui->Img_Option->setCurrentWidget(ui->SelectImagesetPage);

    ui->rad_img_rtmode->setChecked(true);
    ui->rad_img_save_mode->setChecked(false);
    ui->rad_img_rtmode->setEnabled(true);
    ui->rad_img_save_mode->setEnabled(true);

    if (m_nrt.use_count()>0 && m_nrt->get_model_status() == nrt::STATUS_SUCCESS && m_nrt->get_model_type() == "OCR") {
        /*for (int r = 0; r < ui->tableWidget_class->rowCount(); r++) {
            int colCount = ui->tableWidget_class->columnCount();
            if ((r == 3) || (r == 6))
                colCount = 6;
            for (int c = 0; c < colCount; c++) {
                ui->tableWidget_class->item(r, c)->setBackgroundColor(QColor(255, 255, 255));
                ui->tableWidget_class->item(r, c)->setTextColor(QColor(0, 0, 0));
            }
        }*/
    }

    ui->btn_img_play->setEnabled(true);
    ui->btn_img_pause->setEnabled(true);
}

void MainWindow::on_com_cam_input_select_currentTextChanged(const QString &text){
    if(text.toStdString() == SINGLE_VIDEO_TEXT){
        ui->btn_cam_select->setText("Select Video File");
    }
    else if(text.toStdString() == VIDEO_FOLDER_TEXT){
        ui->btn_cam_select->setText("Select Video Folder");
    }
    else if(text.toStdString() == CAMERA_TEXT){
        ui->btn_cam_select->setText("Select Camera");
    }

    if(ui->com_video_list->isEnabled()){
        video_list.clear();
        ui->com_video_list->clear();
        ui->com_video_list->hide();
    }

    if(m_videoInputCap.isOpened()){
        m_videoInputCap.release();
    }

    media_mode = MEDIA_MODE_NONE;

    setCamControlEnabled(false);
}

void MainWindow::on_com_video_list_currentTextChanged(const QString& text){
    if(text.isEmpty()){
        return;
    }

    if(m_videoInputCap.isOpened()){
        m_videoInputCap.release();
    }

    QString cur_video_path = video_list[ui->com_video_list->currentIndex()];

    QString extension = cur_video_path.split('.').last();
    if(extension == "avi" || extension == "mp4" || extension == "MOV"){
        m_videoInputCap.open(cur_video_path.toLocal8Bit().constData(), cv::CAP_MSMF);
    }
    else if(extension == "mpg" || extension == "mpeg"){
        m_videoInputCap.open(cur_video_path.toLocal8Bit().constData(), cv::CAP_FFMPEG);
    }

    if(!m_videoInputCap.isOpened()){
        QMessageBox::information(this,
                                 "Notification",
                                 "Sorry. The video file is corupted.",
                                 QMessageBox::Ok);
        m_videoInputCap.release();
        return;
    }

    showResult();
    setCamControlEnabled(true);
    on_btn_cam_play_clicked();
}

void MainWindow::setInfMode(int infMode){
    inf_mode_status = infMode;
}

bool MainWindow::is_ready_for_inf(NrtExe* nrt_ptr){
    if(nrt_ptr->get_model_status() == nrt::Status::STATUS_SUCCESS && nrt_ptr->get_executor_status() == nrt::Status::STATUS_SUCCESS)
        return true;
    else
        return false;
}

void MainWindow::on_btn_select_single_mode_clicked()
{
    QDialog dialog(this);
    QVBoxLayout* root_cont = new QVBoxLayout;
    QGroupBox* groupbox = new QGroupBox;
    QRadioButton* newModel = new QRadioButton;
    newModel->setText("Select new model from file system.");
    newModel->setChecked(true);
    QRadioButton* existingModel = new QRadioButton;
    existingModel->setText("Select existing model.");
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(newModel);
    vbox->addWidget(existingModel);
    vbox->addStretch(1);
    groupbox->setLayout(vbox);
    root_cont->addWidget(groupbox);

    QSqlDatabase db = QSqlDatabase::database("main_thread");
    QSqlTableModel* modelsTableModel = new QSqlTableModel(this, db);
    modelsTableModel->setTable("Models");
    modelsTableModel->select();
    modelsTableModel->setHeaderData(1, Qt::Horizontal, "Name");
    modelsTableModel->setHeaderData(2, Qt::Horizontal, "Created On");
    modelsTableModel->setHeaderData(4, Qt::Horizontal, "Type");
    modelsTableModel->setHeaderData(5, Qt::Horizontal, "Platform");
    modelsTableModel->setHeaderData(6, Qt::Horizontal, "Search Space");
    modelsTableModel->setHeaderData(7, Qt::Horizontal, "Inference Level");

    QTableView* view = new QTableView;
    view->setModel(modelsTableModel);
    view->hideColumn(0);
    view->hideColumn(3);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setEnabled(false);

    view->verticalHeader()->hide();
    view->horizontalHeader()->show();
    view->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    view->setMinimumWidth(view->horizontalHeader()->width());
    view->horizontalHeader()->setStretchLastSection(true);

    connect(existingModel, SIGNAL(toggled(bool)), view, SLOT(setEnabled(bool)));
    root_cont->addWidget(view);

    QDialogButtonBox*btnbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                   Qt::Horizontal, &dialog);
    connect(btnbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(btnbox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    connect(newModel, SIGNAL(toggled(bool)), view->selectionModel(), SLOT(clearSelection()));

    root_cont->addWidget(btnbox);
    dialog.setLayout(root_cont);

    QString modelPath = "";
    QString modelName;
    if(dialog.exec() == QDialog::Accepted){
        if(newModel->isChecked()){
            insert_new_model_flag = true;
            modelPath = QFileDialog::getOpenFileName(this, tr("Select Model"), QDir::homePath(), tr("default (*.net)"));
            modelName = modelPath.split('/').last();
            // modelId is set later after the NrtExe object is created.(in the set_model_completed() function)
        }
        else if(view->selectionModel()->hasSelection()){
            int rowID = view->selectionModel()->currentIndex().row();
            int modelId = view->model()->index(rowID, 0).data().toInt();
            modelPath = m_db->getModelPath(modelId);
            modelName = m_db->getModelName(modelId);
            currentModelId = modelId;
            qDebug() << "Model ID: " << modelId << ", Model Path: " << modelPath;
        }
    }

    if (modelPath == ""){
        QMessageBox::information(this,
                                 "Model Selection",
                                 "Failed to select model file",
                                 QMessageBox::Ok);
        return;
    }

    QMessageBox::information(this,
                             "Model Selection",
                             modelName + " has been selected.",
                             QMessageBox::Ok);

    if(m_nrt.use_count() > 0){
        m_nrt.reset();
    }
    m_nrt = std::make_shared<NrtExe>();

    if (!m_nrt->get_gpu_status()) {
        select_gpu(m_nrt.get(), "Select GPU for model: " + modelName);
        if (!m_nrt->get_gpu_status())
            return;
    }

    setInfMode(INF_MODE_SINGLE);

    connect(futureWatcher.get(), SIGNAL(started()), this, SLOT(set_model_started()));
    connect(futureWatcher.get(), SIGNAL(finished()), this, SLOT(setUiForSingle()));

    future = QtConcurrent::run(set_model_thread, m_nrt.get(), modelPath, ui->cbx_select_fp16->isChecked());
    futureWatcher->setFuture(future);
}

void MainWindow::set_model_started(){
    ready_for_inference = false;

    if ( ui->Model_Proper->currentIndex() == 0){ // Info Page -> Status Page
        ui->Model_Proper->setCurrentWidget(ui->Model_Status_Page);
    }

    ui->rad_cam_autosave->setEnabled(false);

    ui->lab_model_status->setText("Loading Model...\nIt may take a few seconds...");
    ui->btn_select_single_mode->setEnabled(false);
    ui->btn_select_ensmble_mode->setEnabled(false);
    ui->cbx_select_fp16->setEnabled(false);

    // Clear class label and inference time
    ui->edit_show_inf->clear();
    ui->edit_show_class->clear();

    ui->inf_numbers->hide();
}

void MainWindow::select_gpu(NrtExe* nrt_ptr, QString msg)
{
    int idx = 0;
    QDialog *selectDlg = new QDialog;
    selectDlg->setWindowTitle("GPU Allocation");
    selectDlg->setFixedSize(400, 300);

    QVBoxLayout *dlgVLayout = new QVBoxLayout;

    QLabel *cHeader = new QLabel(msg, selectDlg);
    dlgVLayout->addWidget(cHeader);
    int gpuNum = nrt_ptr->get_gpu_num();
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
        nrt_ptr->set_gpu(idx);
    }
    else
        return;
}

void MainWindow::on_btn_select_ensmble_mode_clicked(){
    QStringList options = {"One Class Detection -> Multi Class Classification"};

    bool isOkPressed;
    QString combination = QInputDialog::getItem(this,
                                                "Ensmble",
                                                "Available combinations: ",
                                                options,
                                                0, false, &isOkPressed);

    if(!isOkPressed){
        QMessageBox::information(this,
                                 "Notification",
                                 "Ensmble configuration has been cancled.",
                                 QMessageBox::Ok);
        return;
    }
    if(combination == "One Class Detection -> Multi Class Classification"){
        QDialog dialog;
        QVBoxLayout* root_cont = new QVBoxLayout;
        root_cont->addWidget(new QLabel("Select Detection Model"));

        QSqlDatabase db = QSqlDatabase::database("main_thread");
        QSqlTableModel* det_modelsTableModel = new QSqlTableModel(this, db);
        det_modelsTableModel->setTable("Models");
        det_modelsTableModel->setFilter("Type='Detection'");
        det_modelsTableModel->select();
        det_modelsTableModel->setHeaderData(1, Qt::Horizontal, "Name");
        det_modelsTableModel->setHeaderData(2, Qt::Horizontal, "Created On");
        det_modelsTableModel->setHeaderData(4, Qt::Horizontal, "Type");
        det_modelsTableModel->setHeaderData(5, Qt::Horizontal, "Platform");
        det_modelsTableModel->setHeaderData(6, Qt::Horizontal, "Search Space");
        det_modelsTableModel->setHeaderData(7, Qt::Horizontal, "Inference");

        QTableView* view = new QTableView;
        view->setModel(det_modelsTableModel);
        view->setEditTriggers(QAbstractItemView::NoEditTriggers);
        view->hideColumn(0);
        view->hideColumn(3);
        view->setSelectionBehavior(QAbstractItemView::SelectRows);
        view->setSelectionMode(QAbstractItemView::SingleSelection);
        view->verticalHeader()->hide();
        view->horizontalHeader()->show();
        view->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        // vertical resize table widget to contents
        int totalRowHeight = 0;
        int count = view->verticalHeader()->count();
        for(int i=0; i < count; i++){
            if(!view->verticalHeader()->isSectionHidden(i)){
                totalRowHeight +=  view->verticalHeader()->sectionSize(i);
            }
        }
        if(!view->horizontalScrollBar()->isHidden()){
            totalRowHeight += view->horizontalScrollBar()->height();
        }
        if(!view->horizontalHeader()->isHidden()){
            totalRowHeight += view->horizontalHeader()->height();
        }
        view->setMaximumHeight(totalRowHeight);
        root_cont->addWidget(view);

        root_cont->addWidget(new QLabel("Select Classification Model"));

        QSqlTableModel* cla_modelsTableModel = new QSqlTableModel(this, db);
        cla_modelsTableModel->setTable("Models");
        cla_modelsTableModel->setFilter("Type='Classification'");
        cla_modelsTableModel->select();
        cla_modelsTableModel->setHeaderData(1, Qt::Horizontal, "Name");
        cla_modelsTableModel->setHeaderData(2, Qt::Horizontal, "Created On");
        cla_modelsTableModel->setHeaderData(4, Qt::Horizontal, "Type");
        cla_modelsTableModel->setHeaderData(5, Qt::Horizontal, "Platform");
        cla_modelsTableModel->setHeaderData(6, Qt::Horizontal, "Search Space");
        cla_modelsTableModel->setHeaderData(7, Qt::Horizontal, "Inference");

        QTableView* view2 = new QTableView;
        view2->setModel(cla_modelsTableModel);
        view2->setEditTriggers(QAbstractItemView::NoEditTriggers);
        view2->hideColumn(0);
        view2->hideColumn(3);
        view2->setSelectionBehavior(QAbstractItemView::SelectRows);
        view2->setSelectionMode(QAbstractItemView::SingleSelection);
        view2->verticalHeader()->hide();
        view2->horizontalHeader()->show();
        view2->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        // vertical resize table widget to contents
        totalRowHeight = 0;
        count = view2->verticalHeader()->count();
        for(int i=0; i < count; i++){
            if(!view2->verticalHeader()->isSectionHidden(i)){
                totalRowHeight +=  view2->verticalHeader()->sectionSize(i);
            }
        }
        if(!view2->horizontalScrollBar()->isHidden()){
            totalRowHeight += view2->horizontalScrollBar()->height();
        }
        if(!view2->horizontalHeader()->isHidden()){
            totalRowHeight += view2->horizontalHeader()->height();
        }
        view2->setMaximumHeight(totalRowHeight);
        root_cont->addWidget(view2);

        QDialogButtonBox*btnbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                       Qt::Horizontal, &dialog);
        connect(btnbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
        connect(btnbox, SIGNAL(rejected()), &dialog, SLOT(reject()));
        root_cont->addWidget(btnbox);
        dialog.setLayout(root_cont);

        int min_width = view->width() > view2->width() ? view->width() : view2->width();
        dialog.setMinimumWidth(min_width);

        int det_modelId = -1, cla_modelId = -1;
        while(1){
            if(dialog.exec() == QDialog::Accepted){
                if(view->selectionModel()->selectedRows().size() > 0 || view2->selectionModel()->selectedRows().size() > 0){
                    int det_rowId = view->selectionModel()->currentIndex().row();
                    det_modelId = view->model()->index(det_rowId, 0).data().toInt();

                    int cla_rowId = view2->selectionModel()->currentIndex().row();
                    cla_modelId = view2->model()->index(cla_rowId, 0).data().toInt();
                    break;
                }
                else{
                    QMessageBox::information(this,
                                             "Notification",
                                             "Please select models from the table to complete ensmble configuration.",
                                             QMessageBox::Ok);
                }
            }
            else{
                QMessageBox::information(this,
                                         "Notification",
                                         "Ensmble configuration has been cancled.",
                                         QMessageBox::Ok);
                return;
            }
        }

        if(det_modelId == -1 || cla_modelId == -1){
            QMessageBox::information(this,
                                     "Notification",
                                     "Ensmble configuration failed due to wrong model ID values.",
                                     QMessageBox::Ok);
            return;
        }

        setInfMode(INF_MODE_DET_CLA);
        currentModelId = det_modelId;
        ensmbleModelId = cla_modelId;

        if(m_nrt.use_count() >0){
            m_nrt.reset();
        }
        m_nrt = std::make_shared<NrtExe>();
        if (!m_nrt->get_gpu_status()) {
            select_gpu(m_nrt.get(), "Select GPU for detection model.");
            if (!m_nrt->get_gpu_status())
                return;
        }

        if(m_nrt_ensmble.use_count() > 0){
            m_nrt_ensmble.reset();
        }
        m_nrt_ensmble = std::make_shared<NrtExe>();
        if (!m_nrt_ensmble->get_gpu_status()) {
            select_gpu(m_nrt_ensmble.get(), "Select GPU for classification model.");
            if (!m_nrt_ensmble->get_gpu_status())
                return;
        }
    }

    // Make detection executor thread
    connect(futureWatcher.get(), SIGNAL(started()), this, SLOT(set_model_started()));
    connect(futureWatcher.get(), SIGNAL(finished()), this, SLOT(ensmble_model_start()));
    QString det_modelPath = m_db->getModelPath(currentModelId);
    qDebug() << "Detection model path: " << det_modelPath;
    future = QtConcurrent::run(set_model_thread, m_nrt.get(), det_modelPath, ui->cbx_select_fp16->isChecked());
    futureWatcher->setFuture(future);
}

void MainWindow::ensmble_model_start(){
    disconnect(futureWatcher.get(), SIGNAL(started()), this, SLOT(set_model_started()));
    disconnect(futureWatcher.get(), SIGNAL(finished()), this, SLOT(ensmble_model_start()));

    // Make classification executor thread
    connect(futureWatcherEns.get(), SIGNAL(finished()), this, SLOT(setUiForEnsmble()));
    QString ens_modelPath = m_db->getModelPath(ensmbleModelId);
    qDebug() << "Ensmble model path: " << ens_modelPath;
    futureEns = QtConcurrent::run(set_model_thread, m_nrt_ensmble.get(), ens_modelPath, ui->cbx_select_fp16->isChecked());
    futureWatcherEns->setFuture(futureEns);
}

void MainWindow::setUiForEnsmble(){
    qDebug() << "setUiForEnsmble) Start ensmble Ui setup.";

    disconnect(futureWatcherEns.get(), SIGNAL(started()), this, SLOT(set_model_started()));
    disconnect(futureWatcherEns.get(), SIGNAL(finished()), this, SLOT(setUiForEnsmble()));

    if(m_nrt.use_count() > 0 && is_ready_for_inf(m_nrt.get()) && m_nrt_ensmble.use_count() > 0 && is_ready_for_inf(m_nrt_ensmble.get())){
        // 0: Model Info Page, 1: Model Status Page
        if( ui->Model_Proper->currentIndex() == 1 ) {
            ui->Model_Proper->setCurrentWidget(ui->Model_Info_Page);
        }

        // GPU Device 설정
        if(m_nrt->get_gpu_num() == 1){
            ui->lab_device->setText(m_nrt->get_gpu_name());
        }
        else if(m_nrt->get_gpu_num() > 1){
            ui->lab_device->setText(m_nrt->get_gpu_name() + ", " + m_nrt_ensmble->get_gpu_name());
        }

        // Model Name 설정
        ui->lab_model_name->setText(m_nrt->get_model_name() + ", " + m_nrt_ensmble->get_model_name());
        QString model_info = "Type: " + m_nrt->get_model_type() + ", " + m_nrt_ensmble->get_model_type() + "\n";
        model_info += "Platform: " + m_nrt->get_model_training_type() + ", " + m_nrt_ensmble->get_model_training_type() + "\n";
        model_info += "Search Space: " + m_nrt->get_model_search_level() + ", " + m_nrt_ensmble->get_model_search_level();
        ui->lab_model_name->setToolTip(model_info);

        // Model treshold 설정
        prob_threshold_dialog(m_nrt.get());
        size_threshold_dialog(m_nrt.get());
        prob_threshold_dialog(m_nrt_ensmble.get());

        // Model 1 UI Setting
        ui->lab_model1->setText("Object Detection Class Probs.");
        ui->table_model1_class_probs->clear();
        ui->table_model1_class_probs->horizontalHeader()->hide();
        ui->table_model1_class_probs->verticalHeader()->hide();
        ui->table_model1_class_probs->horizontalHeader()->show();
        ui->table_model1_class_probs->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);

        QString model_type = m_nrt->get_model_type();
        int NAME_COL = 0, PROB_COL = 1;
        int START_POINT;

        if(model_type == "Detection"){
            START_POINT = 1;
            ui->table_model1_class_probs->setRowCount(m_nrt->get_model_class_num() - START_POINT);
            ui->table_model1_class_probs->setColumnCount(2);
            QStringList horHeader ={"Name", "Probability"};
            ui->table_model1_class_probs->setHorizontalHeaderLabels(horHeader);

            // Name column
            for (int i = START_POINT; i < m_nrt->get_model_class_num(); i++) {
                QTableWidgetItem *newItem = new QTableWidgetItem(0);
                newItem->setText(m_nrt->get_model_class_name(i));
                newItem->setTextAlignment(Qt::AlignCenter);
                ui->table_model1_class_probs->setItem(i - START_POINT, NAME_COL, newItem);
            }

            // Prob column
            int row_cnt = ui->table_model1_class_probs->rowCount();
            for (int i = 0; i < row_cnt; i++) {
                QTableWidgetItem *newItem = new QTableWidgetItem(0);
                newItem->setText(QString::number(0) + QString("%"));
                newItem->setTextAlignment(Qt::AlignCenter);
                ui->table_model1_class_probs->setItem(i, PROB_COL, newItem);
            }
        }
        else return;


        // Model 2 UI Setting
        int COLOR_COL = 0;
        NAME_COL = 1, PROB_COL = 2;
        ui->lab_model2->setText("Classification Class Probs.");

        ui->table_model2_class_probs->clear();
        ui->table_model2_class_probs->horizontalHeader()->hide();
        ui->table_model2_class_probs->verticalHeader()->hide();
        ui->table_model2_class_probs->horizontalHeader()->show();
        ui->table_model2_class_probs->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        //ui->table_model2_class_probs->verticalHeader()->setStretchLastSection(true);

        model_type = m_nrt_ensmble->get_model_type();
        if(model_type == "Classification"){
            START_POINT = 0;
            ui->table_model2_class_probs->setRowCount(m_nrt_ensmble->get_model_class_num() - START_POINT);
            ui->table_model2_class_probs->setColumnCount(3);
            QStringList horHeader = {"Color", "Name", "Probability"};
            ui->table_model2_class_probs->setHorizontalHeaderLabels(horHeader);

            // Color Column
            COLOR_VECTOR.clear();
            int row_cnt = ui->table_model2_class_probs->rowCount();

            for (int i = 0; i < row_cnt; i++) {
                QWidget* cWidget = new QWidget();
                QLabel* lab_color = new QLabel();
                QColor new_color = QColor(0, 0, 0);

                if (i == 0)
                    new_color = QColor(255, 0, 0);
                else if(i == 1)
                    new_color = QColor(0, 0, 255);
                else if(i ==2)
                    new_color = QColor(0, 255, 0);

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
                ui->table_model2_class_probs->setCellWidget(i, COLOR_COL, cWidget);
                COLOR_VECTOR.append(new_color);
            }

            // Name column
            for (int i = START_POINT; i < m_nrt_ensmble->get_model_class_num(); i++) {
                QTableWidgetItem *newItem = new QTableWidgetItem(0);
                newItem->setText(m_nrt_ensmble->get_model_class_name(i));
                newItem->setTextAlignment(Qt::AlignCenter);
                ui->table_model2_class_probs->setItem(i - START_POINT, NAME_COL, newItem);
            }

            // Prob column
            row_cnt = ui->table_model2_class_probs->rowCount();
            for (int i = 0; i < row_cnt; i++) {
                QTableWidgetItem *newItem = new QTableWidgetItem(0);
                newItem->setText(QString::number(0) + QString("%"));
                newItem->setTextAlignment(Qt::AlignCenter);
                ui->table_model2_class_probs->setItem(i, PROB_COL, newItem);
            }
        }
//        else return;

        // Chart View Setting
        if(inf_mode_status == INF_MODE_DET_CLA){
            series = new QPieSeries();
            for(int i = 0; i < m_nrt_ensmble->get_model_class_num(); i++){
                QString classname = m_nrt_ensmble->get_model_class_name(i);
                series->append(classname, 1);
            }
            series->setLabelsVisible(true);
            series->setLabelsPosition(QPieSlice::LabelInsideHorizontal);

            class_ratio.clear();
            class_ratio = QVector<int>(m_nrt_ensmble->get_model_class_num(), 0);

            for(int i=0; i < m_nrt_ensmble->get_model_class_num(); i++){
                if(i < series->slices().length() && i < COLOR_VECTOR.length()){
                    series->slices().at(i)->setColor(COLOR_VECTOR[i]);
                    series->slices().at(i)->setLabelColor(QColor(255, 255, 255));
                    series->slices().at(i)->setLabelFont(QFont("Ubuntu", 10));
                }
            }

            /*QPieSlice* slice = series->slices().at(1);
            slice->setExploded();
            slice->setLabelVisible();
            slice->setPen(QPen(Qt::darkGreen, 2));
            slice->setBrush(Qt::green);*/

            chart = new QChart();
            chart->addSeries(series);
            chart->setTitle("Class Ratio");
            chart->legend()->hide();

            ui->inf_chart_view->setChart(chart);
            ui->inf_chart_view->setRenderHint(QPainter::Antialiasing);
        }

        // vertical resize table widget to contents
        int totalRowHeight = 0;
        // visible row height
        int count = ui->table_model1_class_probs->verticalHeader()->count();
        for(int i=0; i < count; i++){
            if(!ui->table_model1_class_probs->verticalHeader()->isSectionHidden(i)){
                totalRowHeight +=  ui->table_model1_class_probs->verticalHeader()->sectionSize(i);
            }
        }
        //scrollbar visibility
        if(!ui->table_model1_class_probs->horizontalScrollBar()->isHidden()){
            totalRowHeight += ui->table_model1_class_probs->horizontalScrollBar()->height();
        }
        // horizontal header visibility
        if(!ui->table_model1_class_probs->horizontalHeader()->isHidden()){
            totalRowHeight += ui->table_model1_class_probs->horizontalHeader()->height();
        }
        ui->table_model1_class_probs->setMaximumHeight(totalRowHeight);

        totalRowHeight = 0;
        count = ui->table_model2_class_probs->verticalHeader()->count();
        for(int i=0; i < count; i++){
            if(!ui->table_model2_class_probs->verticalHeader()->isSectionHidden(i)){
                totalRowHeight +=  ui->table_model2_class_probs->verticalHeader()->sectionSize(i);
            }
        }
        if(!ui->table_model2_class_probs->horizontalScrollBar()->isHidden()){
            totalRowHeight += ui->table_model2_class_probs->horizontalScrollBar()->height();
        }
        if(!ui->table_model2_class_probs->horizontalHeader()->isHidden()){
            totalRowHeight += ui->table_model2_class_probs->horizontalHeader()->height();
        }
        ui->table_model2_class_probs->setMaximumHeight(totalRowHeight);
    }

    ui->btn_select_single_mode->setEnabled(true);
    ui->btn_select_ensmble_mode->setEnabled(true);
    ui->cbx_select_fp16->setEnabled(true);
    ui->rad_cam_autosave->setEnabled(true);
    ui->rad_img_save_mode->setEnabled(true);
    ui->btn_model_settings->show();

    ui->inf_numbers->setVisible(true);

    ready_for_inference = true;

    return;
}

void MainWindow::setUiForSingle(){
    QString modelPath = futureWatcher->result();

    disconnect(futureWatcher.get(), SIGNAL(started()), this, SLOT(set_model_started()));
    disconnect(futureWatcher.get(), SIGNAL(finished()), this, SLOT(setUiForSingle()));

    if(m_nrt.use_count() > 0 && is_ready_for_inf(m_nrt.get())){
        // db에 저장되지 않은 모델이면 insert 하기
        if(insert_new_model_flag){
            insert_new_model_flag = false;
            int modelId = m_db->InsertModel(modelPath, m_nrt->get_model_name(), m_nrt->get_model_type(), m_nrt->get_model_training_type(), m_nrt->get_model_search_level(), m_nrt->get_model_inference_level());
            if(modelId == -1){
                qDebug() << "Model did not get saved in db correctly.";
                currentModelId = -1;
                return;
            }
            currentModelId = modelId;
        }

        // 0: Model Info Page, 1: Model Status Page
        if( ui->Model_Proper->currentIndex() == 1 ) {
            ui->Model_Proper->setCurrentWidget(ui->Model_Info_Page);
        }

        // GPU Device 설정
        ui->lab_device->setText(m_nrt->get_gpu_name());

        // Model Name 설정
        ui->lab_model_name->setText(m_nrt->get_model_name());
        QString model_info = "Type: " + m_nrt->get_model_type() + "\n";
        model_info += "Platform: " + m_nrt->get_model_training_type() + "\n";
        model_info += "Search Space: " + m_nrt->get_model_search_level();
        ui->lab_model_name->setToolTip(model_info);

        // Model treshold 설정
        prob_threshold_dialog(m_nrt.get());
        size_threshold_dialog(m_nrt.get());

        // Model 1 UI Setting
        ui->lab_model1->setText(m_nrt->get_model_type() + " Predcited Class: ");
        QString model_type = m_nrt->get_model_type();
        int NAME_COL = 0, PROB_COL = 1;
        int START_POINT;
        if(model_type == "Detection" || model_type == "Segmentation"){
            START_POINT = 1;
            ui->table_model1_class_probs->setRowCount(m_nrt->get_model_class_num() - START_POINT);
            ui->table_model1_class_probs->setColumnCount(2);
            QStringList horHeader ={"Name", "Probability"};
            ui->table_model1_class_probs->setHorizontalHeaderLabels(horHeader);

            // Name column
            for (int i = START_POINT; i < m_nrt->get_model_class_num(); i++) {
                QTableWidgetItem *newItem = new QTableWidgetItem(0);
                newItem->setText(m_nrt->get_model_class_name(i));
                newItem->setTextAlignment(Qt::AlignCenter);
                ui->table_model1_class_probs->setItem(i - START_POINT, NAME_COL, newItem);
            }

            // Prob column
            int row_cnt = ui->table_model1_class_probs->rowCount();
            for (int i = 0; i < row_cnt; i++) {
                QTableWidgetItem *newItem = new QTableWidgetItem(0);
                newItem->setText(QString::number(0) + QString("%"));
                newItem->setTextAlignment(Qt::AlignCenter);
                ui->table_model1_class_probs->setItem(i, PROB_COL, newItem);
            }

            // Color Column
            COLOR_VECTOR.clear();
            for (int i = 0; i < row_cnt; i++) {
                QColor new_color = QColor(0, 0, 0);

                if (i == 0)
                    new_color = QColor(255, 0, 0);
                else if(i == 1)
                    new_color = QColor(0, 0, 255);
                else if(i ==2)
                    new_color = QColor(0, 255, 0);

//                QVariant cVariant = new_color;
//                QString cString = cVariant.toString();
//                lab_color->setStyleSheet("background-color:"+cString+";");
//                lab_color->setAlignment(Qt::AlignTop | Qt::AlignLeft);
//                lab_color->setContentsMargins(0,0,0,0);
//                lab_color->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
//                QHBoxLayout* cLayout = new QHBoxLayout(cWidget);
//                cLayout->addWidget(lab_color);
//                cLayout->setAlignment(Qt::AlignCenter);
//                cLayout->setContentsMargins(30, 2, 30, 2);
//                cWidget->setLayout(cLayout);
//                ui->table_model2_class_probs->setCellWidget(i, COLOR_COL, cWidget);
                COLOR_VECTOR.append(new_color);
            }
        }
//        else return;

        // Chart View Setting
        /*if(inf_mode_status == INF_MODE_DET_CLA){
            series = new QPieSeries();
            for(int i = 0; i < m_nrt_ensmble->get_model_class_num(); i++){
                QString classname = m_nrt_ensmble->get_model_class_name(i);
                series->append(classname, 1);
            }
            series->setLabelsVisible(true);
            series->setLabelsPosition(QPieSlice::LabelInsideHorizontal);

            class_ratio.clear();
            class_ratio = QVector<int>(m_nrt_ensmble->get_model_class_num(), 0);

            for(int i=0; i < m_nrt_ensmble->get_model_class_num(); i++){
                if(i < series->slices().length() && i < COLOR_VECTOR.length()){
                    series->slices().at(i)->setColor(COLOR_VECTOR[i]);
                    series->slices().at(i)->setColor(QColor(255, 255, 255));
                }
            }

            QPieSlice* slice = series->slices().at(1);
            slice->setExploded();
            slice->setLabelVisible();
            slice->setPen(QPen(Qt::darkGreen, 2));
            slice->setBrush(Qt::green);

            chart = new QChart();
            chart->addSeries(series);
            chart->setTitle("Class Ratio");
            chart->legend()->hide();

            ui->inf_chart_view->setChart(chart);
            ui->inf_chart_view->setRenderHint(QPainter::Antialiasing);
        }

        // vertical resize table widget to contents
        int totalRowHeight = 0;
        // visible row height
        int count = ui->table_model1_class_probs->verticalHeader()->count();
        for(int i=0; i < count; i++){
            if(!ui->table_model1_class_probs->verticalHeader()->isSectionHidden(i)){
                totalRowHeight +=  ui->table_model1_class_probs->verticalHeader()->sectionSize(i);
            }
        }
        //scrollbar visibility
        if(!ui->table_model1_class_probs->horizontalScrollBar()->isHidden()){
            totalRowHeight += ui->table_model1_class_probs->horizontalScrollBar()->height();
        }
        // horizontal header visibility
        if(!ui->table_model1_class_probs->horizontalHeader()->isHidden()){
            totalRowHeight += ui->table_model1_class_probs->horizontalHeader()->height();
        }
        ui->table_model1_class_probs->setMaximumHeight(totalRowHeight);*/

        // Hide Model2 realted stuff
    }
    else{
        qDebug() << "NRT) Set Model Failed!";

        QMessageBox messageBox;
        messageBox.critical(0,"Error","Model initialization has falied!");
        messageBox.setFixedSize(500,200);

        ui->lab_model_status->setText("Please Select Model.");

        currentModelId = -1;
    }

    ui->btn_select_single_mode->setEnabled(true);
    ui->btn_select_ensmble_mode->setEnabled(true);
    ui->cbx_select_fp16->setEnabled(true);
    ui->rad_cam_autosave->setEnabled(true);
    ui->rad_img_save_mode->setEnabled(true);
    ui->btn_model_settings->show();

    ready_for_inference = true;

    return;
}

void MainWindow::prob_threshold_dialog(NrtExe* nrt_ptr){
    QString model_type = nrt_ptr->get_model_type();
    if(model_type == "Anomaly" || model_type == "OCR") {
        return;
    }

    QDialog dialog(this);
    QVBoxLayout* root_cont = new QVBoxLayout;

    QFormLayout* modelform = new QFormLayout;
    modelform->addRow(new QLabel("Set probability threshold for model: " + nrt_ptr->get_model_name()));

    QVector<QLineEdit*> line_edit_vector;
    for(int i=0; i < nrt_ptr->get_model_class_num(); i++){
        QLineEdit* line_edit = new QLineEdit(nullptr);

        line_edit->setText("0");
        QDoubleValidator* double_valid = new QDoubleValidator(0, 100, 2);
        double_valid->setNotation(QDoubleValidator::StandardNotation);
        line_edit->setValidator(double_valid);

        modelform->addRow(new QLabel(nrt_ptr->get_model_class_name(i) + ": "), line_edit);
        line_edit_vector.append(line_edit);
    }
    root_cont->addLayout(modelform);

    QDialogButtonBox*btnbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                   Qt::Horizontal, &dialog);
    connect(btnbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(btnbox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    root_cont->addWidget(btnbox);
    dialog.setLayout(root_cont);

    if(dialog.exec() == QDialog::Accepted){
        nrt_ptr->prob_threshold.clear();
        qDebug() << nrt_ptr->get_model_name() << "prob threshold.";
        for(int i=0; i < nrt_ptr->get_model_class_num(); i++){
            float thres_value = line_edit_vector[i]->text().toFloat();
            nrt_ptr->prob_threshold.append(thres_value);
            qDebug() << nrt_ptr->get_model_class_name(i) << ": " << QString::number(thres_value);
        }

    }
    else{
        QMessageBox::information(
                    this,
                    "Notification",
                    "Probability threshold selection has been canceled.",
                    QMessageBox::Ok);
    }
    return;
}

void MainWindow::size_threshold_dialog(NrtExe* nrt_ptr){
    QString model_type = nrt_ptr->get_model_type();
    if(model_type == "Classification" || model_type == "OCR" || model_type == "Anomaly") {
        return;
    }

    QDialog dialog(this);
    QVBoxLayout* root_cont = new QVBoxLayout;

    QVector<QLineEdit*> height_vector;
    QVector<QLineEdit*> width_vector;
    QVector<QComboBox*> conjunction_vector;

    for(int i=0; i < nrt_ptr->get_model_class_num(); i++){
        QLabel *lab = new QLabel(nrt_ptr->get_model_class_name(i) + " size threshold");
        root_cont->addWidget(lab);

        QHBoxLayout* hbox = new QHBoxLayout();

        hbox->addWidget(new QLabel("Height: "));
        QLineEdit* line_edit_height = new QLineEdit();
        line_edit_height->setText("0");
        QIntValidator *intValidator = new QIntValidator(0,9999);
        line_edit_height->setValidator(intValidator);
        hbox->addWidget(line_edit_height);

        hbox->addWidget(new QLabel("Conjuction: "));
        QComboBox* comb = new QComboBox();
        comb->addItem("AND");
        comb->addItem("OR");
        hbox->addWidget(comb);

        hbox->addWidget(new QLabel("Width: "));
        QLineEdit* line_edit_width = new QLineEdit();
        line_edit_width->setText("0");
        line_edit_width->setValidator(intValidator);
        hbox->addWidget(line_edit_width);

        root_cont->addLayout(hbox);

        height_vector.append(line_edit_height);
        width_vector.append(line_edit_width);
        conjunction_vector.append(comb);
    }


    QDialogButtonBox*btnbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                   Qt::Horizontal, &dialog);
    connect(btnbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(btnbox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    root_cont->addWidget(btnbox);
    dialog.setLayout(root_cont);

    if(dialog.exec() == QDialog::Accepted){
        nrt_ptr->size_thres_conjunction.clear();
        nrt_ptr->size_threshold.clear();

        qDebug() << nrt_ptr->get_model_name() << "size threshold.";
        for(int i=0; i < nrt_ptr->get_model_class_num(); i++){
            int height_thres = height_vector[i]->text().toInt();
            int width_thres = width_vector[i]->text().toInt();
            QString conj = conjunction_vector[i]->currentText();

            nrt_ptr->size_threshold.append(qMakePair(height_thres, width_thres));
            nrt_ptr->size_thres_conjunction.append(conj);
        }

    }
    else{
        QMessageBox::information(
                    this,
                    "Notification",
                    "Size threshold selection has been canceled.",
                    QMessageBox::Ok);
    }
    return;
}

bool MainWindow::on_btn_select_image_set_clicked(){
    QDialog dialog(this);
    QVBoxLayout* root_cont = new QVBoxLayout;
    QGroupBox* groupbox = new QGroupBox;
    QRadioButton* newImageFolder = new QRadioButton;
    newImageFolder->setText("Select new images from folder.");
    newImageFolder->setChecked(true);
    QRadioButton* existingImageSet = new QRadioButton;
    existingImageSet->setText("Select existing image set.");
    QVBoxLayout *vbox = new QVBoxLayout;
    vbox->addWidget(newImageFolder);
    vbox->addWidget(existingImageSet);
    vbox->addStretch(1);
    groupbox->setLayout(vbox);
    root_cont->addWidget(groupbox);

    QSqlDatabase db = QSqlDatabase::database("main_thread");
    QSqlTableModel* imageSetsTableModel = new QSqlTableModel(this, db);
    imageSetsTableModel->setTable("ImageSets");
    imageSetsTableModel->select();
    imageSetsTableModel->setHeaderData(1, Qt::Horizontal, "Name");
    imageSetsTableModel->setHeaderData(2, Qt::Horizontal, "Created On");
    imageSetsTableModel->setHeaderData(3, Qt::Horizontal, "Updated On");

    QTableView* view = new QTableView;
    view->setModel(imageSetsTableModel);
    view->hideColumn(0);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSelectionMode(QAbstractItemView::SingleSelection);
    view->setEnabled(false);

    view->verticalHeader()->hide();
    view->horizontalHeader()->show();
    view->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
    view->setMinimumWidth(view->horizontalHeader()->width());
    view->horizontalHeader()->setStretchLastSection(true);

    connect(existingImageSet, SIGNAL(toggled(bool)), view, SLOT(setEnabled(bool)));
    connect(newImageFolder, SIGNAL(toggled(bool)), view->selectionModel(), SLOT(clearSelection()));
    root_cont->addWidget(view);

    QDialogButtonBox*btnbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                   Qt::Horizontal, &dialog);
    connect(btnbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(btnbox, SIGNAL(rejected()), &dialog, SLOT(reject()));

    root_cont->addWidget(btnbox);
    dialog.setLayout(root_cont);

    int imageSetId = -1;
    QString imageSetName = "";

    if(dialog.exec() == QDialog::Accepted){
        inf_image_id.clear();
        inf_image_list.clear();
        ui->com_image_list->clear();

        if(newImageFolder->isChecked()){
            QMessageBox *err_msg = new QMessageBox;
            err_msg->setFixedSize(600, 400);

            QString inputPath = QFileDialog::getExistingDirectory(
                        this,
                        tr("Select Input Folder"),
                        QDir::homePath());

            if (inputPath.isNull())
                return false;

            QStringList img_filters;
            img_filters << "*.bmp" << "*.dib"
                        << "*.jpg" << "*.jpeg" << "*.jpe" << "*.jp2"
                        << "*.png" << "*.webp"
                        << "*.pbm" << "*.pgm" << "*.ppm" << "*.pxm" << "*.pnm"
                        << "*.tiff" << "*.tif"
                        << "*.sr" << "*.ras"
                        << "*.hdr" << "*.pic";

            QDirIterator input_it(
                        inputPath,
                        img_filters,
                        QDir::AllEntries | QDir::NoDotAndDotDot,
                        QDirIterator::Subdirectories);

            if(!input_it.hasNext()){
                err_msg->critical(0,"Error", "There are no image files to inference.");
                return false;
            }

            bool ok;
            imageSetName = QInputDialog::getText(this,"Image Selection",
                                                 "Image Set Name", QLineEdit::Normal,
                                                 "IMG_" + QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss") + "_ImageSet", &ok);
            if(imageSetName == ""){
                imageSetName = "IMG_" + QDateTime::currentDateTime().toString("dd-MM-yyyy hh:mm:ss") + "_ImageSet";
            }

            imageSetId = m_db->InsertImageSet(imageSetName);

            QVector<int> temp_id_vector;
            QVector<QString> temp_image_path_vector;
            QVector<QString> temp_image_name_vector;

            while(input_it.hasNext()){
                QString image_path = input_it.next();
                QString image_name = image_path.split('/').last();

                // insert into image set database
                QUuid u_id = QUuid::createUuid();
                QString image_uid = u_id.toString().remove(QChar('{')).remove(QChar('}')) + QString(IMG_FORMAT.c_str());
                QString new_image_path = imagesDir.absoluteFilePath(image_uid);
                if(!QFile::exists(image_path)){
                    continue;
                }
                QFile::copy(image_path, new_image_path);

                cv::Mat img = cv::imread(new_image_path.toLocal8Bit().constData());
                int imageId = m_db->InsertImage(new_image_path, image_name, img.rows, img.cols, imageSetId, "main_thread");

                temp_id_vector.append(imageId);
                temp_image_path_vector.append(new_image_path);
                temp_image_name_vector.append(image_name);
            }

            if(imageSetName.contains("expo", Qt::CaseInsensitive)){
                macro_flag = true;
                macro_cam_flag = false;

                unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                std::shuffle(temp_id_vector.begin(), temp_id_vector.end(), std::default_random_engine(seed));
                std::shuffle(temp_image_path_vector.begin(), temp_image_path_vector.end(), std::default_random_engine(seed));
                std::shuffle(temp_image_name_vector.begin(), temp_image_name_vector.end(), std::default_random_engine(seed));
            }
            else{
                macro_flag = false;
            }

            for(int i = 0; i < temp_id_vector.length(); i++){
                if(i < 0 || i >= temp_image_path_vector.length()){
                    qDebug() << "on_btn_select_image_set) vector out of range for temp_image_path.";
                    return false;
                }
                if(i < 0 || i >= temp_image_name_vector.length()){
                    qDebug() << "on_btn_select_image_set) vector out of range for temp_image_name.";
                    return false;
                }

                inf_image_id.append(temp_id_vector[i]);
                ui->com_image_list->addItem(temp_image_name_vector[i]);
                inf_image_list.append(temp_image_path_vector[i]);
            }
        }

        else if(view->selectionModel()->hasSelection()){
            int row = view->selectionModel()->currentIndex().row();
            imageSetId = view->model()->index(row, 0).data().toInt();
            imageSetName = m_db->getImageSetName(imageSetId);

            QVector<int> temp_id_vector = m_db->getImageIdVector(imageSetId, "main_thread");

            if(imageSetName.contains("expo", Qt::CaseInsensitive)){
                macro_flag = true;
                macro_cam_flag = false;

                //shuffle
                unsigned seed = std::chrono::system_clock::now().time_since_epoch().count();
                std::shuffle(temp_id_vector.begin(), temp_id_vector.end(), std::default_random_engine(seed));
            }
            else{
                macro_flag = false;
            }

            for(auto imageId : temp_id_vector){
                inf_image_id.append(imageId);
                inf_image_list.append(m_db->getImagePath(imageId, "main_thread"));
                ui->com_image_list->addItem(m_db->getImageName(imageId));
            }
        }
    }
    else{
        QMessageBox::information(this,
                             "Image Selection",
                             "Image selection has been canceled.",
                             QMessageBox::Ok);
        return false;
    }

    if(imageSetId == -1){
        return false;
    }

    {
        std::unique_lock<std::mutex> lock(m_save_info._mutex);
        m_save_info.imageSetId = imageSetId;
    }

    if(newImageFolder->isChecked()){
        QMessageBox::information(this,
                             "Image Selection",
                             "Selected " + QString::number(inf_image_list.length()) + "images.",
                             QMessageBox::Ok);
    }
    else {
        QMessageBox::information(this,
                             "Image Selection",
                             "Image set '" + imageSetName + "' has been selected.",
                             QMessageBox::Ok);
    }

    ui->Img_Option->setCurrentWidget(ui->ImageInfoPage);

    ui->btn_img_play->setEnabled(true);
    ui->btn_img_pause->setEnabled(true);
    ui->btn_img_stop->setEnabled(true);

    ui->com_image_list->setEditable(true);
    ui->com_image_list->show();

    ui->spb_img_cur_idx->setReadOnly(false);
    ui->spb_img_cur_idx->setRange(1, inf_image_list.length());
    ui->spb_img_cur_idx->show();

    ui->lab_img_total_num->setText("/ " + QString::number(inf_image_list.length()));
    ui->lab_img_total_num->show();

    ui->cbx_set_show_time->setEnabled(true);
    ui->cbx_set_show_time->setChecked(false);

    on_com_image_list_currentIndexChanged(0);

    return true;
}

void MainWindow::on_com_image_list_currentIndexChanged(int index){
    if(index < 0 || index >= inf_image_list.length()){
        qDebug() << "on_com_image_list_currentIndexChanged) wrong index";
        return;
    }
    ui->spb_img_cur_idx->setValue(index + 1);

    cv::Mat img;
    if(!img_inf_ing_flag && ui->rad_img_save_mode->isChecked()){
        QString result_image_path = "";
        {
            std::unique_lock<std::mutex> lock(m_save_info._mutex);
            result_image_path = m_db->getResultImagePath(inf_image_id[index], m_save_info.evaluationSetId, "main_thread");
        }

        if(!result_image_path.isEmpty()){
            img = cv::imread(result_image_path.toLocal8Bit().constData());
        }

        else{
            QString image_path = inf_image_list[index];
            img = cv::imread(image_path.toLocal8Bit().constData());
        }
    }

    if(!img_inf_ing_flag && ui->rad_img_rtmode->isChecked()){
        QString image_path = inf_image_list[index];
        img = cv::imread(image_path.toLocal8Bit().constData());
    }

    if(img.empty()){
        return;
    }

    cv::cvtColor(img, img, cv::COLOR_RGB2BGR);
    QImage m_qimage;
    m_qimage = QImage((const unsigned char*) (img.data), img.cols, img.rows, img.step, QImage::Format_RGB888);
    QPixmap m_qpixmap = QPixmap::fromImage(m_qimage);
    ui->lab_show_res->setPixmap(m_qpixmap.scaled(ui->lab_show_res->width(), ui->lab_show_res->height(), Qt::KeepAspectRatio));
}

void MainWindow::on_spb_img_cur_idx_valueChanged(int i){
    ui->com_image_list->setCurrentIndex(i-1);
}

void MainWindow::on_btn_model_settings_clicked(){
    // threshold 값 수정, alert 기능
    on_btn_cam_pause_clicked();

    if(inf_mode_status == INF_MODE_SINGLE && m_nrt.use_count() > 0 && is_ready_for_inf(m_nrt.get())){
        QString model_type = m_nrt->get_model_type();
        if(model_type == "OCR" || model_type == "Anomaly"){
            return;
        }

        else {
            QDialog dialog(this);
            QVBoxLayout* root_cont = new QVBoxLayout;

            QFormLayout* model_form = new QFormLayout;
            model_form->addRow(new QLabel("Model: "), new QLabel(m_nrt->get_model_name()));
            model_form->addRow(new QLabel("Type: "), new QLabel(m_nrt->get_model_type()));
            model_form->addRow(new QLabel("Platform: "), new QLabel(m_nrt->get_model_training_type()));
            model_form->addRow(new QLabel("Search Space Level: "), new QLabel(m_nrt->get_model_search_level()));
            model_form->addRow(new QLabel("Inference Level: "), new QLabel(m_nrt->get_model_inference_level()));
            root_cont->addLayout(model_form);
            root_cont->addSpacing(10);

            root_cont->addWidget(new QLabel("Prob threshold"));
            QTableWidget* prob_thres = new QTableWidget;
            prob_thres->setRowCount(m_nrt->get_model_class_num());
            prob_thres->setColumnCount(2);
            QStringList header_labels = {"Class", "Prob Threshold"};
            prob_thres->setHorizontalHeaderLabels(header_labels);

            for(int i = 0; i < m_nrt->get_model_class_num(); i++){
                QTableWidgetItem* class_name = new QTableWidgetItem;
                class_name->setText(m_nrt->get_model_class_name(i));
                class_name->setTextAlignment(Qt::AlignCenter);
                prob_thres->setItem(i, 0, class_name);

                QTableWidgetItem* prob = new QTableWidgetItem;
                int cur_prob_thres = 0;
                if(i < m_nrt->prob_threshold.length()){
                    cur_prob_thres = m_nrt->prob_threshold[i];
                }
                prob->setText(QString::number(cur_prob_thres) + "%");
                prob->setTextAlignment(Qt::AlignCenter);
                prob_thres->setItem(i, 1, prob);
            }

            // table view ui 조정
            prob_thres->verticalHeader()->hide();
            prob_thres->horizontalHeader()->show();
            prob_thres->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
//            prob_thres->setMinimumWidth(prob_thres->horizontalHeader()->width());
//            prob_thres->horizontalHeader()->setStretchLastSection(true);
            verticalResizeTable(prob_thres);
            root_cont->addWidget(prob_thres);
            root_cont->addSpacing(10);

            connect(prob_thres,
                    QOverload<const int, const int>::of(&QTableWidget::cellChanged),
                    [prob_thres](const int row, const int col)
                    {
                        if(col == 1){
                            QString input = prob_thres->item(row, col)->text();
                            input = input.remove(QRegExp("[^\\d]"));

                            float value = 0;
                            bool ok;
                            value = input.toFloat(&ok);
                            if(!ok){
                                qDebug() << "Failed to convert to int";
                                value = 0;
                            }
                            prob_thres->item(row, col)->setText(QString::number(value) + QString("%"));
                        }
                    });

            QTableWidget* size_thres = new QTableWidget;
            if(model_type == "Segmentation" || model_type == "Detection"){
                root_cont->addWidget(new QLabel("Size threshold"));
                size_thres->setRowCount(m_nrt->get_model_class_num());
                size_thres->setColumnCount(4);
                QStringList header_labels = {"Class", "Height", "AND/OR", "Width"};
                size_thres->setHorizontalHeaderLabels(header_labels);

                for(int i = 0; i < m_nrt->get_model_class_num(); i++){
                    QTableWidgetItem* class_name = new QTableWidgetItem;
                    class_name->setText(m_nrt->get_model_class_name(i));
                    class_name->setTextAlignment(Qt::AlignCenter);
                    size_thres->setItem(i, 0, class_name);

                    QTableWidgetItem* width = new QTableWidgetItem;
                    int cur_width = 0;
                    if(i < m_nrt->size_threshold.length()){
                        cur_width = m_nrt->size_threshold[i].second;
                    }
                    width->setText(QString::number(cur_width));
                    width->setTextAlignment(Qt::AlignCenter);
                    size_thres->setItem(i, 1, width);

                    QTableWidgetItem* conj = new QTableWidgetItem;
                    QString cur_conj = "AND";
                    if(i < m_nrt->size_thres_conjunction.length()){
                        cur_conj = m_nrt->size_thres_conjunction[i];
                    }
                    conj->setText(cur_conj);
                    conj->setTextAlignment(Qt::AlignCenter);
                    size_thres->setItem(i, 2, conj);

                    QTableWidgetItem* height = new QTableWidgetItem;
                    int cur_height = 0;
                    if(i < m_nrt->size_threshold.length()){
                        cur_height = m_nrt->size_threshold[i].first;
                    }
                    height->setText(QString::number(cur_height));
                    height->setTextAlignment(Qt::AlignCenter);
                    size_thres->setItem(i, 3, height);

                    // table view ui 조정
                    size_thres->verticalHeader()->hide();
                    size_thres->horizontalHeader()->show();
//                    size_thres->horizontalHeaderItem(0)
//                    size_thres->setColumnWidth(1, 100);
                    size_thres->horizontalHeader()->resizeSections(QHeaderView::ResizeToContents);
                    size_thres->setMaximumWidth(size_thres->horizontalHeader()->width());
//                    size_thres->horizontalHeader()->setStretchLastSection(true);
                    verticalResizeTable(size_thres);
                }

                root_cont->addWidget(size_thres);

                connect(size_thres,
                        QOverload<const int, const int>::of(&QTableWidget::cellChanged),
                        [size_thres](const int row, const int col)
                        {
                            // Height, Width
                            if(col == 1 || col == 3){
                                QString input = size_thres->item(row, col)->text();
                                input = input.remove(QRegExp("[^\\d]"));

                                int value = 0;
                                bool ok;
                                value = input.toInt(&ok);
                                if(!ok){
                                    qDebug() << "Failed to convert to int";
                                    value = 0;
                                }
                                size_thres->item(row, col)->setText(QString::number(value));
                            }
                        });

                connect(size_thres,
                        QOverload<const int, const int>::of(&QTableWidget::cellClicked),
                        [size_thres](const int row, const int col)
                        {
                            // AND, OR
                            if(col == 2){
                                QString cur_txt = size_thres->item(row, col)->text();
                                size_thres->item(row, col)->setText((cur_txt == "AND" ? "OR" : "AND"));
                            }
                        });
            }

            QDialogButtonBox*btnbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                           Qt::Horizontal, &dialog);
            connect(btnbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
            connect(btnbox, SIGNAL(rejected()), &dialog, SLOT(reject()));
            root_cont->addWidget(btnbox);
            dialog.setLayout(root_cont);

            if(dialog.exec() == QDialog::Accepted){
                m_nrt->prob_threshold.clear();
                for(int i = 0; i < prob_thres->rowCount(); i++){
                    QString prob = prob_thres->item(i, 1)->text().remove(QChar('%'));
                    bool ok;
                    float prob_val = prob.toFloat(&ok);
                    if(!ok){
                        prob_val = 0.0;
                    }
                    m_nrt->prob_threshold.append(prob_val);
                }

                m_nrt->size_threshold.clear();
                for(int i = 0; i < size_thres->rowCount(); i++){
                    QString h_thres = size_thres->item(i, 1)->text();
                    bool ok;
                    int h_thres_val = h_thres.toInt(&ok);
                    if(!ok){
                        h_thres_val = 0;
                    }

                    QString w_thres = size_thres->item(i, 3)->text();
                    int w_thres_val = w_thres.toInt(&ok);
                    if(!ok){
                        w_thres_val = 0;
                    }

                    m_nrt->size_threshold.append(qMakePair(h_thres_val, w_thres_val));

                    QString conj = size_thres->item(i, 2)->text();
                    m_nrt->size_thres_conjunction.append(conj);
                }
            }
        }
    }
    else if(inf_mode_status == INF_MODE_DET_CLA){

    }

    on_btn_cam_play_clicked();
}

void MainWindow::on_rad_img_save_mode_toggled(bool checked){
    if(checked){
        // 새로 선택한 것
        if(ui->rad_img_rtmode->isChecked()){
            QMessageBox* err_msg = new QMessageBox;

            // 아직 모델이 선택되지 않은 상태
            if(!ready_for_inference){
                ui->rad_img_save_mode->setChecked(false);
                err_msg->setFixedSize(600, 400);
                err_msg->critical(0,"Error", "Please choose model to inference");
                return;
            }

            ui->rad_img_rtmode->setChecked(false);
            img_save_flag = true;

            // 아직 image set이 선택되지 않은 상태
            if(ui->Img_Option->currentWidget() == ui->SelectImagesetPage){
                if(!on_btn_select_image_set_clicked()){
                    return;
                }
            }

            int evaluationSetId = -1;
            int imageSetId  = -1;
            {
                std::unique_lock<std::mutex> lock(m_save_info._mutex);
                imageSetId = m_save_info.imageSetId;
            }
            if(inf_mode_status == INF_MODE_SINGLE){
                evaluationSetId = m_db->getEvaluationSetID(imageSetId, currentModelId, -1);

                if(evaluationSetId == -1){
                    evaluationSetId = m_db->InsertEvaluationSet(imageSetId, currentModelId, -1);

                    if(evaluationSetId == -1){
                        qDebug() << "Evaluation set insert failed";
                        return;
                    }
                    initJson(evaluationSetId);
                }

                else{
                    bool init_json_flag = false;
                    {
                        std::unique_lock<std::mutex> lock(m_save_info._mutex);
                        QString path = m_db->getEvalutionJsonPath(evaluationSetId);

                        QFile& file = m_save_info.json_file;
                        file.setFileName(path);

                        if(!file.open(QIODevice::ReadWrite)){
                            qDebug() << "Failed to open json file.";
                            return;
                        }

                        QByteArray load_data = file.readAll();
                        m_save_info.json_doc = QJsonDocument::fromJson(load_data);
                        m_save_info.title = m_save_info.json_doc.object();
                        if(m_save_info.title.empty()){
                            init_json_flag = true; // mutex 때문에 block을 빠져나간 뒤에 initJson을 수행하게 함.
                        }
                        file.close();
                    }
                    if(init_json_flag){
                        initJson(evaluationSetId);
                    }
                }
            }
            else if(inf_mode_status == INF_MODE_DET_CLA){
                evaluationSetId = m_db->getEvaluationSetID(imageSetId, currentModelId, ensmbleModelId);
            }

            {
                std::unique_lock<std::mutex> lock(m_save_info._mutex);
                m_save_info.evaluationSetId = evaluationSetId;
            }

        }

        //다시 선택한 것
        else{
            return;
        }
    }
    else{
        if(!ui->rad_img_rtmode->isChecked())
            ui->rad_img_save_mode->setChecked(true);
    }
}

void MainWindow::on_rad_img_rtmode_toggled(bool checked){
    if(checked){
        // 새로 선택한 것
        if(ui->rad_img_save_mode->isChecked()){
            ui->rad_img_save_mode->setChecked(false);
            img_save_flag = false;
        }

        // 다시 선택한 것
        else{
            return;
        }
    }
    else{
        if(!ui->rad_img_save_mode->isChecked())
            ui->rad_img_rtmode->setChecked(true);
    }
}

void MainWindow::verticalResizeTable(QTableView* table_view){
    // vertical resize table widget to contents
    int totalRowHeight = 0;

    // visible row height
    int count =table_view->verticalHeader()->count();
    for(int i=0; i < count; i++){
        if(!table_view->verticalHeader()->isSectionHidden(i)){
            totalRowHeight +=  table_view->verticalHeader()->sectionSize(i);
        }
    }

    //scrollbar visibility
    if(!table_view->horizontalScrollBar()->isHidden()){
        totalRowHeight += table_view->horizontalScrollBar()->height();
    }

//    // horizontal header visibility
//    if(!table_view->horizontalHeader()->isHidden()){
//        totalRowHeight += table_view->horizontalHeader()->height();
//    }

    table_view->setMaximumHeight(totalRowHeight);
}

