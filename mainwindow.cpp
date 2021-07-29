#include "mainwindow.h"
#include "ui_mainwindow.h"

#define TERM_MIN 0.5
#define TERM_MAX 60.0

const string PathSeparator = QString(QDir::separator()).toStdString();

QString IMG_FORMAT = ".png";

QString CAM_TEXT = "Connect Camera";
QString CAM_FOLDER_TEXT = "Watch Camera Directory";

QString VIDEO_FILE_TEXT = "Video File";
QString VIDEO_FOLDER_TEXT = "Video Folder";

QVector<QColor> COLOR_VECTOR;

int ensmble_crop_size = 128;

cv::Scalar red(0, 0, 255);
cv::Scalar green(0, 255, 0);
cv::Scalar blue(255, 0, 0);
cv::Scalar white(255, 255, 255);
cv::Scalar black(0, 0, 0);

int COLOR_COL = 0, CLASS_COL = 1, ACTUAL_COL = 2, COUNT_COL = 3, AVG_SCORE_COL = 4;

bool save_worker_busy = false;

bool rotation_flag = false;

QString model1_filter = "";
QString model2_filter = "";

int score_col = 2;
int inf_col = 3;

QString set_model_thread(NrtExe* nrt_ptr, QString modelPath, bool fp16_flag){ 
    return nrt_ptr->set_model(modelPath, fp16_flag);
}

MainWindow::MainWindow(QWidget *parent) :
    QMainWindow(parent),
    ui(new Ui::MainWindow)
{
    ui->setupUi(this);
    this->setWindowTitle("Neuro-E");

    // TopBar Style
    ui->btn_settings->setToolTip("Settings");
    ui->btn_settings->setIconSize(QSize(ui->btn_settings->width()*0.5, ui->btn_settings->height()*0.5));
    ui->TopBar->setStyleSheet("background-color: rgb(54, 93, 157)");
    ui->edit_topbar_title->setStyleSheet("QLineEdit { border: none; color: rgb(255, 255, 255); }");

    // Camera Select ComboBox
    ui->com_cam_input_select->addItem(CAM_TEXT);
    ui->com_cam_input_select->addItem(CAM_FOLDER_TEXT);
    on_com_cam_input_select_currentTextChanged(CAM_TEXT);

    // Video Select Combobox
    ui->com_video_input->addItem(VIDEO_FILE_TEXT);
    ui->com_video_input->addItem(VIDEO_FOLDER_TEXT);
    on_com_video_input_currentTextChanged(VIDEO_FILE_TEXT);

    ui->com_video_list->setSizeAdjustPolicy(QComboBox::AdjustToContents);

    // Table Images
    ui->table_images->verticalHeader()->hide();
    ui->table_images->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->table_images->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    // Table Videos
    ui->table_video_files->verticalHeader()->hide();
    ui->table_video_files->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    ui->table_video_files->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);

    // 제어 버튼 모두 hide-> start inference 버튼 선택 후 필요한 버튼만 사용 가능.
    ui->btn_play->hide();
    ui->btn_pause->hide();
    ui->btn_stop->hide();
    ui->btn_disconnect_camera->hide();
    ui->btn_video_stop_inf->hide();
    ui->lab_time->hide();

    ui->media_info->setCurrentWidget(ui->media_info_cam);

    // 전체화면 뷰어 버튼
    btn_full_screen_viewer = new QPushButton(this);
    btn_full_screen_viewer->setFlat(true);
    btn_full_screen_viewer->setIcon(QIcon(":/icons/full_screen.png"));
    btn_full_screen_viewer->setIconSize(QSize(30, 30));

    QRect rect = ui->lab_show_res->geometry();
    QPoint bottom_right_point = QPoint(rect.x() + rect.width() - 50, rect.y() + rect.height() - 50);
    QPoint pos = ui->lab_show_res->mapTo(this, bottom_right_point);

    btn_full_screen_viewer->setGeometry(pos.x(), pos.y(), 30, 30);
    btn_full_screen_viewer->show();

    connect(btn_full_screen_viewer, SIGNAL(clicked()), this, SLOT(on_btn_full_screen_viewer_clicked()));

    lab_full_screen->setAlignment(Qt::AlignCenter);

    // show prediction 버튼
    btn_show_prediction =  new QPushButton(this);
    btn_show_prediction->setFlat(true);
    btn_show_prediction->setIcon(QIcon(":/icons/show.png"));
    btn_show_prediction->setIconSize(QSize(30, 30));

    pos.setX(pos.x() - 50);
    btn_show_prediction->setGeometry(pos.x(), pos.y(), 30, 30);
    btn_show_prediction->show();

    connect(btn_show_prediction, &QPushButton::clicked, [&]{
        show_pred_flag = !show_pred_flag;

        if(show_pred_flag){
            btn_show_prediction->setIcon(QIcon(":/icons/show.png"));
        }
        else{
            btn_show_prediction->setIcon(QIcon(":/icons/unshow.png"));
        }
    });

    // loader gif
    movie = new QMovie(this);
    movie->setFileName(":/icons/loader.gif");
    movie_connection = connect(movie, &QMovie::frameChanged, [&]{
        ui->btn_select_single_mode->setIcon(movie->currentPixmap());
    });

    // file system watcher 초기화
    file_sys_watcher = new QFileSystemWatcher(this);
    ui->list_watch_dirs->setSortingEnabled(true);

    // Ensemble Options
    QStringList options = {
        "Detection -> Classification",
        "Segmentation -> Classification",
        "Classification -> Detection",
        "Classification -> Segmentation",
        "Classification -> Classification"
    };
    ui->com_ensemble_options->addItems(options);
    on_com_ensemble_options_currentIndexChanged(0);

    connect(ui->table_model1, SIGNAL(cellClicked(int, int)), this, SLOT(model_table_item_clicked(int, int)));
    connect(ui->table_model1, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(model_table_item_clicked(int, int)));
    connect(ui->table_model1, SIGNAL(cellChanged(int, int)), this, SLOT(model_table_item_changed(int, int)));

    connect(ui->table_model2, SIGNAL(cellClicked(int, int)), this, SLOT(model_table_item_clicked(int, int)));
    connect(ui->table_model2, SIGNAL(cellDoubleClicked(int, int)), this, SLOT(model_table_item_clicked(int, int)));
    connect(ui->table_model2, SIGNAL(cellChanged(int, int)), this, SLOT(model_table_item_changed(int, int)));

    ui->Settings_Realtime_Review_Stack->setCurrentWidget(ui->run_realtime_page);
    on_btn_run_clicked();

    ready_for_inference_check();

    QSqlError err = m_db->InitialDBSetup();
    if(err.type() != QSqlError::NoError){
        qDebug() << "Initial DB Setup failed: " << err;
        m_db.reset();
    }

    root_path = m_db->getSavePath("main_thread");
    if(root_path == ""){
        root_path = "C:/Neuro-E/images";
        m_db->ReplaceSavePath(root_path);
    }

    QDir root_dir(root_path);
    root_dir.mkpath(root_path);
    qDebug() << root_path;

    QString year_month = QDate::currentDate().toString("yyyy-MM");
    QString year_month_day = QDate::currentDate().toString("yyyy-MM-dd");

    current_save_path = root_path + "/" + year_month + "/" + year_month_day;
    qDebug() << current_save_path;
}

MainWindow::~MainWindow()
{
    delete ui;
}

void MainWindow::center_and_resize()
{
    QSize availableSize = QGuiApplication::screens().at(0)->availableSize();
    int width = availableSize.width() * 0.7;
    int height = availableSize.height() * 0.7;
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

bool MainWindow::eventFilter(QObject* obj, QEvent* ev){
    if(obj == full_screen_dialog && ev->type() == QEvent::NonClientAreaMouseButtonDblClick){
        QSize availableSize = QGuiApplication::screens().at(0)->availableSize();
        int width = availableSize.width();
        int height = availableSize.height();
        QSize newSize(width, height);

        if(full_screen_dialog->isMaximized()){
            full_screen_dialog->show();
        }
        else{
            full_screen_dialog->showMaximized();
        }

        return true;
    }
    return false;
}

void MainWindow::resizeEvent(QResizeEvent *event){
    QRect rect = ui->lab_show_res->geometry();
    QPoint bottom_right_point = QPoint(rect.x() + rect.width() - 50, rect.y() + rect.height() - 50);

    QPoint pos = ui->lab_show_res->mapTo(this, bottom_right_point);

    btn_full_screen_viewer->move(pos);

    pos.setX(pos.x() - 50);
    btn_show_prediction->move(pos);
}



void MainWindow::claSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name){
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

            if(!m_nrt->m_roi_info.empty()){
                cv::resize(cam_colormap_mat, cam_colormap_mat, cv::Size(m_nrt->resized_roi_w, m_nrt->resized_roi_h), 0, 0, cv::INTER_LINEAR);
                cv::Rect roi = cv::Rect(m_nrt->resized_roi_x, m_nrt->resized_roi_y, m_nrt->resized_roi_w, m_nrt->resized_roi_h);
                cv::addWeighted(cam_colormap_mat, 0.4, PRED_IMG(roi), 0.6, 0.0, PRED_IMG(roi));
            }
            else{
                cv::resize(cam_colormap_mat, cam_colormap_mat, cv::Size(PRED_IMG.cols, PRED_IMG.rows), 0, 0, cv::INTER_LINEAR);
                cv::addWeighted(cam_colormap_mat, 0.4, PRED_IMG, 0.6, 0.0, PRED_IMG);
            }
        }
    }

    std::string pred_cls = "";
    int pred_cls_idx = -1;
    float pred_score = 0;
    if(m_nrt->PRED_IDX != -1 || m_nrt->PROB_IDX != -1) {
        // Predited Class
        output_pred = outputs.get_at(m_nrt->PRED_IDX);
        nrt::Shape output_pred_shape = output_pred.get_shape();

        for (int i = 0; i < output_pred_shape.dims[0]; i++) {
            pred_cls_idx = *output_pred.get_at_ptr<int>(i);
        }
        pred_cls = m_nrt->get_model_class_name(pred_cls_idx).toStdString();

        // Prob threshold
        float cur_prob_thres;
        if(pred_cls_idx < m_nrt->prob_threshold.length()){
            cur_prob_thres = m_nrt->prob_threshold[pred_cls_idx];
        }
        else{
            qDebug() << "claSetResults) " << "prob_threshold vector out of range";
            cur_prob_thres = 90;
        }

        // Predicted score
        output_prob = outputs.get_at(m_nrt->PROB_IDX);
        pred_score = (*output_prob.get_at_ptr<float>(0, pred_cls_idx)) * 100;

        if (pred_score < cur_prob_thres)
            pred_cls = "Unknown";

        ui->edit_show_class->setText(QString::fromStdString(pred_cls));
        ui->edit_show_score->setText(QString::number(pred_score, 'f', 2) + "%");
    }
    new_row.push_back(pred_cls);

    std::ostringstream out;
    out.precision(2);
    out << std::fixed << pred_score;
    new_row.push_back(out.str());

    avg_scores_model1[pred_cls_idx] += pred_score;
    class_ratio_model1[pred_cls_idx] ++;

    if(media_mode == MEDIA_MODE_CAM || media_mode == MEDIA_MODE_VIDEO){
        if(inferenced_images == 1 || inferenced_images % 10 == 0){
            update_pie_chart(0, 0);

            // update total image count
            ui->edit_total_1->setText(QString::number(inferenced_images));
        }
    }
    else{
        update_pie_chart(0, 0);

        // update total image count
        ui->edit_total_1->setText(QString::number(inferenced_images));
    }

    //update avg prob (image mode: every image, camvideo: every 5 frame)
    if(inferenced_images == 1 || inferenced_images % update_result_table_rate == 0){
        update_results_table(0, 0);
    }

    // Image Name
    if(class_include_flag){
        cur_img_name = QString::fromStdString(pred_cls) + "_" + cur_img_name;
    }
    if(!src_prefix.isEmpty()){
        cur_img_name = src_prefix + "_" + cur_img_name;
    }
}

void MainWindow::segSetResults(nrt::NDBuffer merged_pred_output, nrt::NDBuffer merged_prob_output, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name){
    vector<float> seg_avg_score(m_nrt->get_model_class_num(), 0); // 각 blob의 maen score를 구한 후, 이를 class 단위로 합산. -> class 별 average score 값으로 활용
    vector<long long> seg_pixel_cnt(m_nrt->get_model_class_num(), 0); // class 마다 전체 이미지에서 차지하는 pixel 갯수
    vector<int> seg_blob_count(m_nrt->get_model_class_num(), 0);

    long long total_pixel = PRED_IMG.rows * PRED_IMG.cols;
    QString pred_class = "";

    if (!merged_pred_output.empty() && !merged_prob_output.empty()) {
        nrt::Status status;

        //Threshold by pixel prob
        nrt::NDBuffer prob_threshold_buf = nrt::NDBuffer::zeros(m_nrt->get_model_class_num(), nrt::DTYPE_FLOAT32);
        float* prob_threshold_ptr = prob_threshold_buf.get_at_ptr<float>();
        for(int i = 0; i < m_nrt->get_model_class_num(); i++){
            float value = m_nrt->prob_threshold[i];
            prob_threshold_ptr[i] = value;
        }
        status = nrt::pred_map_threshold_by_pixel_prob(merged_pred_output, merged_prob_output, prob_threshold_buf);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "pred_map_threshold_by_pixel_prob failed.  : " << QString(nrt::get_last_error_msg());
        }

        // Treshold by blob prob
        status = nrt::pred_map_threshold_by_blob_prob(merged_pred_output, merged_prob_output, prob_threshold_buf);
        if(status != nrt::STATUS_SUCCESS){
            qDebug() << "pred_map_threshold_by_blob_prob failed. : " << QString(nrt::get_last_error_msg());
        }

        // Threshold by size
        nrt::NDBuffer bounding_rects;
        nrt::NDBuffer size_threshold_buf = nrt::NDBuffer::zeros(nrt::Shape(m_nrt->get_model_class_num(), 3), nrt::DTYPE_INT32);
        int* thres_ptr = size_threshold_buf.get_at_ptr<int>();
        for(int i = 0; i < m_nrt->get_model_class_num(); i++){
          int height = m_nrt->size_threshold[i].first;
          int width = m_nrt->size_threshold[i].second;
          int conjunction = m_nrt->size_thres_conjunction[i] == "AND" ? 0 : 1;
          thres_ptr[3*i + 0] = height;
          thres_ptr[3*i + 1] = width;
          thres_ptr[3*i + 2] = conjunction;
        }
        status = nrt::pred_map_threshold_by_size(merged_pred_output, bounding_rects, size_threshold_buf, m_nrt->get_model_class_num());
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "pred_map_threshold_by_size failed.  : " << QString(nrt::get_last_error_msg());
        }

        // Class Name
        unsigned char* output_ptr = merged_pred_output.get_at_ptr<unsigned char>(0);
        nrt::Shape bounding_rects_shape = bounding_rects.get_shape();

        int img_w = PRED_IMG.cols;
        for (int j = 0; j < bounding_rects_shape.dims[0]; j++) {
            int* output_rect_ptr = bounding_rects.get_at_ptr<int>(j);
            int image_batch_index = output_rect_ptr[0];
            int rect_x = output_rect_ptr[1];
            int rect_y = output_rect_ptr[2];
            int rect_h = output_rect_ptr[3];
            int rect_w = output_rect_ptr[4];
            int rect_class_index = output_rect_ptr[5];

            if (rect_class_index < 1 || rect_class_index >= m_nrt->get_model_class_num())
                continue;

            double alp_src = 0.5, alp_mask = 0.5;
            QColor cur_class_color = COLOR_VECTOR[rect_class_index-1];
            int r, g, b;
            cur_class_color.getRgb(&r, &g, &b);
            cv::Scalar cur_class_color_scalar = cv::Scalar(b, g, r);

            float blob_mean_score = 0;
            int total_blob_pixel_size = 0;

            int cur_ofs = 0;
            for(int h = rect_y; h < rect_y + rect_h; h++){
                cur_ofs = h * img_w;
                for(int w = rect_x; w < rect_x + rect_w; w++){
                    int cur_class = output_ptr[cur_ofs + w];
                    if(cur_class != rect_class_index)
                        continue;

                    seg_pixel_cnt[cur_class] ++;

                    cv::Vec3b pix = PRED_IMG.at<cv::Vec3b>(h, w);
                    pix[2] = (unsigned char)(((double)pix[2] * alp_src) + ((double)cur_class_color.red() * alp_mask));
                    pix[1] = (unsigned char)(((double)pix[1] * alp_src) + ((double)cur_class_color.green() * alp_mask));
                    pix[0] = (unsigned char)(((double)pix[0] * alp_src) + ((double)cur_class_color.blue() * alp_mask));
                    PRED_IMG.at<cv::Vec3b>(h, w) = pix;

                    // prob map size [batch index, height, width, num of classes]
                    float pixel_prob = *merged_prob_output.get_at_ptr<float>(image_batch_index, h, w, cur_class);
                    blob_mean_score += pixel_prob * 100;
                    total_blob_pixel_size ++;
                }
            }

            if(total_blob_pixel_size == 0){
                continue;
            }
            seg_blob_count[rect_class_index]++;
            class_ratio_model1[rect_class_index]++;

            blob_mean_score = blob_mean_score / total_blob_pixel_size;
            seg_avg_score[rect_class_index] += blob_mean_score; // 해당 이미지에서의 class 별 blob mean score
            avg_scores_model1[rect_class_index] += blob_mean_score; // 모든 이미지에서의 class 별 blob mean score

            QString classname = m_nrt->get_model_class_name(rect_class_index);
            if(!pred_class.isEmpty()){
                pred_class += ", ";
            }
            pred_class += classname;

            if (!classname.isEmpty()) {
                cv::putText(PRED_IMG, classname.toStdString() + "(" + QString::number(blob_mean_score, 'f', 2).toStdString() +"%)", cv::Point(rect_x, rect_y), FONT_HERSHEY_SIMPLEX, 0.7 , cur_class_color_scalar, 2);
            }
        }
    }

    int total_blob_count = std::accumulate(seg_blob_count.begin(), seg_blob_count.end(), 0);
    bool ok;
    int current_total_blob = ui->edit_total_1->text().toInt(&ok);
    if(!ok)
        current_total_blob = 0;
    ui->edit_total_1->setText(QString::number(current_total_blob + total_blob_count));

    // new_row[2 ~ (2 + num_of_classes - 1)] : class 별로 이미지 전체에서 차지하는 영역 비율 (pixel rate)
    for(int i = 1; i < m_nrt->get_model_class_num(); i++){
        float cur_image_class_pixel_rate = ((float)seg_pixel_cnt[i] / (float)total_pixel) * 100;
        new_row.push_back(QString::number(cur_image_class_pixel_rate, 'f', 2).toStdString());

        if(inferenced_images == 1 || inferenced_images % update_result_table_rate == 0){
            if(ui->table_model1->item(i-1, COUNT_COL)){
                ui->table_model1->item(i-1, COUNT_COL)->setText(QString::number(cur_image_class_pixel_rate, 'f', 2) + "%");
            }
        }
    }

    // new_row[(3 + num_of_classes) ~ (3 + 2 * num_of_classes -1)] : class 별로 avg score (blob의 mean score)
    QString class_info_edit = "";
    for(int i = 1; i < m_nrt->get_model_class_num(); i++){
        float cur_class_avg_score = 0;

        if(seg_blob_count[i] != 0){
            cur_class_avg_score = seg_avg_score[i] / (float)seg_blob_count[i];

            if(!class_info_edit.isEmpty()){
                class_info_edit += ", ";
            }
            class_info_edit += m_nrt->get_model_class_name(i);
        }

        new_row.push_back(QString::number(cur_class_avg_score, 'f', 2).toStdString());
    }

    // new_row[1]: predicted class
    ui->edit_show_class->setText(class_info_edit);
    new_row.push_back(class_info_edit.toStdString());

    // new_row[last] : 현재 이미지의 평균 score (blob prob들의 평균)
    float image_avg_score = 0;
    if(total_blob_count != 0){
        image_avg_score = (float)std::accumulate(seg_avg_score.begin(), seg_avg_score.end(), 0) / (float)total_blob_count;
    }
    new_row.push_back(QString::number(image_avg_score, 'f', 2).toStdString());
    ui->edit_show_score->setText(QString::number(image_avg_score, 'f', 2) + "%");

    if(media_mode == MEDIA_MODE_CAM || media_mode == MEDIA_MODE_VIDEO){
        if(inferenced_images == 1 || inferenced_images % 10 == 0){
            update_pie_chart(0, 1);
        }
    }
    else{
        update_pie_chart(0, 1);
    }

    if(inferenced_images == 1 || inferenced_images % update_result_table_rate == 0){
        update_results_table(0, 1);
    }

    // Image Name
    if(class_include_flag && !class_info_edit.isEmpty()){
        cur_img_name = class_info_edit.replace(", ", "_") + "_" + cur_img_name;
    }
    if(!src_prefix.isEmpty()){
        cur_img_name = src_prefix + "_" + cur_img_name;
    }
}

void MainWindow::detSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name){
    vector<int> det_box_cnt(m_nrt->get_model_class_num(), 0);
    vector<float> det_box_prob(m_nrt->get_model_class_num(), 0);
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

            if (bbox.class_number < 1 || bbox.class_number >= m_nrt->get_model_class_num()){
                qDebug() << "Detection Box class is either background or out of range.";
                continue;
            }

            // Threshold by size
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
            float cur_prob = probs[bbox.class_number + 1] * 100;
            float cur_prob_thres = m_nrt->prob_threshold[bbox.class_number];
            if (cur_prob < cur_prob_thres){
                continue;
            }

            exist_class[bbox.class_number] = true;

            det_box_cnt[bbox.class_number] ++;
            class_ratio_model1[bbox.class_number] ++;

            det_box_prob[bbox.class_number] += cur_prob;
            avg_scores_model1[bbox.class_number] += cur_prob;

            int r, g, b;
            COLOR_VECTOR[bbox.class_number - 1].getRgb(&r, &g, &b);
            cv::Scalar class_color_scalar = cv::Scalar(b, g, r);
            cv::rectangle(PRED_IMG,
                          cv::Point(bbox.box_center_X - bbox.box_width/2, bbox.box_center_Y - bbox.box_height/2),
                          cv::Point(bbox.box_center_X + bbox.box_width/2 + (bbox.box_width % 2), bbox.box_center_Y + bbox.box_height / 2 + (bbox.box_height % 2)),
                          class_color_scalar,
//                          red,
                          2
                          );
            QString classname = m_nrt->get_model_class_name(bbox.class_number);

            classname += "(" + QString::number(cur_prob, 'f', 2) + "%)";

            if (!classname.isEmpty()) {
                cv::putText(PRED_IMG, classname.toLocal8Bit().constData(), cv::Point(bbox.box_center_X - bbox.box_width / 2, bbox.box_center_Y - bbox.box_height / 2), FONT_HERSHEY_SIMPLEX, 1, white, 7);
                cv::putText(PRED_IMG, classname.toLocal8Bit().constData(), cv::Point(bbox.box_center_X - bbox.box_width / 2, bbox.box_center_Y - bbox.box_height / 2), FONT_HERSHEY_SIMPLEX, 1, class_color_scalar, 4);
            }
        }

        // new_row[1] : predicted class
        QString class_info_edit = "";
        for(int i = 0; i < m_nrt->get_model_class_num(); i++){
            if(exist_class[i]){
                if(!class_info_edit.isEmpty()){
                    class_info_edit += ", ";
                }
                class_info_edit += m_nrt->get_model_class_name(i);
            }
        }
        ui->edit_show_class->setText(class_info_edit);
        new_row.push_back(class_info_edit.toStdString());

        // new_row[2 ~ 2 + (number of classes) - 1] : class 별 detected box 개수
        int total_box_count = 0;
        for(int i = 1; i < det_box_cnt.size(); i++){
            new_row.push_back(to_string(det_box_cnt[i]));

            if(inferenced_images == 1 || inferenced_images % update_result_table_rate == 0){
                if(ui->table_model1->item(i-1, COUNT_COL)){
                    ui->table_model1->item(i-1, COUNT_COL)->setText(QString::number(det_box_cnt[i]));
                }

                if(detect_count_flag){
                    int actual = ui->table_model1->item(i-1, ACTUAL_COL)->text().toInt();
                    int count = ui->table_model1->item(i-1, COUNT_COL)->text().toInt();
                    if(actual != count){
                        ui->table_model1->item(i-1, COUNT_COL)->setTextColor(QColor(255, 0, 0));
                        ui->table_model1->item(i-1, ACTUAL_COL)->setTextColor(QColor(255, 0, 0));
                    }
                    else{
                        ui->table_model1->item(i-1, COUNT_COL)->setTextColor(QColor(0, 0, 0));
                        ui->table_model1->item(i-1, ACTUAL_COL)->setTextColor(QColor(0, 0, 0));
                    }
                }
            }

            total_box_count += det_box_cnt[i];
        }

        // new_row[2 + (number of classes)] : total number of boxes
        bool ok;
        int current_total_box = ui->edit_total_1->text().toInt(&ok);
        if(!ok)
            current_total_box = 0;
        ui->edit_total_1->setText(QString::number(current_total_box + total_box_count));
        new_row.push_back(to_string(total_box_count));

        // new_row[] : class 별 avg score (각 클래스에 해당하는 box들 끼리, box prob을 평균 낸 것)
        float total_prob = 0;
        for(int i = 0; i < det_box_prob.size(); i++){
            float cur_class_avg_score = 0;

            if(det_box_cnt[i] != 0){
                cur_class_avg_score = det_box_prob[i] / (float)det_box_cnt[i];
            }

            total_prob += det_box_prob[i];
            new_row.push_back(QString::number(cur_class_avg_score, 'f', 2).toStdString());
        }

        // new_row[마지막] : box들의 eman score
        float image_avg_score = 0;
        if(total_box_count !=0){
            image_avg_score = total_prob / (int)total_box_count;
        }
        new_row.push_back(QString::number(image_avg_score, 'f', 2).toStdString());
        ui->edit_show_score->setText(QString::number(image_avg_score, 'f', 2) + "%");

        if(media_mode == MEDIA_MODE_CAM || media_mode == MEDIA_MODE_VIDEO){
            if(inferenced_images == 1 || inferenced_images % 10 == 0){
                update_pie_chart(0, 1);
            }
        }
        else{
            update_pie_chart(0, 1);
        }

        if(inferenced_images == 1 || inferenced_images % update_result_table_rate == 0){
            update_results_table(0, 1);
        }

        // Image Name
        if(class_include_flag && !class_info_edit.isEmpty()){
            class_info_edit.replace(", ", "_");
            cur_img_name = class_info_edit + "_" + cur_img_name;
        }
        if(!src_prefix.isEmpty()){
            cur_img_name = src_prefix + "_" + cur_img_name;
        }
    }
}

void MainWindow::ocrSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name) {
    vector<BoundingBox> exist_bbox;
    float box_mean_score = 0;
    int number_of_boxes = 0;

    if (m_nrt->PRED_IDX != -1 && m_nrt->PROB_IDX != -1) {
        nrt::Shape input_image_shape = m_nrt->get_model_input_shape(0);
        int input_h = PRED_IMG.rows;
        int input_w = PRED_IMG.cols;
        double h_ratio = (double)input_image_shape.dims[0] / input_h;
        double w_ratio = (double)input_image_shape.dims[1] / input_w;
        if(rotation_flag && m_nrt->ROT_IDX != -1){
            h_ratio = 1.0;
            w_ratio = 1.0;
        }

        nrt::NDBuffer output_box;
        nrt::NDBuffer output_prob;
        nrt::NDBuffer output_rot;

        output_box = outputs.get_at(m_nrt->PRED_IDX);
        output_prob = outputs.get_at(m_nrt->PROB_IDX);

        if(rotation_flag && m_nrt->ROT_IDX != -1){
            output_rot = outputs.get_at(m_nrt->ROT_IDX);
            // Rotation apply needed!
            int batch_size = 1;
            vector<int> rotation_degree_vector;
            for(int j = 0; j < batch_size; j++){
                int degree = output_rot.get_data_ptr<int>()[j];
                rotation_degree_vector.push_back(degree);

                auto M = cv::getRotationMatrix2D({ static_cast<float>(input_w) / 2, static_cast<float>(input_h) / 2 }, int(degree), 1.0);
                cv::warpAffine(PRED_IMG, PRED_IMG, M, { input_w, input_h }, cv::INTER_CUBIC, cv::BORDER_REPLICATE);
            }
        }

        nrt::Shape output_box_shape = output_box.get_shape();
        number_of_boxes = output_box_shape.dims[0];
        const int* bcyxhw_ptr = output_box.get_data_ptr<int>();

        nrt::Shape output_prob_shape = output_prob.get_shape();
        int prob_interval = output_prob_shape.dims[output_prob_shape.num_dim - 1];
        const float* prob_ptr = output_prob.get_data_ptr<float>();

        for (int box_idx = 0; box_idx < number_of_boxes; ++box_idx) {
            BoundingBox bbox = convert_to_bounding_box(bcyxhw_ptr + box_idx * 6, h_ratio, w_ratio);

            const float* probs = prob_ptr + box_idx * prob_interval;

            float pred_prob = probs[bbox.class_number + 1] * 100;

            if(pred_prob < m_nrt->prob_threshold[0]){
                continue;
            }

            exist_bbox.push_back(bbox);

            int r, g, b;
            COLOR_VECTOR[0].getRgb(&r, &g, &b);
            cv::Scalar class_color_scalar(b, g, r);
            cv::rectangle(PRED_IMG,
                          cv::Point(bbox.box_center_X - bbox.box_width / 2, bbox.box_center_Y - bbox.box_height / 2),
                          cv::Point(bbox.box_center_X + bbox.box_width / 2 + (bbox.box_width % 2), bbox.box_center_Y + bbox.box_height / 2 + (bbox.box_height % 2)),
                          class_color_scalar,
                          2
                          );

            if(bbox.class_number < 1 || bbox.class_number >  m_nrt->get_model_class_num()){
                qDebug() << "OCR Box class is either background or out of range.";
                continue;
            }
            QString classname = m_nrt->get_model_class_name(bbox.class_number);
            box_mean_score += pred_prob;

            if (!classname.isEmpty()) {
                double font_scale = cv::getFontScaleFromHeight(FONT_HERSHEY_SIMPLEX, 30, 4);
                int scale_gap = (int)(font_scale - 1) * 60;

                cv::putText(PRED_IMG, classname.toStdString(), cv::Point(bbox.box_center_X - (bbox.box_width / 2), bbox.box_center_Y + bbox.box_height + scale_gap), FONT_HERSHEY_SIMPLEX, font_scale, white, 7, 8, false);
                cv::putText(PRED_IMG, classname.toStdString(), cv::Point(bbox.box_center_X - (bbox.box_width / 2), bbox.box_center_Y + bbox.box_height + scale_gap), FONT_HERSHEY_SIMPLEX, font_scale, class_color_scalar, 4, 8, false);
            }
        }
        sort(exist_bbox.begin(), exist_bbox.end(), bbox_cmp);

        // OCR String in edit_show_class
        QString ocr_string = "";
        for (int i = 0; i < exist_bbox.size(); i++) {
            QString classname = m_nrt->get_model_class_name(exist_bbox[i].class_number);
            ocr_string += classname;
        }
        ui->edit_show_class->setText(ocr_string);
        new_row.push_back(ocr_string.toStdString());

        // Total mean score
        if(number_of_boxes != 0){
            box_mean_score /= (float)number_of_boxes;
            class_ratio_model1[0] ++;
            avg_scores_model1[0] += box_mean_score;

            ui->edit_show_score->setText(QString::number(box_mean_score, 'f', 2) + "%");
        }
        else{
            ui->edit_show_score->setText("0%");
        }
        new_row.push_back(QString::number(box_mean_score, 'f', 2).toStdString());

        if(inferenced_images == 1 || inferenced_images % update_result_table_rate == 0){
            float overall_avg_score;
            if(class_ratio_model1[0] != 0){
                overall_avg_score = avg_scores_model1[0] / (float)class_ratio_model1[0];
            }
            else{
                overall_avg_score = 0;
            }

            if(ui->table_model1->item(0, AVG_SCORE_COL)){
                ui->table_model1->item(0, AVG_SCORE_COL)->setText(QString::number(overall_avg_score, 'f', 2) + "%");
            }

            if(ui->table_model1->item(0, COUNT_COL)){
                ui->table_model1->item(0, COUNT_COL) ->setText(QString::number(number_of_boxes));
            }

            if(detect_count_flag){
                int actual = ui->table_model1->item(0, ACTUAL_COL)->text().toInt();

                if(actual != number_of_boxes){
                    ui->table_model1->item(0, COUNT_COL)->setTextColor(QColor(255, 0, 0));
                    ui->table_model1->item(0, ACTUAL_COL)->setTextColor(QColor(255, 0, 0));
                }
                else{
                    ui->table_model1->item(0, COUNT_COL)->setTextColor(QColor(0, 0, 0));
                    ui->table_model1->item(0, COUNT_COL)->setTextColor(QColor(0, 0, 0));
                }
            }
        }

        // image name
        if(class_include_flag && !ocr_string.isEmpty()){
            cur_img_name = ocr_string + "_" + cur_img_name;
        }
        if(!src_prefix.isEmpty()){
            cur_img_name = src_prefix + "_" + cur_img_name;
        }
    }
}

void MainWindow::anomSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name) {
    if(m_nrt->PRED_IDX != -1 && m_nrt->ANO_IDX != -1 && m_nrt->BOX_IDX != -1){
        nrt::NDBuffer output_anom_score;
        nrt::NDBuffer output_pred;
        nrt::NDBuffer output_boxes;

        // Predicted Class
        output_pred = outputs.get_at(m_nrt->PRED_IDX);
        auto output_pred_shp = output_pred.get_shape();
        int pred_cls_idx = -1;
        QString pred_cls = "";
        for(int j = 0; j < output_pred_shp.dims[0]; j++){
            pred_cls_idx = *output_pred.get_at_ptr<int>(j);
            pred_cls = m_nrt->get_model_class_name(pred_cls_idx);
        }
        new_row.push_back(to_string(pred_cls_idx));

        // Anomaly Score (degree of deviation from normal)
        output_anom_score = outputs.get_at(m_nrt->ANO_IDX);
        auto output_anom_score_shp = output_anom_score.get_shape();
        float pred_anom_score = 0.0;
        for(int j = 0; j < output_anom_score_shp.dims[0]; j++){
            pred_anom_score = *output_anom_score.get_at_ptr<float>(j);
        }
        QString score_str = QString::number(pred_anom_score, 'f', 3);
        new_row.push_back(score_str.toStdString());

        ui->edit_show_class->setText(pred_cls);
        ui->edit_show_score->setText(score_str);

        avg_scores_model1[pred_cls_idx] += pred_anom_score;
        class_ratio_model1[pred_cls_idx] ++;

        // Draw Anomaly Boxes
        auto input_image_shp = m_nrt->get_model_input_shape(0);
        int input_h = PRED_IMG.rows;
        int input_w = PRED_IMG.cols;
        double h_ratio = (double)input_image_shp.dims[0] / input_h;
        double w_ratio = (double)input_image_shp.dims[1] / input_w;

        output_boxes = outputs.get_at(m_nrt->BOX_IDX);
        auto output_boxes_shp = output_boxes.get_shape();
        int number_of_boxes = output_boxes_shp.dims[0];
        const int* bcyxhw_ptr = output_boxes.get_data_ptr<int>();

        for(int box_idx = 0; box_idx < number_of_boxes; box_idx ++){
            BoundingBox bbox = convert_to_bounding_box(bcyxhw_ptr + box_idx * 6, h_ratio, w_ratio);

            if(bbox.class_number < 0 || bbox.class_number > 1){
                qDebug() << "Anomaly box out of range";
                continue;
            }

            int r, g, b;
            COLOR_VECTOR[bbox.class_number].getRgb(&r, &g, &b);
            cv::Scalar class_color_scalar = cv::Scalar(b, g, r);
            cv::rectangle(PRED_IMG,
                          cv::Point(bbox.box_center_X - bbox.box_width/2, bbox.box_center_Y - bbox.box_height/2),
                          cv::Point(bbox.box_center_X + bbox.box_width/2 + (bbox.box_width % 2), bbox.box_center_Y + bbox.box_height / 2 + (bbox.box_height % 2)),
                          class_color_scalar,
                          2
                          );
        }

        if(media_mode == MEDIA_MODE_CAM || media_mode == MEDIA_MODE_VIDEO){
            if(inferenced_images == 1 || inferenced_images % 10 == 0){
                update_pie_chart(0, 0);
            }
        }
        else{
            update_pie_chart(0, 0);
        }

        // update total image count
        ui->edit_total_1->setText(QString::number(inferenced_images));

        // update avg prob (image mode: every image, cam video: every 5 frame)
        if(inferenced_images == 1 || inferenced_images % update_result_table_rate == 0){
            update_results_table(0, 0);
        }

        // Image Name
        if(class_include_flag){
            cur_img_name = pred_cls + "_" + cur_img_name;
        }
        if(!src_prefix.isEmpty()){
            cur_img_name = src_prefix + "_" + cur_img_name;
        }
    }
}

void MainWindow::detClaEnsmbleSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name){
    if(crop_h > PRED_IMG.rows){
        crop_h = PRED_IMG.rows;
    }
    if(crop_w > PRED_IMG.cols){
        crop_w = PRED_IMG.cols;
    }

    vector<int> det_box_cnt(m_nrt->get_model_class_num(), 0);
    vector<float> det_box_probs(m_nrt->get_model_class_num(), 0.0);

    vector<int> cla_crop_cnt(m_nrt_ensmble->get_model_class_num(), 0);
    vector<float> cla_crop_score(m_nrt_ensmble->get_model_class_num(), 0.0);
    vector<bool> cla_crop_exist(m_nrt_ensmble->get_model_class_num(), false);

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

        cv::Mat intact_frame;
        PRED_IMG.copyTo(intact_frame);

        for(int box_idx = 0; box_idx < num_of_boxes; box_idx++){
            BoundingBox bbox = convert_to_bounding_box(bcyxhw_ptr + box_idx * 6, h_ratio, w_ratio);

            if (bbox.class_number < 1 || bbox.class_number > m_nrt->get_model_class_num()){
                qDebug() << "Detection Box class is either background or out of range.";
                continue;
            }

            if(!model1_class_trigger_map[bbox.class_number]){
                qDebug() << "This class does not trigger the execution of the second model." << m_nrt->get_model_class_name(bbox.class_number);
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

            det_box_cnt[bbox.class_number]++;
            class_ratio_model1[bbox.class_number]++;
            det_box_probs[bbox.class_number] += cur_box_pred_class_prob;
            avg_scores_model1[bbox.class_number] += cur_box_pred_class_prob;

            int x_start = bbox.box_center_X - crop_w/2;
            int y_start = bbox.box_center_Y - crop_h/2;

            if(x_start < 0) {
                x_start = 0;
            }
            if(x_start + crop_w > PRED_IMG.cols){
                x_start = PRED_IMG.cols - crop_w;
            }
            if(y_start < 0){
                y_start = 0;
            }
            if(y_start + crop_h > PRED_IMG.rows){
                y_start = PRED_IMG.rows - crop_h;
            }

            cv::Rect bounds(x_start, y_start, crop_w, crop_h);
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
                    cla_crop_exist[pred_cls_idx] = true;
                    cla_crop_cnt[pred_cls_idx]++;
                    class_ratio_model2[pred_cls_idx]++;
                    cla_crop_score[pred_cls_idx] += pred_cls_prob;
                    avg_scores_model2[pred_cls_idx] += pred_cls_prob;
                }
                int r, g, b;
                COLOR_VECTOR[pred_cls_idx].getRgb(&r, &g, &b);
                class_color_scalar = cv::Scalar(b, g, r);
            }
            else{
                continue;
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

            classname += "(" + QString::number(cur_box_pred_class_prob) + "%, " +QString::number(pred_cls_prob) +"%)";

            if (!classname.isEmpty()) {
                cv::putText(PRED_IMG, classname.toStdString(), cv::Point(bbox.box_center_X - bbox.box_width / 2, bbox.box_center_Y - bbox.box_height / 2), FONT_HERSHEY_SIMPLEX, 1, white, 4);
                cv::putText(PRED_IMG, classname.toStdString(), cv::Point(bbox.box_center_X - bbox.box_width / 2, bbox.box_center_Y - bbox.box_height / 2), FONT_HERSHEY_SIMPLEX, 1, class_color_scalar, 2);
            }
        }

        QString show_class_str;
        for(int i=0; i < m_nrt_ensmble->get_model_class_num(); i++){
            if(cla_crop_exist[i]){
                if(show_class_str != ""){
                    show_class_str += ", ";
                }
                show_class_str += m_nrt_ensmble->get_model_class_name(i);
            }
        }
        ui->edit_show_class->setText(QString(" ") + show_class_str);

        // Cla: crop하여 분류한 이미지 개수
        int total_crop_regions = std::accumulate(cla_crop_cnt.begin(), cla_crop_cnt.end(), 0);
        bool ok;
        int current_total_region = ui->edit_total_2->text().toInt(&ok);
        if(!ok)
            current_total_region = 0;
        ui->edit_total_2->setText(QString::number(current_total_region + total_crop_regions));

        // Cal: class별 평균 prob
//        for(int i = 0; i < cla_crop_score.size(); i++){
//            float cur_class_avg_score = 0;

//            if(cla_crop_cnt[i] != 0){
//                cur_class_avg_score = cla_crop_score[i] / (float)det_box_cnt[i];
//            }
//        }

        // Detection과 Classification의 클래스 별 평균 score 값과 pie chart을 업데이트.
        if(inferenced_images == 1 || inferenced_images % update_result_table_rate == 0){
            update_results_table(0, 1);
            update_results_table(1, 0);

            for(int i = 0; i < cla_crop_cnt.size(); i++){
                if(ui->table_model2->item(i, COUNT_COL)){
                    ui->table_model2->item(i, COUNT_COL)->setText(QString::number(cla_crop_cnt[i]));
                }
            }
        }

        if(media_mode == MEDIA_MODE_CAM || media_mode == MEDIA_MODE_VIDEO){
            if(inferenced_images == 1 || inferenced_images % 10 == 0){
                update_pie_chart(1, 0);
            }
        }
        else{
            update_pie_chart(1, 0);
        }

        // Image Name
        if(class_include_flag){
            show_class_str.replace(", ", "_");
            cur_img_name = show_class_str + "_" + cur_img_name;
        }
        if(!src_prefix.isEmpty()){
            cur_img_name = src_prefix + "_" + cur_img_name;
        }
    }
}

void MainWindow::segClaEnsembleSetResults(nrt::NDBuffer merged_pred_output, nrt::NDBuffer merged_prob_output, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name){
    if(crop_h > PRED_IMG.rows){
        crop_h = PRED_IMG.rows;
    }
    if(crop_w > PRED_IMG.cols){
        crop_w = PRED_IMG.cols;
    }

    // table2에서 class별 area ratio를 보여주기 위함
    long long total_pixel_cnt = PRED_IMG.rows * PRED_IMG.cols;
    vector<long long> seg_pixel_cnt(m_nrt_ensmble->get_model_class_num(), 0);
    vector<bool> exist_cla(m_nrt_ensmble->get_model_class_num(), false);

    int cur_img_total_blob_cnt = 0;
    float cur_img_blob_seg_mean_score = 0.0;
    float cur_img_blob_cla_mean_score = 0.0;

    cv::Mat intact_frame;
    PRED_IMG.copyTo(intact_frame);

    if (!merged_pred_output.empty() && !merged_prob_output.empty()) {
        nrt::Status status;

        nrt::NDBuffer prob_threshold_buf = nrt::NDBuffer::zeros(m_nrt->get_model_class_num(), nrt::DTYPE_FLOAT32);
        float* prob_threshold_ptr = prob_threshold_buf.get_at_ptr<float>();
        for(int i = 0; i < m_nrt->get_model_class_num(); i++){
            float value = m_nrt->prob_threshold[i];
            prob_threshold_ptr[i] = value;
        }

        // Treshold by blob prob
        status = nrt::pred_map_threshold_by_blob_prob(merged_pred_output, merged_prob_output, prob_threshold_buf);
        if(status != nrt::STATUS_SUCCESS){
            qDebug() << "pred_map_threshold_by_blob_prob failed. : " << QString(nrt::get_last_error_msg());
        }

        // Threshold by size
        nrt::NDBuffer bounding_rects;
        nrt::NDBuffer size_threshold_buf = nrt::NDBuffer::zeros(nrt::Shape(m_nrt->get_model_class_num(), 3), nrt::DTYPE_INT32);
        int* thres_ptr = size_threshold_buf.get_at_ptr<int>();
        for(int i = 0; i < m_nrt->get_model_class_num(); i++){
          int height = m_nrt->size_threshold[i].first;
          int width = m_nrt->size_threshold[i].second;
          int conjunction = m_nrt->size_thres_conjunction[i] == "AND" ? 0 : 1;
          thres_ptr[3*i + 0] = height;
          thres_ptr[3*i + 1] = width;
          thres_ptr[3*i + 2] = conjunction;
        }
        status = nrt::pred_map_threshold_by_size(merged_pred_output, bounding_rects, size_threshold_buf, m_nrt->get_model_class_num());
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "pred_map_threshold_by_size failed.  : " << QString(nrt::get_last_error_msg());
        }

        // Class Name
        unsigned char* output_ptr = merged_pred_output.get_at_ptr<unsigned char>(0);
        nrt::Shape bounding_rects_shape = bounding_rects.get_shape();
        for (int j = 0; j < bounding_rects_shape.dims[0]; j++) {
            int* output_rect_ptr = bounding_rects.get_at_ptr<int>(j);
            int image_batch_index = output_rect_ptr[0];
            int rect_x = output_rect_ptr[1];
            int rect_y = output_rect_ptr[2];
            int rect_h = output_rect_ptr[3];
            int rect_w = output_rect_ptr[4];
            int rect_class_index = output_rect_ptr[5];

            if (rect_class_index < 1 || rect_class_index >= m_nrt->get_model_class_num())
                continue;

            if(!model1_class_trigger_map[rect_class_index]){
                continue;
            }

            // 해당 blob의 최종 class는 model2(cla model)을 통해 결정.
            int cla_class_idx = -1;

            int x_start = (rect_x + rect_w / 2) - (crop_w / 2);
            int y_start = (rect_y + rect_h / 2) - (crop_h / 2);
            if(x_start < 0){
                x_start = 0;
            }
            if(x_start + crop_w > PRED_IMG.cols){
                x_start = PRED_IMG.cols - crop_w;
            }
            if(y_start < 0){
                y_start = 0;
            }
            if(y_start + crop_h > PRED_IMG.rows){
                y_start = PRED_IMG.rows - crop_h;
            }

            cv::Rect bounds(x_start, y_start, crop_w, crop_h);
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

            nrt::Shape output_pred_shape = cla_output_pred.get_shape();
            for (int i = 0; i < output_pred_shape.dims[0]; i++) {
                cla_class_idx = *cla_output_pred.get_at_ptr<int>(i);
            }

            // Threshold by prob
            QString cla_classname = m_nrt_ensmble->get_model_class_name(cla_class_idx);
            float cla_score = *cla_output_prob.get_at_ptr<float>(0, cla_class_idx) * 100;

            if(cla_class_idx < m_nrt_ensmble->prob_threshold.length()){
                float cur_cla_prob_thres;
                cur_cla_prob_thres = m_nrt_ensmble->prob_threshold[cla_class_idx];

                if (cla_score< cur_cla_prob_thres){
                    cla_classname = "Unknown";
                }
            }

//            qDebug() << "Predicted class: " << cla_classname << ", Detection prob: " << QString::number(cur_box_pred_class_prob) << ", Classification Prob: " << QString::number(pred_cls_prob);

            double alp_src = 0.5, alp_mask = 0.5;
            QColor cur_class_color = COLOR_VECTOR[cla_class_idx];
            int r, g, b;
            cur_class_color.getRgb(&r, &g, &b);
            cv::Scalar cur_class_color_scalar = cv::Scalar(b, g, r);

            int blob_pixel_cnt = 0;
            float blob_seg_score = 0;
            int cur_ofs = 0;
            int img_w = PRED_IMG.cols;
            for(int h = rect_y; h < rect_y + rect_h; h++){
                cur_ofs = h * img_w;
                for(int w = rect_x; w < rect_x + rect_w; w++){
                    int seg_class = output_ptr[cur_ofs + w];
                    if(seg_class != rect_class_index)
                        continue;

                    // prob map size [batch index, height, width, num of classes]
                    float pixel_prob = *merged_prob_output.get_at_ptr<float>(image_batch_index, h, w, rect_class_index);
                    blob_seg_score += pixel_prob * 100;
                    blob_pixel_cnt++;

                    seg_pixel_cnt[cla_class_idx] ++;

                    cv::Vec3b pix = PRED_IMG.at<cv::Vec3b>(h, w);
                    pix[2] = (unsigned char)(((double)pix[2] * alp_src) + ((double)cur_class_color.red() * alp_mask));
                    pix[1] = (unsigned char)(((double)pix[1] * alp_src) + ((double)cur_class_color.green() * alp_mask));
                    pix[0] = (unsigned char)(((double)pix[0] * alp_src) + ((double)cur_class_color.blue() * alp_mask));
                    PRED_IMG.at<cv::Vec3b>(h, w) = pix;
                }
            }
            if(blob_pixel_cnt == 0){

                continue;
            }

            if(cla_classname != "Unknown"){
                cur_img_total_blob_cnt++;
                exist_cla[cla_class_idx] = true;
                cur_img_blob_cla_mean_score += cla_score;
                class_ratio_model2[cla_class_idx]++;
                avg_scores_model2[cla_class_idx] += cla_score;
            }
            else{
                continue;
            }

            class_ratio_model1[rect_class_index]++;
            blob_seg_score = blob_seg_score / blob_pixel_cnt;
            cur_img_blob_seg_mean_score += blob_seg_score;
            avg_scores_model1[rect_class_index] += blob_seg_score;

            if (!cla_classname.isEmpty()) {
//                cv::putText(PRED_IMG, cla_classname.toStdString() + "(" + QString::number(blob_mean_score, 'f', 2).toStdString() +"%)", cv::Point(rect_x, rect_y), FONT_HERSHEY_SIMPLEX, 0.7 , cur_class_color_scalar, 2);
                cv::putText(PRED_IMG, cla_classname.toStdString(), cv::Point(rect_x, rect_y), FONT_HERSHEY_SIMPLEX, 0.7, cur_class_color_scalar, 2);
            }
        }

        QString class_str = "";
        for(int i = 0; i < exist_cla.size(); i++){
            if(exist_cla[i]){
                if(!class_str.isEmpty()){
                    class_str += ", ";
                }
                class_str += m_nrt_ensmble->get_model_class_name(i);
            }
        }
        ui->edit_show_class->setText(class_str);

        // 해당 이미지에 대해 seg blob mean score와 cla mean score를 계산 -> edit_show_score에 반영
        if(cur_img_total_blob_cnt != 0){
            cur_img_blob_seg_mean_score /= cur_img_total_blob_cnt;
            cur_img_blob_cla_mean_score /= cur_img_total_blob_cnt;
            ui->edit_show_score->setText(QString::number(cur_img_blob_seg_mean_score, 'f', 1) + "% -> " + QString::number(cur_img_blob_cla_mean_score, 'f', 1) + "%");
        }
        else{
            ui->edit_show_score->clear();
        }

        // total blob count 업데이트
        bool ok;
        int current_total_blob = ui->edit_total_2->text().toInt(&ok);
        if(!ok)
            current_total_blob = 0;
        ui->edit_total_2->setText(QString::number(current_total_blob + cur_img_total_blob_cnt));

        // table2의 class 별 avg socre와 area ratio업데이트
        if(inferenced_images == 1 || inferenced_images % update_result_table_rate == 0){
            for(int i = 0; i < m_nrt_ensmble->get_model_class_num(); i++){
                float cla_area_ratio = ((float)seg_pixel_cnt[i] / (float)total_pixel_cnt) * 100;
                if(ui->table_model2->item(i, COUNT_COL)){
                    ui->table_model2->item(i, COUNT_COL)->setText(QString::number(cla_area_ratio, 'f', 2) + "%");
                }
                update_results_table(1, 0);
                update_results_table(0, 1);
            }
        }

        // class별 blob 수 chart 업데이트
        if(media_mode == MEDIA_MODE_CAM || media_mode == MEDIA_MODE_VIDEO){
            if(inferenced_images == 1 || inferenced_images % 10 == 0){
                update_pie_chart(1, 0);
            }
        }
        else{
            update_pie_chart(1, 0);
        }
    }
}

void MainWindow::claClaEnsembleSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name){

}

void MainWindow::claDetEnsembleSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name){

}

void MainWindow::claSegEnsembleSetResults(nrt::NDBufferList outputs, cv::Mat &PRED_IMG, vector<std::string> &new_row, QString &cur_img_name){

}

void MainWindow::showResult() {
    cv::Mat ORG_IMG, PRED_IMG;
    Mat_With_Name org_mwn, pred_mwn;
    QString cur_inf_img_path;

    vector<std::string> new_row;
    QString cur_img_name;

    bool img_mode_end = false;
    bool img_init_pie_chart = false;

    // 각 Media mode에 따라 이미지 추출
    if (media_mode == MEDIA_MODE_CAM) {
        if(!m_usbCam->isExist()){
            if(m_timer){
                m_timer->stop();
            }
            on_btn_disconnect_camera_clicked();
            return;
        }

        m_usbCam->setResult();
        if(m_usbCam->m_frame.empty()){
            if(m_timer){
                m_timer->stop();
            }
            on_btn_disconnect_camera_clicked();
            return;
        }

        ORG_IMG = m_usbCam->m_frame.clone();

        cur_img_name = QDateTime::currentDateTime().toString("yyyyMMddhhmmssz");
    }

    else if(media_mode == MEDIA_MODE_CAM_FOLDER){
        if(new_img_buff.isEmpty()){
            return;
        }

        QString new_img_path = new_img_buff.front();
        new_img_buff.pop_front();

        QFileInfo file_info(new_img_path);
        if(!file_info.exists()){
            return;
        }

        bool found = false;
        int patience = 30;
        while(!found || patience == 0){
            ORG_IMG = cv::imread(new_img_path.toLocal8Bit().constData(), cv::IMREAD_COLOR);
            if(!ORG_IMG.empty()){
                found = true;
            }
            patience--;
        }

        if(ORG_IMG.empty()){
            return;
        }

        cur_img_name = file_info.fileName().split('.', QString::SkipEmptyParts).at(0);

        QString size_str = QString::number(ORG_IMG.rows) + "*" + QString::number(ORG_IMG.cols);
        ui->lab_media_info->setText("Image: " + cur_img_name + "\n\nSize: " + size_str);
    }

    else if(media_mode == MEDIA_MODE_VIDEO){
        if(!m_videoInputCap.isOpened()){
            qDebug() << "Video capture is not opened.";
            return;
        }

        m_videoInputCap >> ORG_IMG;

        int cur_msec_pos = m_videoInputCap.get(cv::CAP_PROP_POS_MSEC);
        QString cur_video_time = QDateTime::fromMSecsSinceEpoch(cur_msec_pos).toUTC().toString("mm:ss");
        ui->lab_time->setText(current_video_name + "       "  + cur_video_time + "/" + current_video_length);

        if(ORG_IMG.empty()){
            qDebug() << "Video is over!";
            m_videoInputCap.release();

            if(m_timer){
                m_timer->stop();
            }

            if(ui->com_video_list->currentIndex() == ui->com_video_list->count()-1){
                QMessageBox::information(this, "Notification", "This is the last video.");
                on_btn_stop_clicked();
            }
            else{
                ui->com_video_list->setCurrentIndex(ui->com_video_list->currentIndex() + 1);
            }
            return;
        }

        cur_img_name = QDateTime::currentDateTime().toString("yyyyMMddhhmmssz");
    }

    else if (media_mode == MEDIA_MODE_IMAGE) {
      if (cur_inf_img_idx < 0 || cur_inf_img_idx >= inf_image_list.size()) {
            return;
        }

        cur_inf_img_path = inf_image_list[cur_inf_img_idx];
        cur_img_name = cur_inf_img_path.split('/').last();
        cur_img_name = cur_img_name.split('.', QString::SkipEmptyParts).at(0);

        ORG_IMG = cv::imread(cur_inf_img_path.toLocal8Bit().constData(), cv::IMREAD_COLOR);
        qDebug() << "Show Result) Imread";

        if (ORG_IMG.empty()) {
            // 해당 이미지는 inf image list에서 제외
            inf_image_list.removeAt(cur_inf_img_idx);
            return;
        }

        ui->lab_time->setText(QString::number(cur_inf_img_idx + 1) + " / " + QString::number(inf_image_list.length()));
        QString size_str = QString::number(ORG_IMG.rows) + "*" + QString::number(ORG_IMG.cols);
        ui->lab_media_info->setText("Image: " + cur_img_name + "\n\nSize: " + size_str);

        if(cur_inf_img_idx == inf_image_list.length() - 1){
            if(repeat_flag){
                if(result_show_flag){
                    cur_inf_img_idx = 0;
                    img_init_pie_chart = true;
                }
            }
            else{
                if(m_timer)
                    m_timer->stop();
                img_mode_end = true;
            }
        }

        else{
            if(!repeat_flag || (repeat_flag && result_show_flag)){
                cur_inf_img_idx ++;
            }
        }
    }

    else return;

    PRED_IMG = ORG_IMG.clone();

    nrt::NDBufferList outputs;
    nrt::NDBuffer merged_pred_output;
    nrt::NDBuffer merged_prob_output;

    std::chrono::duration<double, std::milli> inf_time;
    QString model_type;

    if(inference_flag && m_nrt.use_count() > 0 && is_ready_for_inf(m_nrt.get())){
        inferenced_images ++;

        model_type = m_nrt->get_model_type();

        if(model_type == "Segmentation"){
            // Non patch mode 이미지 리사이즈 처리
            if(model_type == "Segmentation" && !m_nrt->is_model_patch_mode()){
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

            QVector<nrt::NDBuffer> output_vec = seg_execute(img_buffer, inf_time, m_nrt.get());
            merged_pred_output = output_vec[0];
            merged_prob_output = output_vec[1];
            QString inf_time_str = QString::number(inf_time.count(), 'f', 2);
            ui->edit_show_inf->setText(inf_time_str);
            new_row.push_back(inf_time_str.toStdString());
        }
        else if(model_type == "Classification" || model_type == "Detection" || model_type == "OCR" || model_type == "Anomaly") {
            nrt::NDBuffer resized_img_buffer = get_img_buffer(ORG_IMG, m_nrt.get());

            auto start = std::chrono::high_resolution_clock::now();
            outputs = m_nrt->execute(resized_img_buffer);
            auto end = std::chrono::high_resolution_clock::now();

            inf_time = end - start;
            QString inf_time_str = QString::number(inf_time.count(), 'f', 2);
            ui->edit_show_inf->setText(inf_time_str);
            new_row.push_back(inf_time_str.toStdString());
        }

        if (inf_mode_status == INF_MODE_SINGLE) {
            if(model_type == "Classification") {
                claSetResults(outputs, PRED_IMG, new_row, cur_img_name);
            }
            else if (model_type == "Segmentation") {
                segSetResults(merged_pred_output, merged_prob_output, PRED_IMG, new_row, cur_img_name);
            }
            else if (model_type == "Detection") {
                detSetResults(outputs, PRED_IMG, new_row, cur_img_name);
            }
            else if (model_type == "OCR") {
                ocrSetResults(outputs, PRED_IMG, new_row, cur_img_name);
            }
            else if (model_type == "Anomaly") {
                anomSetResults(outputs, PRED_IMG, new_row, cur_img_name);
            }
        }
        else if(inf_mode_status == INF_MODE_ENSEMBLE && m_nrt_ensmble.use_count() > 0 && is_ready_for_inf(m_nrt_ensmble.get())){
            int idx = ui->com_ensemble_options->currentIndex();
            if(idx == 0){
                detClaEnsmbleSetResults(outputs, PRED_IMG, new_row, cur_img_name);
            }
            else if(idx == 1){
                segClaEnsembleSetResults(merged_pred_output, merged_prob_output, PRED_IMG, new_row, cur_img_name);
            }
            else if(idx == 2){
                claDetEnsembleSetResults(outputs, PRED_IMG, new_row, cur_img_name);
            }
            else if(idx == 3){
                claSegEnsembleSetResults(outputs, PRED_IMG, new_row, cur_img_name);
            }
            else if(idx == 4){
                claClaEnsembleSetResults(outputs, PRED_IMG, new_row, cur_img_name);
            }
        }
    }

    cv::cvtColor(ORG_IMG, ORG_IMG, COLOR_RGB2BGR);
    cv::cvtColor(PRED_IMG, PRED_IMG, COLOR_RGB2BGR);

    org_mwn.image = ORG_IMG.clone();
    pred_mwn.image = PRED_IMG.clone();

    if(save_flag){
        bool flag = true;

        if(media_mode != MEDIA_MODE_IMAGE){
            if(inferenced_images % frame_interval != 0){
                flag = false;
            }
        }

        if(flag){
            std::unique_lock<std::mutex> lock(m_save_info._mutex);

            org_mwn.name = (cur_img_name + IMG_FORMAT).toStdString();
            pred_mwn.name = (cur_img_name + "_pred" + IMG_FORMAT).toStdString();

            m_save_info.org_buffer.push(org_mwn);
            m_save_info.pred_buffer.push(pred_mwn);
            m_save_info.row_buffer.push(new_row);
        }
    }

    // Show Result Task
    QImage m_qimage;
    if (repeat_flag) {
        if (!result_show_flag) {
            m_qimage = QImage((const unsigned char*) (ORG_IMG.data), ORG_IMG.cols, ORG_IMG.rows, ORG_IMG.step, QImage::Format_RGB888);
            result_show_flag = true;
            qDebug() << "result show flag(org): " << result_show_flag;
        }
        else {
            m_qimage = QImage((const unsigned char*) (PRED_IMG.data), PRED_IMG.cols, PRED_IMG.rows, PRED_IMG.step, QImage::Format_RGB888);
            result_show_flag = false;
            qDebug() << "result show flag(pred): " << result_show_flag;
        }
    }
    else {
        if (!show_pred_flag)
            m_qimage = QImage((const unsigned char*) (ORG_IMG.data), ORG_IMG.cols, ORG_IMG.rows, ORG_IMG.step, QImage::Format_RGB888);
        else{
            m_qimage = QImage((const unsigned char*) (PRED_IMG.data), PRED_IMG.cols, PRED_IMG.rows, PRED_IMG.step, QImage::Format_RGB888);
        }
    }

    QPixmap m_qpixmap = QPixmap::fromImage(m_qimage);
    ui->lab_show_res->setPixmap(m_qpixmap.scaled(ui->lab_show_res->width(), ui->lab_show_res->height(), Qt::KeepAspectRatio));
    lab_full_screen->setPixmap(m_qpixmap.scaled(lab_full_screen->width(), lab_full_screen->height(), Qt::KeepAspectRatio));

    if(img_mode_end){
        inference_flag = false;
        if(save_flag){
            while(save_worker_busy){
                qDebug() << save_worker_busy;
            }
            qDebug() << save_worker_busy;

            bool has_any_images = saveEvaluationJson();

            if(has_any_images){
                int evaluationSetId = -1;
                {
                    std::unique_lock<std::mutex> lock(m_save_info._mutex);
                    evaluationSetId = m_save_info.evaluationSetId;
                }

                QMessageBox::information(this,
                                         "Notification",
                                         "Evaluation done.\nThe results have been saved.",
                                         QMessageBox::Ok);

                show_result_image_table(evaluationSetId);

                ui->btn_play->hide();
                ui->btn_pause->hide();
                ui->btn_stop->hide();
            }
            else{
                on_btn_run_clicked();
            }

            save_flag = false;
        }
        else{
            on_btn_run_clicked();
        }
    }

    if(img_init_pie_chart){
        set_model_chart(0);
        inferenced_images = 0;
    }
}



/* run과 review 왔다 갔다 */
void MainWindow::on_btn_run_clicked(){
    if(inference_flag){
        return;
    }

    ui->btn_run->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 2px solid rgb(54, 93, 157); color: rgb(54, 93, 157);");
    ui->btn_review->setStyleSheet("border: none; color: #757575;");

    if(ui->Settings_Realtime_Review_Stack->currentWidget() == ui->run_settings_page){
        return;
    }

    initialize_settings_page();
    initialize_realtime_page();
    ui->Settings_Realtime_Review_Stack->setCurrentWidget(ui->run_settings_page);
}

void MainWindow::on_btn_review_clicked(){
    // Inference가 진행 중 -> 이동 불가
    if(inference_flag){
        return;
    }
    // 현재 이미 Evaluated Images를 조회 중 -> 현재 값들을 초기화하면 안되므로 return
    if(ui->media_info->currentWidget() == ui->review_page){
        return;
    }
    // 모델 생성 중 -> 이동 불가
    if(futureWatcher->isRunning() || futureWatcherEns->isRunning()){
        return;
    }

    ui->btn_review->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 2px solid rgb(54, 93, 157); color: rgb(54, 93, 157);");
    ui->btn_run->setStyleSheet("border: none; color: #757575;");

    initialize_review_page();

    // settings page -> realtime page
    ui->Settings_Realtime_Review_Stack->setCurrentWidget(ui->run_realtime_page);
}

void MainWindow::initialize_review_page(){
    // Evaluated Images Page
    ui->media_info->setCurrentWidget(ui->review_page);
    ui->media_info->setMaximumHeight(1000);

    ui->model_info_frame->hide();
    ui->vSpacer_first->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Minimum);
    ui->show_save_path_widget->hide();

    // Model and date information clear
    ui->lab_eval_model_names->clear();
    ui->lab_eval_start_time->clear();
    ui->lab_eval_end_time->clear();

    // result images table clear
    ui->result_images_table->clearContents();
    ui->result_images_table->setRowCount(0);

    // table model1 & table model2 clear
    ui->table_model1->clearContents();
    ui->table_model1->setRowCount(0);
    ui->table_model2->clearContents();
    ui->table_model2->setRowCount(0);
    ui->edit_total_1->setText("0");
    ui->edit_total_2->setText("0");

    // pie chart clear
    chart_model1->removeAllSeries();
    chart_model2->removeAllSeries();

    // total 값 초기화
    ui->edit_total_1->setText("0");
    ui->edit_total_2->setText("0");

    // 숨겨져있었던 model2관련 값들 보이게 하기
    ui->lab_total_2->show();
    ui->edit_total_2->show();
    ui->table_model2->show();
    ui->ratio_chart_model2->show();
    ui->horizontal_box_stats->setStyleSheet("QFrame#horizontal_box_stats{border-top: 0px; border-left:0px; border-right: 0px; border-bottom: 1px solid #757575;}");
}

void MainWindow::initialize_settings_page(){
    ui->lab_show_res->setText("Image");
    lab_full_screen->setText("Image");

    ui->edit_show_class->clear();
    ui->edit_show_score->clear();
    ui->edit_show_inf->clear();

    ui->btn_play->hide();
    ui->btn_pause->hide();
    ui->btn_stop->hide();
    ui->btn_disconnect_camera->hide();
    ui->btn_video_stop_inf->hide();

    ui->lab_time->hide();

    on_btn_single_clicked();
    on_btn_cam_mode_clicked();
    ui->com_cam_input_select->setCurrentIndex(0);

    // images table 초기화
    ui->table_images->clearContents();
    ui->table_images->setRowCount(0);
    inf_image_list.clear();

    // video file table 초기화, video cap 초기화
    ui->table_video_files->clearContents();
    ui->table_video_files->setRowCount(0);
    ui->com_video_list->clear();
    video_list.clear();

    // camera 객체 초기화
    if(m_usbCam.use_count() > 0){
        m_usbCam.reset();
    }

    // camera dirs table 초기화
    ui->list_watch_dirs->clear();
    file_sys_watcher->removePaths(file_sys_watcher->directories());

    media_mode = MEDIA_MODE_NONE;
}

void MainWindow::initialize_realtime_page(){
    // review -> realtime
    if(ui->media_info->currentWidget() == ui->review_page){
        ui->model_info_frame->show();
        ui->vSpacer_first->changeSize(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding);
        ui->show_save_path_widget->show();

        ui->media_info->setMaximumHeight(130);

        ui->media_info->setCurrentWidget(ui->media_info_cam);

        disconnect(btn_show_pred_connection);
    }

    // table model1과 table model2 초기화
    ui->table_model1->clearContents();
    ui->table_model1->setRowCount(0);
    ui->table_model1->horizontalHeader()->hide();

    ui->table_model2->clearContents();
    ui->table_model2->setRowCount(0);
    ui->table_model2->horizontalHeader()->hide();

    // total 값 초기화
    ui->edit_total_1->setText("0");
    ui->edit_total_2->setText("0");

    // 숨겨져있었던 model2관련 값들 보이게 하기
    ui->lab_total_2->show();
    ui->edit_total_2->show();
    ui->table_model2->show();
    ui->ratio_chart_model2->show();
    ui->horizontal_box_stats->setStyleSheet("QFrame#horizontal_box_stats{border-top: 0px; border-left:0px; border-right: 0px; border-bottom: 1px solid #757575;}");

    // pie chart 초기화
    if(!chart_model1->series().isEmpty()){
        chart_model1->removeAllSeries();
    }
    if(!chart_model2->series().isEmpty()){
        chart_model2->removeAllSeries();
    }
}



/* 초기 화면에서의 메뉴 버튼들 */
void MainWindow::on_btn_cam_mode_clicked(){
    ui->btn_cam_mode->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 2px solid #444444;");
    ui->btn_img_mode->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 1px solid #757575;");
    ui->btn_video_mode->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 1px solid #757575;");

    ui->Input_Source_Stack->setCurrentWidget(ui->cam_page);

    // Video mode 였을 경우 정지
    if(m_videoInputCap.isOpened()){
        if(m_timer.use_count() > 0){
            m_timer.reset();
        }
        m_videoInputCap.release();
    }

    // Img mode 였을 경우, 또는 프레임이 show res에 보여지고 있었을 경우 -> clear
    ui->lab_show_res->setText("Image");
    lab_full_screen->setText("Image");

    // Camera mode
    if(ui->com_cam_input_select->currentIndex() == 0){
        if(m_usbCam.use_count() > 0) {
            if(m_timer.use_count() > 0){
                m_timer->setInterval(0);
                m_timer->start();
            }
            else{
                m_timer = make_shared<QTimer>(this);
                connect(m_timer.get(), SIGNAL(timeout()), this, SLOT(showResult()));
                m_timer->setInterval(0);
                m_timer->start();
            }
            media_mode = MEDIA_MODE_CAM;
        }
        else{
            media_mode = MEDIA_MODE_NONE;
        }
    }

    // Camera Folder 추적 모드
    else{
        if(!file_sys_watcher->directories().isEmpty()){
            media_mode = MEDIA_MODE_CAM_FOLDER;
        }
        else{
            media_mode = MEDIA_MODE_NONE;
        }
    }
    ready_for_inference_check();
}

void MainWindow::on_btn_video_mode_clicked(){
    ui->btn_video_mode->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 2px solid #444444;");
    ui->btn_img_mode->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 1px solid #757575;");
    ui->btn_cam_mode->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 1px solid #757575;");

    ui->Input_Source_Stack->setCurrentWidget(ui->video_page);

    // Cam mode 였을 경우 정지
    if(m_usbCam.use_count() > 0){
        if(m_timer.use_count() > 0){
            m_timer.reset();
        }
    }

    // Img mode 였을 경우, 또는 프레임이 show res에 보여지고 있었을 경우 -> clear
    ui->lab_show_res->setText("Image");
    lab_full_screen->setText("Image");

    if(!video_list.isEmpty()){
        media_mode = MEDIA_MODE_VIDEO;
    }
    else{
        media_mode = MEDIA_MODE_NONE;
    }

    ready_for_inference_check();
}

void MainWindow::on_btn_img_mode_clicked(){
    ui->btn_img_mode->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 2px solid #444444;");
    ui->btn_cam_mode->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 1px solid #757575;");
    ui->btn_video_mode->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 1px solid #757575;");

    ui->Input_Source_Stack->setCurrentWidget(ui->image_page);

    // Cam mode 였을 경우, 정지
    if(m_usbCam.use_count() > 0){
        if(m_timer.use_count() > 0){
            m_timer.reset();
        }
        ui->lab_show_res->setText("Image");
    }

    // Video mode 였을 경우, 정지
    if(m_videoInputCap.isOpened()){
        if(m_timer.use_count() > 0){
            m_timer.reset();
        }
        m_videoInputCap.release();
        ui->lab_show_res->setText("Image");
    }

    if(!inf_image_list.isEmpty()){
        media_mode = MEDIA_MODE_IMAGE;
    }
    else{
        media_mode = MEDIA_MODE_NONE;
    }

    ready_for_inference_check();
}

void MainWindow::on_btn_single_clicked(){
    if(futureWatcher->isRunning()){
        QMessageBox *err_msg = new QMessageBox;
        err_msg->critical(0,"Notificaiton", "Other models are being created.");
        err_msg->setFixedSize(600, 400);
        return;
    }
    ui->btn_single->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 2px solid #444444; color: #444444");
    ui->btn_ensmble->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 1px solid #757575; color: #757575");

    ui->Model_Select_Stack->setCurrentWidget(ui->single_mode_page);
}

void MainWindow::on_btn_ensmble_clicked(){
    if(futureWatcher->isRunning()){
        QMessageBox *err_msg = new QMessageBox;
        err_msg->critical(0,"Notificaiton", "Other models are being created.");
        err_msg->setFixedSize(600, 400);
        return;
    }

    ui->btn_ensmble->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 2px solid #444444; color: #444444");
    ui->btn_single->setStyleSheet("border-top:0px; border-left:0px; border-right:0px; border-bottom: 1px solid #757575; color: #757575");

    ui->Model_Select_Stack->setCurrentWidget(ui->ensmble_mode_page);
}



/* CAM Mode & CAM Folder Mode */
void MainWindow::on_com_cam_input_select_currentTextChanged(const QString &text){
    if(text == CAM_TEXT){
        ui->btn_cam_select->setText("Select Camera");
        ui->lab_cam_page->setText("Please Select Camera Input");
        ui->widget_watch_dirs->hide();

        if(m_usbCam.use_count() > 0){
            if(m_timer.use_count() > 0){
                m_timer->setInterval(0);
                m_timer->start();
            }
            else{
                m_timer = make_shared<QTimer>(this);
                connect(m_timer.get(), SIGNAL(timeout()), this, SLOT(showResult()));
                m_timer->setInterval(0);
                m_timer->start();
            }
            media_mode = MEDIA_MODE_CAM;
        }
        else{
            media_mode = MEDIA_MODE_NONE;
        }
    }
    else if(text == CAM_FOLDER_TEXT){
        if(m_timer.use_count() > 0){
            m_timer.reset();
        }
        ui->lab_show_res->setText("Image");
        lab_full_screen->setText("Image");

        ui->btn_cam_select->setText("Select Directory");
        ui->widget_watch_dirs->show();

        if(file_sys_watcher->directories().length() == 0){
            ui->lab_cam_page->setText("Select the directory the images from your camera is being saved.\nYou may also select and watch several directories.");
            media_mode = MEDIA_MODE_NONE;
        }
        else{
            ui->lab_cam_page->setText("These paths will be watched for images uploaded from a camera.");
            ui->list_watch_dirs->clear();
            ui->list_watch_dirs->addItems(file_sys_watcher->directories());
            ui->list_watch_dirs->sortItems(Qt::AscendingOrder);

            media_mode = MEDIA_MODE_CAM_FOLDER;
        }
    }

    ready_for_inference_check();
}

void MainWindow::on_btn_cam_select_clicked()
{
    if(m_timer.use_count() > 0){
        m_timer.reset();
    }
    ui->lab_show_res->setText("Image");
    lab_full_screen->setText("Image");

    // USB CAM
    if (ui->com_cam_input_select->currentText() == CAM_TEXT) {
        m_usbCam = std::make_shared<UsbCam>();
        m_usbCam->selectCam();
        if (!m_usbCam->isExist()) {
            return;
        }
        media_mode = MEDIA_MODE_CAM;
        ui->media_info->setCurrentWidget(ui->media_info_cam);
        ui->lab_media_info_topbar->setText("Camera Information");
        ui->lab_media_info->setText("");

        // Start showing the camera buffer and disable other media options
        showResult();
        if(m_timer.use_count() <= 0){
            m_timer = std::make_shared<QTimer>(this);
        }
        connect(m_timer.get(), SIGNAL(timeout()), this, SLOT(showResult()));
        m_timer->setInterval(0);
        m_timer->start();
    }

    // Camera Folder Track
    else if(ui->com_cam_input_select->currentText() == CAM_FOLDER_TEXT){
        QString path = QFileDialog::getExistingDirectory(
                    this,
                    "Select Watch Directory",
                    QDir::homePath());
        if(path.isNull()){
            return;
        }

        if(file_sys_watcher->directories().contains(path)){
            qDebug() << "This path is already selected";
            return;
        }
        file_sys_watcher->addPath(path);
        ui->list_watch_dirs->addItems(file_sys_watcher->directories());
        ui->list_watch_dirs->sortItems(Qt::AscendingOrder);

        media_mode = MEDIA_MODE_CAM_FOLDER;

        ui->media_info->setCurrentWidget(ui->media_info_cam);
        ui->lab_media_info_topbar->setText("Folder Information");
        ui->lab_media_info->setText("");
    }

    // Executor 생성 thread가 완료 되었음 + media 선택 완료 -> Start Inference 버튼 활성화
    ready_for_inference_check();
}

void MainWindow::on_btn_delete_dir_clicked(){
    if(ui->list_watch_dirs->count() == 0){
        qDebug() << "No dirs";
        return;
    }
    if(!ui->list_watch_dirs->selectionModel()->hasSelection()){
        qDebug() << "No selected dir";
        return;
    }

    int row = ui->list_watch_dirs->selectionModel()->currentIndex().row();
    QString path = ui->list_watch_dirs->item(row)->text();
    file_sys_watcher->removePath(path);
    ui->list_watch_dirs->clear();
    ui->list_watch_dirs->addItems(file_sys_watcher->directories());
    ui->list_watch_dirs->sortItems(Qt::AscendingOrder);

    if(file_sys_watcher->directories().isEmpty()){
        media_mode = MEDIA_MODE_NONE;
        ready_for_inference_check();
    }
}

void MainWindow::directory_updated(QString path){
    QDir dir(path);

    QStringList cur_entry_list = currentContentsMap[path];
    QStringList new_entry_list = dir.entryList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files, QDir::DirsFirst);

    // Added files
    QSet<QString> new_files = QSet<QString>::fromList(new_entry_list) - QSet<QString>::fromList(cur_entry_list);
    QStringList new_files_list = new_files.toList();

    // Update entry list
    currentContentsMap[path] = new_entry_list;

    for(QString new_file : new_files_list){
        new_file = dir.absoluteFilePath(new_file);

        QFileInfo file_info(new_file);
        if(!file_info.exists()){
            continue;
        }

        if(file_info.isFile()){
            QStringList img_filters;
            img_filters << "bmp" << "dib"
                        << "jpg" << "jpeg" << "jpe" << "jp2"
                        << "png" << "webp"
                        << "pbm" << "pgm" << "ppm" << "pxm" << "pnm"
                        << "tiff" << "tif"
                        << "sr" << "ras"
                        << "hdr" << "pic";

            QString ext = file_info.suffix();
            if(img_filters.contains(ext, Qt::CaseInsensitive)){
                new_img_buff.append(new_file);
            }
        }
        else if(file_info.isDir()){
            qDebug() << new_file;
            file_sys_watcher->addPath(new_file);
        }
    }
}

void MainWindow::on_btn_disconnect_camera_clicked(){
    if(media_mode == MEDIA_MODE_CAM){
        if (m_timer.use_count() > 0) {
            qDebug() << "CAM Mode) Stop";
            m_timer.reset();
        }

        if (m_usbCam.use_count() > 0) {
            qDebug() << "CAM Mode) Delete CAM";
            m_usbCam.reset();
        }
        ui->lab_show_res->setText("Image");
        lab_full_screen->setText("Image");
    }
    else if(media_mode == MEDIA_MODE_CAM_FOLDER){
        disconnect(file_sys_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(directory_updated(QString)));
        file_sys_watcher->removePaths(file_sys_watcher->directories());
        ui->list_watch_dirs->clear();
    }
    media_mode = MEDIA_MODE_NONE;

    if(inference_flag){
        inference_flag = false;

        if(save_flag){
            while(save_worker_busy){
                qDebug() << save_worker_busy;
            }
            qDebug() << save_worker_busy;

            bool has_any_images = saveEvaluationJson();

            QMessageBox::information(this,
                                     "Notification",
                                     "Evaluation done.\nThe results have been saved.",
                                     QMessageBox::Ok);

            if(has_any_images){
                int evaluationSetId = -1;
                {
                    std::unique_lock<std::mutex> lock(m_save_info._mutex);
                    evaluationSetId = m_save_info.evaluationSetId;
                }

                show_result_image_table(evaluationSetId);
                ui->btn_disconnect_camera->hide();
            }
            else{
                on_btn_run_clicked();
            }

            save_flag = false;
        }
        else{
            on_btn_run_clicked();
        }
    }

    ready_for_inference_check();
}



/* Video Mode */
void MainWindow::on_com_video_input_currentTextChanged(const QString& text){
    if(text == VIDEO_FILE_TEXT){
        ui->btn_video_input->setText("Select Video File");
    }
    else if(text == VIDEO_FOLDER_TEXT){
        ui->btn_video_input->setText("Select Video Folder");
    }
}

void MainWindow::on_btn_video_input_clicked(){
    if(m_videoInputCap.isOpened()){
        m_videoInputCap.release();

        ui->lab_show_res->setText("Image");
        lab_full_screen->setText("Image");
    }

    if(ui->com_video_input->currentText() == VIDEO_FILE_TEXT){
        QString video_filepath = QFileDialog::getOpenFileName(this,
                                                              tr("Select Input Video"),
                                                              QDir::homePath(), tr(" (*.avi *.mp4 *.MOV *.mpg *.mpeg)"));

        if(video_filepath.isEmpty()){
            return;
        }

        QString video_filename = video_filepath.split('/').last();

        QString extension = video_filename.split('.').last();
        if(extension.compare("avi", Qt::CaseInsensitive) == 0 || extension.compare("mp4", Qt::CaseInsensitive) == 0 || extension.compare("mov", Qt::CaseInsensitive) == 0){
            m_videoInputCap.open(video_filepath.toLocal8Bit().constData(), cv::CAP_MSMF);
        }
        else if(extension.compare("mpg", Qt::CaseInsensitive) == 0 || extension.compare("mpeg", Qt::CaseInsensitive) == 0){
            m_videoInputCap.open(video_filepath.toLocal8Bit().constData(), cv::CAP_FFMPEG);
        }

        if(!m_videoInputCap.isOpened()){
            QMessageBox::information(this,
                                     "Notification",
                                     "Sorry. The video file is corupted.",
                                     QMessageBox::Ok);
            m_videoInputCap.release();
            return;
        }

        insert_video_table(video_filepath);

        ui->table_video_files->selectRow(video_list.length() - 1);
        current_video_name = video_filename;
    }
    else if(ui->com_video_input->currentText() == VIDEO_FOLDER_TEXT){
        QString dir = QFileDialog::getExistingDirectory(this,
                                                        tr("Open Directory"),
                                                        "/home",
                                                        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks);
        if(dir.isEmpty()){
            return;
        }

        QDirIterator itr(dir,
                         {"*.mp4", "*.avi", "*.mov", "*.mpg", "*.mpeg", "*.mkv",
                          "*.MP4", "*.AVI", "*.MOV", "*.MPG", "*.MPEG", "*.MKV"},
                         QDir::AllEntries | QDir::NoDotAndDotDot,
                         QDirIterator::Subdirectories);

        // 모든 비디오 파일들을 불러온 후, video table에서 선택하여 재생할 비디오의 row index
        int row_to_play = video_list.length();

        QString video_path, video_name;
        while(itr.hasNext()){
            video_path = itr.next();
            video_name = video_path.split("/").last();

            insert_video_table(video_path);
        }

        if(row_to_play >= video_list.length()){
            return;
        }

        QString cur_video_path = video_list[row_to_play];
        current_video_name = cur_video_path.split('/').last();
        ui->table_video_files->selectRow(row_to_play);
        QString extension = current_video_name.split('.').last();
        if(extension.compare("avi", Qt::CaseInsensitive) == 0 || extension.compare("mp4", Qt::CaseInsensitive) == 0 || extension.compare("mov", Qt::CaseInsensitive) == 0){
            m_videoInputCap.open(cur_video_path.toLocal8Bit().constData(), cv::CAP_MSMF);
        }
        else if(extension.compare("mpg", Qt::CaseInsensitive) == 0 || extension.compare("mpeg", Qt::CaseInsensitive) == 0){
            m_videoInputCap.open(cur_video_path.toLocal8Bit().constData(), cv::CAP_FFMPEG);
        }

        if(!m_videoInputCap.isOpened()){
            QMessageBox::information(this,
                                     "Notification",
                                     "Sorry. The video file is corrupted.",
                                     QMessageBox::Ok);
            m_videoInputCap.release();
            return;
        }
    }

    media_mode = MEDIA_MODE_VIDEO;
    ui->media_info->setCurrentWidget(ui->media_info_video);

    // lab_time에 현재 비디오 파일 이름과 시간 표시
    int frame_num = m_videoInputCap.get(cv::CAP_PROP_FRAME_COUNT);
    int frame_rate = m_videoInputCap.get(cv::CAP_PROP_FPS);
    int video_length = frame_num/frame_rate;
    QString length = QDateTime::fromTime_t(video_length).toUTC().toString("mm:ss");
    ui->lab_time->setText(current_video_name + "    00:00/" + length);

    // Start showing the video and disable other media options
    showResult();
    if(m_timer.use_count() <= 0){
        m_timer = std::make_shared<QTimer>(this);
    }
    connect(m_timer.get(), SIGNAL(timeout()), this, SLOT(showResult()));
    m_timer->setInterval(0);
    m_timer->start();

    // Executor 생성 thread가 완료 되었음 + media 선택 완료 -> Start Inference 버튼 활성화
    ready_for_inference_check();
}

void MainWindow::insert_video_table(QString video_filepath){
    cv::VideoCapture cap;

    QString extension = video_filepath.split('.').last();
    if(extension.compare("avi", Qt::CaseInsensitive) == 0 || extension.compare("mp4", Qt::CaseInsensitive) == 0 || extension.compare("mov", Qt::CaseInsensitive) == 0){
        cap.open(video_filepath.toLocal8Bit().constData(), cv::CAP_MSMF);
    }
    else if(extension.compare("mpg", Qt::CaseInsensitive) == 0 || extension.compare("mpeg", Qt::CaseInsensitive) == 0){
        cap.open(video_filepath.toLocal8Bit().constData(), cv::CAP_FFMPEG);
    }
    else{
        return;
    }

    if(!cap.isOpened()){
        qDebug() << "insert_video_table) failed to open video capture.";
        return;
    }

    ui->table_video_files->insertRow(ui->table_video_files->rowCount());
    int row = ui->table_video_files->rowCount() - 1;

    QTableWidgetItem* no_item = new QTableWidgetItem;

    no_item->setText(QString::number(row + 1));
    no_item->setTextAlignment(Qt::AlignCenter);
    ui->table_video_files->setItem(row, 0, no_item);

    QTableWidgetItem* name_item = new QTableWidgetItem;
    name_item->setText(video_filepath.split('/').last());
    name_item->setTextAlignment(Qt::AlignCenter);
    ui->table_video_files->setItem(row, 1, name_item);

    QTableWidgetItem* length_item = new QTableWidgetItem;
    int frame_num = cap.get(cv::CAP_PROP_FRAME_COUNT);
    int frame_rate = cap.get(cv::CAP_PROP_FPS);
    int video_length = frame_num/frame_rate;
    length_item->setText(QDateTime::fromTime_t(video_length).toUTC().toString("hh:mm:ss"));
    length_item->setTextAlignment(Qt::AlignCenter);
    ui->table_video_files->setItem(row, 2, length_item);

    QTableWidgetItem* resolution_item = new QTableWidgetItem;
    int width = cap.get(cv::CAP_PROP_FRAME_WIDTH);
    int height = cap.get(cv::CAP_PROP_FRAME_HEIGHT);
    resolution_item->setText(QString::number(width) + "*" + QString::number(height));
    resolution_item->setTextAlignment(Qt::AlignCenter);
    ui->table_video_files->setItem(row, 3, resolution_item);

    QTableWidgetItem* fps_item = new QTableWidgetItem;
    fps_item->setText(QString::number(frame_rate));
    fps_item->setTextAlignment(Qt::AlignCenter);
    ui->table_video_files->setItem(row, 4, fps_item);

    video_list.append(video_filepath);
    ui->com_video_list->addItem(video_filepath.split('/').last());
}

void MainWindow::on_btn_delete_videos_clicked(){
    if(ui->table_video_files->selectionModel()->hasSelection()){
        on_btn_stop_clicked();
        QVector<int> rows;
        for(auto model_idx : ui->table_video_files->selectionModel()->selectedRows()){
            int row = model_idx.row();
            if(row >= video_list.length()) break;
            rows.append(row);
        }
        sort(rows.begin(), rows.end(), greater<int>());
        for(auto row : rows){
            video_list.removeAt(row);
            ui->table_video_files->removeRow(row);
        }
    }
    else return;

    if(video_list.isEmpty()){
        media_mode = MEDIA_MODE_NONE;
        ui->lab_show_res->setText("Image");
        lab_full_screen->setText("Image");
        ready_for_inference_check();
        return;
    }

    for(int i = 0; i < ui->table_video_files->rowCount(); i++){
        QTableWidgetItem* no_item = new QTableWidgetItem;
        no_item->setText(QString::number(i + 1));
        no_item->setTextAlignment(Qt::AlignCenter);

        ui->table_video_files->setItem(i, 0, no_item);
    }
}

void MainWindow::on_table_video_files_itemSelectionChanged(){
    if(ui->table_video_files->selectionModel()->hasSelection()){
        if(m_timer.use_count() > 0 ){
            m_timer.reset();
        }

        if(m_videoInputCap.isOpened()){
            m_videoInputCap.release();
        }

        int row = ui->table_video_files->selectionModel()->currentIndex().row();
        play_video(row);
    }
}

void MainWindow::on_com_video_list_currentTextChanged(const QString& text){
    if(text.isEmpty()){
        return;
    }

    if(m_videoInputCap.isOpened()){
        m_videoInputCap.release();
    }

    if(m_timer.use_count() > 0 ){
        m_timer.reset();
    }

    play_video(ui->com_video_list->currentIndex());
}

void MainWindow::play_video(int video_idx){
    if(video_idx >= video_list.length()){
        qDebug() << "play_video) video index out of range";
        return;
    }

    QString cur_video_path = video_list[video_idx];
    current_video_name = cur_video_path.split('/').last();

    QString extension = cur_video_path.split('.').last();
    if(extension.compare("avi", Qt::CaseInsensitive) == 0 || extension.compare("mp4", Qt::CaseInsensitive) == 0 || extension.compare("mov", Qt::CaseInsensitive) == 0){
        m_videoInputCap.open(cur_video_path.toLocal8Bit().constData(), cv::CAP_MSMF);
    }
    else if(extension.compare("mpg", Qt::CaseInsensitive) == 0|| extension.compare("mpeg", Qt::CaseInsensitive) == 0){
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

    // lab_time에 현재 비디오 파일 이름과 시간 표시
    int frame_num = m_videoInputCap.get(cv::CAP_PROP_FRAME_COUNT);
    int frame_rate = m_videoInputCap.get(cv::CAP_PROP_FPS);
    int video_length = frame_num/frame_rate;
    current_video_length = QDateTime::fromSecsSinceEpoch(video_length).toUTC().toString("mm:ss");

    ui->lab_time->setText(current_video_name + "       00:00/" + current_video_length);

    // Video information에 정보 추가
    ui->lab_video_info->setText("Video Length (mm:ss): " + current_video_length + "\n\n" + "FPS: " + QString::number(frame_rate));

    showResult();
    if(m_timer.use_count() == 0){
        m_timer = make_shared<QTimer>(this);
    }
    connect(m_timer.get(), SIGNAL(timeout()), this, SLOT(showResult()));
    m_timer->setInterval(0);
    m_timer->start();
}

void MainWindow::on_btn_video_stop_inf_clicked(){
    ui->table_video_files->clearContents();
    ui->com_video_list->clear();
    video_list.clear();

    if(m_timer.use_count() > 0){
        qDebug() << "Video Mode) Inference Stop";
    }

    if(m_usbCam.use_count() > 0){
        qDebug() << "Video Mode) Delete cam";
    }

    if(inference_flag){
        inference_flag = false;
        if(save_flag){
            while(save_worker_busy){
                qDebug() << save_worker_busy;
            }
            qDebug() << save_worker_busy;

            on_btn_stop_clicked();

            bool has_any_images = saveEvaluationJson();

            QMessageBox::information(this,
                                     "Notification",
                                     "Evaluation done.\nThe results have been saved.",
                                     QMessageBox::Ok);

            if(has_any_images){
                int evaluationSetId = -1;
                {
                    std::unique_lock<std::mutex> lock(m_save_info._mutex);
                    evaluationSetId = m_save_info.evaluationSetId;
                }

                show_result_image_table(evaluationSetId);

                ui->btn_play->hide();
                ui->btn_pause->hide();
                ui->btn_stop->hide();
                ui->btn_video_stop_inf->hide();
            }
            else{
                on_btn_run_clicked();
            }

            save_flag = false;
        }
        else{
            on_btn_run_clicked();
        }
    }
}



/* Image Mode */
void MainWindow::on_btn_new_images_clicked(){
    QMessageBox *err_msg = new QMessageBox;
    err_msg->setFixedSize(600, 400);

    QString inputPath = QFileDialog::getExistingDirectory(
                this,
                tr("Select Input Folder"),
                QDir::homePath());

    if (inputPath.isNull()){
        return;
    }

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
        return;
    }

    int row_to_show = inf_image_list.length();

    QString image_path, image_name;
    while(input_it.hasNext()){
        image_path = input_it.next();
        image_name = image_path.split('/').last();

        cv::Mat img = cv::imread(image_path.toLocal8Bit().constData(), cv::IMREAD_COLOR);
        if(img.empty()){
            continue;
        }
        QString size_str = QString::number(img.rows) + "*" + QString::number(img.cols);

        ui->table_images->insertRow(ui->table_images->rowCount());
        int row = ui->table_images->rowCount() - 1;

        QTableWidgetItem* no_item = new QTableWidgetItem;
        no_item->setText(QString::number(row + 1));
        no_item->setTextAlignment(Qt::AlignCenter);
        ui->table_images->setItem(row, 0, no_item);

        QTableWidgetItem* name_item = new QTableWidgetItem;
        name_item->setText(image_name);
        name_item->setTextAlignment(Qt::AlignCenter);
        ui->table_images->setItem(row, 1, name_item);

        QTableWidgetItem* size_item = new QTableWidgetItem;
        size_item->setText(size_str);
        size_item->setTextAlignment(Qt::AlignCenter);
        ui->table_images->setItem(row, 2, size_item);

        inf_image_list.append(image_path);
    }

    if(row_to_show < ui->table_images->rowCount()){
        ui->table_images->selectRow(row_to_show);
    }

    media_mode = MEDIA_MODE_IMAGE;
    ui->media_info->setCurrentWidget(ui->media_info_cam);
    ui->lab_media_info_topbar->setText("Image Information");

    ready_for_inference_check();
}

void MainWindow::on_table_images_itemSelectionChanged(){
    if(ui->table_images->selectionModel()->hasSelection()){
        int row = ui->table_images->selectionModel()->currentIndex().row();

        if(row >= inf_image_list.length()){
            return;
        }

        QString cur_image_path = inf_image_list[row];
//        qDebug() << cur_image_path;
        cv::Mat img = cv::imread(cur_image_path.toLocal8Bit().constData(), cv::IMREAD_COLOR);
        if(img.empty()){
            inf_image_list.removeAt(row);
            ui->table_images->removeRow(row);
            return;
        }
        cv::cvtColor(img, img, COLOR_RGB2BGR);
        QImage m_qimage((const unsigned char*)img.data, img.cols, img.rows, img.step, QImage::Format_RGB888);
        QPixmap m_qpixmap = QPixmap::fromImage(m_qimage);
        ui->lab_show_res->setPixmap(m_qpixmap.scaled(ui->lab_show_res->width(), ui->lab_show_res->height(), Qt::KeepAspectRatio));
        lab_full_screen->setPixmap(m_qpixmap.scaled(lab_full_screen->width(), lab_full_screen->height(), Qt::KeepAspectRatio));
    }
}

void MainWindow::on_btn_delete_images_clicked(){
    if(ui->table_images->selectionModel()->hasSelection()){
        QVector<int> rows;
        for(auto model_idx : ui->table_images->selectionModel()->selectedRows()){
            int row = model_idx.row();
            if(row >= inf_image_list.length()) break;
            rows.append(row);
        }
        sort(rows.begin(), rows.end(), greater<int>());
        for(auto row : rows){
            inf_image_list.removeAt(row);
            ui->table_images->removeRow(row);
        }
    }
    else return;

    if(inf_image_list.isEmpty()){
        media_mode = MEDIA_MODE_NONE;
        ready_for_inference_check();
        ui->lab_show_res->setText("Image");
        lab_full_screen->setText("Image");
        return;
    }

    for(int i = 0; i < ui->table_images->rowCount(); i++){
        QTableWidgetItem* no_item = new QTableWidgetItem;
        no_item->setText(QString::number(i + 1));
        no_item->setTextAlignment(Qt::AlignCenter);

        ui->table_images->setItem(i, 0, no_item);
    }
}



/* Video mode와 Image mode 제어 */
void MainWindow::on_btn_play_clicked(){
    if(media_mode == MEDIA_MODE_VIDEO){
        // Video has been paused -> Replay
        if(m_timer.use_count() > 0){
            m_timer->setInterval(0);
            m_timer->start();
        }

        // Video has been stopped -> Play new video
        else{
            if(inference_flag){
                on_com_video_list_currentTextChanged(ui->com_video_list->currentText());
            }
            else{
                if(ui->table_video_files->selectionModel()->hasSelection()){
                    on_table_video_files_itemSelectionChanged();
                }
                else{
                    ui->table_video_files->selectRow(0);
                }
            }
        }
    }

    else if(media_mode == MEDIA_MODE_IMAGE){
        double interval = repeat_flag ? show_time / 2 : show_time;
        qDebug() << "interval: " << interval;

        if(m_timer.use_count() > 0){
            m_timer->setInterval(interval * 1000);
            m_timer->start();
        }
        else{
            m_timer = make_shared<QTimer>(this);
            connect(m_timer.get(), SIGNAL(timeout()), this, SLOT(showResult()));
            m_timer->setInterval(interval * 1000);
            m_timer->start();
        }
    }
}

void MainWindow::on_btn_pause_clicked(){
    if (m_timer.use_count() <= 0)
        return;

    if (m_timer->isActive()) {
        qDebug() << "CAM Mode) Pause";
        m_timer->stop();
    }
}

void MainWindow::on_btn_stop_clicked(){
    if (m_timer.use_count() > 0) {
        m_timer.reset();
    }

    if(media_mode == MEDIA_MODE_VIDEO){
        if(m_videoInputCap.isOpened()){
            qDebug() << "VIDEO Mode) Delete Video Capture";
            m_videoInputCap.release();
        }

        ui->lab_show_res->setText("Image");
        lab_full_screen->setText("Image");
    }

    else if(media_mode == MEDIA_MODE_IMAGE){
        if(inference_flag){
            inference_flag = false;

            if(save_flag){
                while(save_worker_busy){
                    qDebug() << save_worker_busy;
                }
                qDebug() << save_worker_busy;

                saveEvaluationJson();

                int evaluationSetId = -1;
                {
                    std::unique_lock<std::mutex> lock(m_save_info._mutex);
                    evaluationSetId = m_save_info.evaluationSetId;
                }

                QMessageBox::information(this,
                                         "Notification",
                                         "Evaluation done.\nThe results have been saved.",
                                         QMessageBox::Ok);

                show_result_image_table(evaluationSetId);

                save_flag = false;
            }
            else{
                on_btn_run_clicked();
            }
        }
    }
    else return;
}



/* Model Select */
void MainWindow::on_btn_select_single_mode_clicked()
{
    if(futureWatcher->isRunning()){
        return;
    }
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

    QString modelPath;
    QString modelName;
    while(1){
        if(dialog.exec() == QDialog::Accepted){
            if(newModel->isChecked()){
                modelPath = QFileDialog::getOpenFileName(this, tr("Select Model"), QDir::homePath(), tr("default (*.net)"));
                if(modelPath.isEmpty()){
                    return;
                }
                modelName = modelPath.split('/').last();

                // QString to wchar_t*
                wchar_t mModelPath[512];
                modelPath.toWCharArray(mModelPath);
                mModelPath[modelPath.length()] = L'\0';
                nrt::Model model(mModelPath);

                QString search_space_level = (model.get_training_search_space_level() == -1) ? QString("Fast") : (QString("Lv") + QString::number(model.get_training_search_space_level()+1));
                QString inf_level = "Lv" + QString::number(model.get_training_inference_time_level() + 1);

                QString model_type = "";
                nrt::ModelType nrt_model_type = model.get_model_type();
                if(nrt_model_type == nrt::CLASSIFICATION){
                    model_type = "Classification";
                }
                else if(nrt_model_type == nrt::SEGMENTATION){
                    model_type = "Segmentation";
                }
                else if(nrt_model_type == nrt::DETECTION){
                    model_type = "Detection";
                }
                else if(nrt_model_type == nrt::OCR){
                    model_type = "OCR";
                }
                else if(nrt_model_type == nrt::ANOMALY){
                    model_type = "Anomaly";
                }
                else return;

                int modelId = m_db->InsertModel(modelPath, modelName, model_type, model.get_training_type(), search_space_level, inf_level);
                if(modelId == -1){
                    qDebug() << "Model did not get saved in db correctly.";
                    currentModelId = -1;
                    return;
                }
                currentModelId = modelId;
                ensembleModelId = -1;

                break;
            }
            else{
                if(view->selectionModel()->hasSelection()){
                    int rowID = view->selectionModel()->currentIndex().row();
                    int modelId = view->model()->index(rowID, 0).data().toInt();
                    modelPath = m_db->getModelPath(modelId);
                    modelName = m_db->getModelName(modelId);
                    currentModelId = modelId;
                    ensembleModelId = -1;
                    qDebug() << "Model ID: " << modelId << ", Model Path: " << modelPath;
                    break;
                }
            }
        }
        else return;
    }

    if(m_nrt.use_count() > 0){
        m_nrt.reset();
    }
    m_nrt = std::make_shared<NrtExe>();

    bool set_flag = false;
    if (!m_nrt->get_gpu_status()) {
        set_flag = select_device_dialog(m_nrt.get());
    }
    if(!set_flag) return;

    setInfMode(INF_MODE_SINGLE);

    wchar_t mModelPath[512];
    modelPath.toWCharArray(mModelPath);
    mModelPath[modelPath.length()] = L'\0';

    nrt::Model temp_model(mModelPath);
    ui->lab_single_model_name->setText(temp_model.get_model_name());
    nrt::ModelType modelType = temp_model.get_model_type();
    QString type = "";
    if (modelType == nrt::NONE)
        type = "None";
    else if (modelType == nrt::CLASSIFICATION)
        type = "Classification";
    else if (modelType == nrt::SEGMENTATION)
        type = "Segmentation";
    else if (modelType == nrt::DETECTION)
        type = "Detection";
    else if (modelType == nrt::ANOMALY)
        type = "Anomaly";
    else if (modelType == nrt::OCR){
        type = "OCR";
        QMessageBox* msg = new QMessageBox;
        msg->setFixedSize(600, 400);
        rotation_flag = false;
        if(msg->question(this, "OCR Rotation", "Predict rotation values?", QMessageBox::Yes|QMessageBox::No, QMessageBox::Yes) == QMessageBox::Yes){
            rotation_flag = true;
        }
    }

    ui->lab_single_model_type->setText(type);

    future = QtConcurrent::run(set_model_thread, m_nrt.get(), modelPath, false);

    connect(futureWatcher.get(), SIGNAL(started()), this, SLOT(set_model_started()));
    connect(futureWatcher.get(), SIGNAL(finished()), this, SLOT(setUiForSingle()));
    futureWatcher->setFuture(future);
}

void MainWindow::on_com_ensemble_options_currentIndexChanged(int index){
    //Detection(1 class) -> Classification(N class)
    if(index == 0){
        model1_filter = "Type='Detection'";
        model2_filter = "Type='Classification'";
    }
    //Segmentation(1 class) -> Classification(N class)
    else if(index == 1){
        model1_filter = "Type='Segmentation'";
        model2_filter = "Type='Classification'";
    }
    //Binary Classification -> Detection
    else if(index == 2){
        model1_filter = "Type='Classification'";
        model2_filter = "Type='Detection'";
    }
    //Binary Classification -> Segmentation
    else if(index == 3){
        model1_filter = "Type='Classification'";
        model2_filter = "Type='Segmentation'";
    }
    //Binary Classification -> Classification
    else if(index == 4){
        model1_filter = "Type='Classification'";
        model2_filter = "Type='Classification'";
    }
}

void MainWindow::on_btn_select_ens_mode_clicked(){
    QMessageBox* err_msg = new QMessageBox;

    QFont roboto_10("Roboto", 10);
    QFont robotolight_10("Roboto Light", 10);

    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
//    dialog.setFixedSize(650, 400);
    dialog.setMinimumWidth(650);
    QVBoxLayout* root_cont = new QVBoxLayout;

    QHBoxLayout* model1_cont = new QHBoxLayout;

    QLabel* model1_lab = new QLabel;
    QString model1_type = model1_filter;
    model1_type.remove("Type='").remove("'");
    model1_lab->setText("Select 1st Model (" + model1_type + ")");
    model1_lab->setFont(roboto_10);
    model1_cont->addWidget(model1_lab);

    model1_cont->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    QPushButton* model1_btn = new QPushButton;
    model1_btn->setFlat(true);
    model1_btn->setIcon(QIcon(":/icons/folder.png"));
    model1_btn->setIconSize(QSize(20, 20));
    model1_cont->addWidget(model1_btn);
    root_cont->addLayout(model1_cont);

    QSqlDatabase db = QSqlDatabase::database("main_thread");
    QSqlTableModel* model1TableModel = new QSqlTableModel(this, db);
    model1TableModel->setTable("Models");
    model1TableModel->setFilter(model1_filter);
    model1TableModel->select();
    model1TableModel->setHeaderData(1, Qt::Horizontal, "Name");
    model1TableModel->setHeaderData(2, Qt::Horizontal, "Created On");
    model1TableModel->setHeaderData(4, Qt::Horizontal, "Type");
    model1TableModel->setHeaderData(5, Qt::Horizontal, "Platform");
    model1TableModel->setHeaderData(6, Qt::Horizontal, "Search Space");
    model1TableModel->setHeaderData(7, Qt::Horizontal, "Inference");

    QTableView* view = new QTableView;
    view->setModel(model1TableModel);
    view->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view->hideColumn(0);
    view->hideColumn(3);
    view->setSelectionBehavior(QAbstractItemView::SelectRows);
    view->setSelectionMode(QAbstractItemView::SingleSelection);

    view->verticalHeader()->hide();
    view->horizontalHeader()->show();

    view->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    view->setGridStyle(Qt::NoPen);

    verticalResizeTable(view);
    view->setFont(robotolight_10);
    root_cont->addWidget(view);

    root_cont->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));

    connect(model1_btn, &QPushButton::clicked, [&]{
        QString modelPath = QFileDialog::getOpenFileName(this, tr("Select Model"), QDir::homePath(), tr("default (*.net)"));
        if(modelPath.isEmpty()){
            return;
        }

        // QString to wchar_t*
        wchar_t mModelPath[512];
        modelPath.toWCharArray(mModelPath);
        mModelPath[modelPath.length()] = L'\0';
        nrt::Model model(mModelPath);

        QString model_type = "";
        nrt::ModelType nrt_model_type = model.get_model_type();
        if(nrt_model_type == nrt::CLASSIFICATION){
            model_type = "Classification";
        }
        else if(nrt_model_type == nrt::SEGMENTATION){
            model_type = "Segmentation";
        }
        else if(nrt_model_type == nrt::DETECTION){
            model_type = "Detection";
        }
        else if(nrt_model_type == nrt::OCR){
            model_type = "OCR";
        }
        else if(nrt_model_type == nrt::ANOMALY){
            model_type = "Anomaly";
        }
        else return;

        if(model_type != model1_type){
            err_msg->setFixedSize(600, 400);
            err_msg->critical(0, "Error", "This is not a " + model1_type + " model.");
        }
        else{
            QString modelName = modelPath.split('/').last();
            QString search_space_level = (model.get_training_search_space_level() == -1) ? QString("Fast") : (QString("Lv") + QString::number(model.get_training_search_space_level()+1));
            QString inf_level = "Lv" + QString::number(model.get_training_inference_time_level() + 1);

            int modelId = m_db->InsertModel(modelPath, modelName, model_type, model.get_training_type(), search_space_level, inf_level);
            if(modelId == -1){
                qDebug() << "Model did not get saved in db correctly.";
                currentModelId = -1;
                return;
            }

            model1TableModel->select();
            view->selectRow(view->verticalHeader()->count() - 1);
            view->scrollToBottom();
        }
    });

    QHBoxLayout* model2_cont = new QHBoxLayout;

    QLabel* model2_lab = new QLabel;
    QString model2_type = model2_filter;
    model2_type.remove("Type='").remove("'");
    model2_lab->setText("Select 2nd Model (" + model2_type + ")");
    model2_lab->setFont(roboto_10);
    model2_cont->addWidget(model2_lab);

    model2_cont->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

    QPushButton* model2_btn = new QPushButton;
    model2_btn->setFlat(true);
    model2_btn->setIcon(QIcon(":/icons/folder.png"));
    model2_btn->setIconSize(QSize(20, 20));
    model2_cont->addWidget(model2_btn);
    root_cont->addLayout(model2_cont);

    QSqlTableModel* model2TableModel = new QSqlTableModel(this, db);
    model2TableModel->setTable("Models");
    model2TableModel->setFilter(model2_filter);
    model2TableModel->select();
    model2TableModel->setHeaderData(1, Qt::Horizontal, "Name");
    model2TableModel->setHeaderData(2, Qt::Horizontal, "Created On");
    model2TableModel->setHeaderData(4, Qt::Horizontal, "Type");
    model2TableModel->setHeaderData(5, Qt::Horizontal, "Platform");
    model2TableModel->setHeaderData(6, Qt::Horizontal, "Search Space");
    model2TableModel->setHeaderData(7, Qt::Horizontal, "Inference");

    QTableView* view2 = new QTableView;
    view2->setModel(model2TableModel);
    view2->setEditTriggers(QAbstractItemView::NoEditTriggers);
    view2->hideColumn(0);
    view2->hideColumn(3);
    view2->setSelectionBehavior(QAbstractItemView::SelectRows);
    view2->setSelectionMode(QAbstractItemView::SingleSelection);

    view2->verticalHeader()->hide();
    view2->horizontalHeader()->show();

    view2->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    view2->horizontalHeader()->setSectionResizeMode(1, QHeaderView::Stretch);
    view2->setGridStyle(Qt::NoPen);

    verticalResizeTable(view2);
    view2->setFont(robotolight_10);
    root_cont->addWidget(view2);

    connect(model2_btn, &QPushButton::clicked, [&]{
        QString modelPath = QFileDialog::getOpenFileName(this, tr("Select Model"), QDir::homePath(), tr("default (*.net)"));
        if(modelPath.isEmpty()){
            return;
        }

        // QString to wchar_t*
        wchar_t mModelPath[512];
        modelPath.toWCharArray(mModelPath);
        mModelPath[modelPath.length()] = L'\0';
        nrt::Model model(mModelPath);

        QString model_type = "";
        nrt::ModelType nrt_model_type = model.get_model_type();
        if(nrt_model_type == nrt::CLASSIFICATION){
            model_type = "Classification";
        }
        else if(nrt_model_type == nrt::SEGMENTATION){
            model_type = "Segmentation";
        }
        else if(nrt_model_type == nrt::DETECTION){
            model_type = "Detection";
        }
        else if(nrt_model_type == nrt::OCR){
            model_type = "OCR";
        }
        else if(nrt_model_type == nrt::ANOMALY){
            model_type = "Anomaly";
        }
        else return;

        if(model_type != model2_type){
            err_msg->setFixedSize(600, 400);
            err_msg->critical(0, "Error", "This is not a " + model2_type + " model.");
        }
        else{
            QString modelName = modelPath.split('/').last();
            QString search_space_level = (model.get_training_search_space_level() == -1) ? QString("Fast") : (QString("Lv") + QString::number(model.get_training_search_space_level()+1));
            QString inf_level = "Lv" + QString::number(model.get_training_inference_time_level() + 1);

            int modelId = m_db->InsertModel(modelPath, modelName, model_type, model.get_training_type(), search_space_level, inf_level);
            if(modelId == -1){
                qDebug() << "Model did not get saved in db correctly.";
                currentModelId = -1;
                return;
            }

            model2TableModel->select();
            view2->selectRow(view2->verticalHeader()->count() - 1);
            view2->scrollToBottom();
        }
    });

    QDialogButtonBox*btnbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                   Qt::Horizontal, &dialog);
    connect(btnbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(btnbox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    root_cont->addWidget(btnbox);
    dialog.setLayout(root_cont);

    int model1Id = -1, model2Id = -1;
    while(1){
        if(dialog.exec() == QDialog::Accepted){
            if(view->selectionModel()->selectedRows().size() > 0 || view2->selectionModel()->selectedRows().size() > 0){
                int model1_rowId = view->selectionModel()->currentIndex().row();
                model1Id = view->model()->index(model1_rowId, 0).data().toInt();

                int model2_rowId = view2->selectionModel()->currentIndex().row();
                model2Id = view2->model()->index(model2_rowId, 0).data().toInt();
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
            return;
        }
    }

    if(model1Id == -1 || model2Id == -1){
        QMessageBox::information(this,
                                 "Notification",
                                 "Ensmble configuration failed due to wrong model ID values.",
                                 QMessageBox::Ok);
        return;
    }

    setInfMode(INF_MODE_ENSEMBLE);
    currentModelId = model1Id;
    ensembleModelId = model2Id;

    if(m_nrt.use_count() > 0){
        m_nrt.reset();
    }
    m_nrt = std::make_shared<NrtExe>();
    if (!m_nrt->get_gpu_status()) {
        select_device_dialog(m_nrt.get());
    }

    if(m_nrt_ensmble.use_count() > 0){
        m_nrt_ensmble.reset();
    }
    m_nrt_ensmble = std::make_shared<NrtExe>();
    if (!m_nrt_ensmble->get_gpu_status()) {
        select_device_dialog(m_nrt_ensmble.get());
    }

    // Disable option combobox -> enabled after both models are set
    ui->com_ensemble_options->setEnabled(false);

    QString model1Path = m_db->getModelPath(currentModelId);
    wchar_t mModel1Path[512];
    model1Path.toWCharArray(mModel1Path);
    mModel1Path[model1Path.length()] = L'\0';
    nrt::Model temp_model1(mModel1Path);
    ui->lab_model1_name->setText(temp_model1.get_model_name());
    ui->lab_model1_type->setText(model1_type);

    QString model2Path = m_db->getModelPath(ensembleModelId);
    wchar_t mModel2Path[512];
    model2Path.toWCharArray(mModel2Path);
    mModel2Path[model2Path.length()] = L'\0';
    nrt::Model temp_model2(mModel2Path);
    ui->lab_model2_name->setText(temp_model2.get_model_name());
    ui->lab_model2_type->setText(model2_type);

    ens_settings_dialog();

    future = QtConcurrent::run(set_model_thread, m_nrt.get(), model1Path, false);
    futureWatcher->setFuture(future);
    connect(futureWatcher.get(), SIGNAL(started()), this, SLOT(set_model_started()));
    connect(futureWatcher.get(), SIGNAL(finished()), this, SLOT(ensemble_model_start()));
}

void MainWindow::ens_settings_dialog(){
    QString modelPath = m_db->getModelPath(currentModelId);
    wchar_t mModelPath[512];
    modelPath.toWCharArray(mModelPath);
    mModelPath[modelPath.length()] = L'\0';
    nrt::Model model(mModelPath);

    QFont roboto_10("Roboto", 10);
    QFont robotolight_10("Roboto Light", 10);

    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    QVBoxLayout* root_cont = new QVBoxLayout;

    QLabel* trigger_class_lab = new QLabel;
    trigger_class_lab->setText("Trigger class");
    trigger_class_lab->setFont(roboto_10);
    root_cont->addWidget(trigger_class_lab);

    QLabel* explain_class_lab =  new QLabel;
    QString model1 = model1_filter;
    QString model2 = model2_filter;
    QString model1_type = model1.remove("Type='").remove("'");
    QString model2_type = model2.remove("Type='").remove("'");
    explain_class_lab->setText("Please select the classes from the " + model1_type + " model\nthat will trigger the execution of the " + model2_type + " model.");
    explain_class_lab->setFont(robotolight_10);
    root_cont->addWidget(explain_class_lab);

    QVBoxLayout* cbx_cont = new QVBoxLayout;
    cbx_cont->setSpacing(3);
    cbx_cont->setContentsMargins(0, 0, 0, 0);

    QVector<QCheckBox*> cbx_vector;
    int START_POINT;
    if(model.get_model_type() == nrt::SEGMENTATION || model.get_model_type() == nrt::DETECTION){
        START_POINT = 1;
    }
    else{
        START_POINT = 0;
    }
    for(int i = START_POINT; i < model.get_num_classes(); i++){
        QCheckBox* cbx = new QCheckBox;
        cbx->setText(model.get_class_name(i));
        cbx->setFont(robotolight_10);
        cbx_cont->addWidget(cbx);
        cbx_vector.append(cbx);
    }
    root_cont->addLayout(cbx_cont);

    QLineEdit* edit_crop_w = new QLineEdit;
    edit_crop_w->setText("128");
    edit_crop_w->setAlignment(Qt::AlignCenter);
    QLineEdit* edit_crop_h = new QLineEdit;
    edit_crop_h->setText("128");
    edit_crop_h->setAlignment(Qt::AlignCenter);

    QIntValidator* v = new QIntValidator(50, 1028);
    edit_crop_w->setValidator(v);
    edit_crop_h->setValidator(v);

    if(model.get_model_type() == nrt::SEGMENTATION || model.get_model_type() == nrt::DETECTION){
        root_cont->addSpacerItem(new QSpacerItem(0, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));

        QLabel* crop_lab = new QLabel;
        crop_lab->setText("Crop size (Width x Height)");
        crop_lab->setFont(roboto_10);
        root_cont->addWidget(crop_lab);

        QLabel* explain_crop_lab = new QLabel;
        explain_crop_lab->setText("The image will be cropped at the detected bounding boxes/regions.\nPlease select the crop size.");
        explain_crop_lab->setFont(robotolight_10);
        root_cont->addWidget(explain_crop_lab);

        QHBoxLayout* crop_size_cont = new QHBoxLayout;
        edit_crop_w->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        edit_crop_h->setSizePolicy(QSizePolicy(QSizePolicy::Fixed, QSizePolicy::Fixed));
        crop_size_cont->addWidget(edit_crop_w);
        crop_size_cont->addWidget(new QLabel("x"));
        crop_size_cont->addWidget(edit_crop_h);
        crop_size_cont->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
        root_cont->addLayout(crop_size_cont);
    }

    QDialogButtonBox*btnbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                   Qt::Horizontal, &dialog);
    connect(btnbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(btnbox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    root_cont->addWidget(btnbox);
    dialog.setLayout(root_cont);

    model1_class_trigger_map = QVector<bool>(model.get_num_classes(), false);
    while(1){
        if(dialog.exec() == QDialog::Accepted){
            int cnt = 0;
            for(int i = START_POINT; i < model.get_num_classes(); i++){
                bool flag = cbx_vector[i - START_POINT]->isChecked();

                model1_class_trigger_map[i] = flag;

                if(flag){
                    cnt++;
                    qDebug() << "Trigger class: " << model.get_class_name(i);
                }
            }

            bool ok, ok2;
            crop_h = edit_crop_h->text().toInt(&ok);
            crop_w = edit_crop_w->text().toInt(&ok2);

            if(cnt != 0 && ok && ok2){
                qDebug() << "Height: " << crop_h << ", Width: " << crop_w;
                break;
            }
        }
        else{
            break;
        }
    }
    return;
}

bool MainWindow::select_device_dialog(NrtExe* nrt_ptr)
{
    QFont robotolight_10("Roboto Light", 10);

    int idx = 0;
    QDialog *selectDlg = new QDialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    selectDlg->setWindowTitle("Device Allocation");
    selectDlg->setFixedSize(300, 200);
    selectDlg->setFont(robotolight_10);

    QVBoxLayout *dlgVLayout = new QVBoxLayout;

    QLabel *cHeader = new QLabel("Select Device for model: " + nrt_ptr->get_model_name());
    dlgVLayout->addWidget(cHeader);

    int gpuNum = nrt_ptr->get_gpu_num();
    QListWidget *deviceList = new QListWidget;
    for (int idx = 0; idx < gpuNum; idx++) {
        QListWidgetItem *deviceItem = new QListWidgetItem;
        deviceItem->setText(QString("GPU ").append(QString::number(idx)));
        deviceList->insertItem(idx, deviceItem);
    }
    QListWidgetItem* cpuItem = new QListWidgetItem;
    cpuItem->setText("CPU");
    deviceList->addItem(cpuItem);

    deviceList->setSelectionMode(QAbstractItemView::SingleSelection);
    deviceList->setCurrentRow(0);

    dlgVLayout->addWidget(deviceList);

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
        if(deviceList->currentItem()->text() == "CPU"){
            nrt_ptr->set_cpu();
        }
        else{
            idx = deviceList->currentRow();
            nrt_ptr->set_gpu(idx);
        }
        return true;
    }
    else{
        return false;
    }
}

void MainWindow::set_model_started(){
    model_ready_for_inf = false;
    ready_for_inference_check();

    // Clear class label and inference time
    ui->edit_show_inf->clear();
    ui->edit_show_class->clear();
    ui->edit_show_score->clear();

    // Loading gif
    disconnect(movie_connection);
    if(inf_mode_status == INF_MODE_SINGLE){
        movie_connection = connect(movie, &QMovie::frameChanged, [&]{
            ui->btn_select_single_mode->setIcon(movie->currentPixmap());
        });

        ui->lab_model1_name->clear();
        ui->lab_model1_type->clear();
        ui->lab_model2_name->clear();
        ui->lab_model2_name->clear();
    }
    else if(inf_mode_status == INF_MODE_ENSEMBLE){
        movie_connection = connect(movie, &QMovie::frameChanged, [&]{
            ui->btn_select_ens_mode->setIcon(movie->currentPixmap());
        });

        ui->lab_single_model_name->setText("Select a model");
        ui->lab_single_model_type->clear();
    }
    movie->start();
}

void MainWindow::ensemble_model_start(){
    disconnect(futureWatcher.get(), SIGNAL(started()), this, SLOT(set_model_started()));
    disconnect(futureWatcher.get(), SIGNAL(finished()), this, SLOT(ensemble_model_start()));

    // Make classification executor thread
    connect(futureWatcherEns.get(), SIGNAL(finished()), this, SLOT(setUiForEnsemble()));
    QString ens_modelPath = m_db->getModelPath(ensembleModelId);
    qDebug() << "Ensmble model path: " << ens_modelPath;
    futureEns = QtConcurrent::run(set_model_thread, m_nrt_ensmble.get(), ens_modelPath, false);
    futureWatcherEns->setFuture(futureEns);
}



/* Executor 생성 완료 후, 관련 UI 세팅 */
void MainWindow::set_model_table(int idx){
    bool actual_flag = false;
    bool count_flag = false;

    NrtExe* nrt_ptr = (idx == 0) ? m_nrt.get() : m_nrt_ensmble.get();
    QTableWidget* table = (idx==0) ? ui->table_model1 : ui->table_model2;
    QString model_type = nrt_ptr->get_model_type();

    QString count_col_label = (model_type == "Segmentation") ? "Area Ratio" : "Count";
    QStringList hor_header_label = {"Color",  "Class",  "Actual", count_col_label, "Avg Score"};

    if(model_type == "OCR"){
        table->clear();
        table->showColumn(ACTUAL_COL);
        table->showColumn(COUNT_COL);
        table->setRowCount(1);
        table->setColumnCount(5);
        table->setHorizontalHeaderLabels(hor_header_label);

        table->verticalHeader()->hide();
        table->horizontalHeader()->show();
        table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        table->horizontalHeader()->setSectionResizeMode(COLOR_COL, QHeaderView::Fixed);
        table->horizontalHeader()->setSectionResizeMode(ACTUAL_COL, QHeaderView::Fixed);
        table->horizontalHeader()->setSectionResizeMode(COUNT_COL, QHeaderView::Fixed);
        verticalResizeTable(table);

        COLOR_VECTOR.clear();
        QWidget* c_widget = new QWidget;
        QLabel* c_lab = new QLabel;
        QColor new_color = QColor(255, 0, 0);
        QVariant c_variant = new_color;
        c_lab->setStyleSheet("background-color:" + c_variant.toString() + ";");
        c_lab->setAlignment(Qt::AlignTop | Qt::AlignLeft);
        c_lab->setContentsMargins(0, 0, 0, 0);
        c_lab->setSizePolicy(QSizePolicy::MinimumExpanding, QSizePolicy::MinimumExpanding);
        QHBoxLayout* cLayout = new QHBoxLayout(c_widget);
        cLayout->addWidget(c_lab);
        cLayout->setAlignment(Qt::AlignCenter);
        cLayout->setContentsMargins(30, 2, 30, 2);
        c_widget->setLayout(cLayout);
        COLOR_VECTOR.append(new_color);
        table->setCellWidget(0, COLOR_COL, c_widget);

        QTableWidgetItem* class_item = new QTableWidgetItem;
        class_item->setText("0~9, A-Z, a-z");
        class_item->setTextAlignment(Qt::AlignCenter);
        table->setItem(0, CLASS_COL, class_item);

        QTableWidgetItem* actual_item = new QTableWidgetItem;
        actual_item->setText(QString::number(0));
        actual_item->setTextAlignment(Qt::AlignCenter);
        table->setItem(0, ACTUAL_COL, actual_item);

        QTableWidgetItem* count_item = new QTableWidgetItem;
        count_item->setText(QString::number(0));
        count_item->setTextAlignment(Qt::AlignCenter);
        table->setItem(0, COUNT_COL, count_item);
        table->item(0, COUNT_COL)->setFlags(table->item(0, COUNT_COL)->flags() & ~Qt::ItemIsEditable);

        QTableWidgetItem* score_item = new QTableWidgetItem;
        score_item->setText(QString::number(0) + "%");
        score_item->setTextAlignment(Qt::AlignCenter);
        table->setItem(0, AVG_SCORE_COL, score_item);
        table->item(0, AVG_SCORE_COL)->setFlags(table->item(0, AVG_SCORE_COL)->flags() & ~Qt::ItemIsEditable);
        return;
    }

    int START_POINT;
    if(model_type == "Classification"){
         START_POINT = 0;

         // 앙상블 모드 중에서 두번째 모델이 cla 인 경우 (seg->cla, det->cla) : count를 보여줌
         if(inf_mode_status == INF_MODE_ENSEMBLE && idx == 1){
             // det->cla
             if(ui->com_ensemble_options->currentIndex() == 0){
                 actual_flag = true;
                 count_flag = true;
             }

             // seg->cla
             else if(ui->com_ensemble_options->currentIndex() == 1){
                 actual_flag = false;
                 count_flag = true;
                 hor_header_label[3] = "Area Ratio";
             }
         }
         else{
             actual_flag = false;
             count_flag = false;
         }
    }
    else if(model_type == "Anomaly"){
        START_POINT = 0;
        actual_flag = false;
        count_flag = false;
    }
    else if(model_type == "Detection"){
        START_POINT = 1;

        if(inf_mode_status == INF_MODE_ENSEMBLE && idx == 0){
            actual_flag = false;
            count_flag = false;
        }
        else{
            actual_flag = true;
            count_flag = true;
        }
    }
    else if(model_type == "Segmentation"){
        START_POINT = 1;

        actual_flag = false;
        if(inf_mode_status == INF_MODE_ENSEMBLE && idx == 0){
            count_flag = false;
        }
        else{
            count_flag = true;
        }
    }
    else return;

    table->clear();
    table->showColumn(COLOR_COL);
    table->showColumn(ACTUAL_COL);
    table->showColumn(COUNT_COL);
    table->setRowCount(nrt_ptr->get_model_class_num() - START_POINT);
    table->setColumnCount(5);
    table->setHorizontalHeaderLabels(hor_header_label);

    table->verticalHeader()->hide();
    table->horizontalHeader()->show();
    table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
    table->horizontalHeader()->setSectionResizeMode(COLOR_COL, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(ACTUAL_COL, QHeaderView::Fixed);
    table->horizontalHeader()->setSectionResizeMode(COUNT_COL, QHeaderView::Fixed);
    verticalResizeTable(table);

    // 앙상블 모드의 첫번째 모델인 경우, color를 지정하지 않음.
    if(!(inf_mode_status == INF_MODE_ENSEMBLE && idx == 0)){
        COLOR_VECTOR.clear();
    }

    for(int i = START_POINT; i < nrt_ptr->get_model_class_num(); i++){
        // 첫번째 앙상블 모델의 클래스들 중, 다음 모델의 execution을 trigger 하지 않는 클래스는 테이블에서 제외.
        if(inf_mode_status == INF_MODE_ENSEMBLE && idx == 0){
            if(!model1_class_trigger_map[i]){
                table->setRowHeight(i - START_POINT, 0);
                verticalResizeTable(table);
            }
            table->hideColumn(COLOR_COL);
        }
        else{
            std::random_device rd;
            std::mt19937 gen(rd());
            std::uniform_int_distribution<int> dis(0, 255);

            QWidget* cWidget = new QWidget();
            QLabel* lab_color = new QLabel();
            QColor new_color;
            if(model_type == "Anomaly"){
                new_color = (nrt_ptr->get_model_class_name(i) == "Normal") ? QColor(0, 0, 255) : QColor(255, 0, 0);
            }
            else{
                new_color = QColor(dis(gen), dis(gen), dis(gen));
            }
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
            COLOR_VECTOR.append(new_color);

            table->setCellWidget(i - START_POINT, COLOR_COL, cWidget);
        }

        // Class Column
        QTableWidgetItem *newClassItem = new QTableWidgetItem(0);
        newClassItem->setText(nrt_ptr->get_model_class_name(i));
        newClassItem->setTextAlignment(Qt::AlignCenter);
        table->setItem(i - START_POINT, CLASS_COL, newClassItem);
        table->item(i - START_POINT, CLASS_COL)->setFlags(table->item(i - START_POINT, CLASS_COL)->flags() & ~Qt::ItemIsEditable);

        // Actual Column
        if(actual_flag){
            QTableWidgetItem *newActualItem = new QTableWidgetItem(0);
            newActualItem->setText(QString::number(0));
            newActualItem->setTextAlignment(Qt::AlignCenter);

            table->setItem(i - START_POINT, ACTUAL_COL, newActualItem);
        }
        else{
            table->hideColumn(ACTUAL_COL);
        }

        // Count Column
        if(count_flag){
            QTableWidgetItem *newItem = new QTableWidgetItem(0);
            newItem->setText(QString::number(0));
            newItem->setTextAlignment(Qt::AlignCenter);

            table->setItem(i - START_POINT, COUNT_COL, newItem);
            table->item(i - START_POINT, COUNT_COL)->setFlags(table->item(i - START_POINT, COUNT_COL)->flags() & ~Qt::ItemIsEditable);
        }
        else{
            table->hideColumn(COUNT_COL);
        }

        // Average Score Column
        QTableWidgetItem *newScoreItem = new QTableWidgetItem(0);
        QString score_txt = (model_type == "Anomaly") ? QString::number(0) : QString::number(0) + "%";
        newScoreItem->setText(score_txt);
        newScoreItem->setTextAlignment(Qt::AlignCenter);

        table->setItem(i - START_POINT, AVG_SCORE_COL, newScoreItem);
        table->item(i - START_POINT, AVG_SCORE_COL)->setFlags(table->item(i - START_POINT, AVG_SCORE_COL)->flags() & ~Qt::ItemIsEditable);
    }
}

void MainWindow::set_model_chart(int idx){
    NrtExe* nrt_ptr = (idx ==0) ? m_nrt.get() : m_nrt_ensmble.get();
    QString model_type = nrt_ptr->get_model_type();

    QPieSeries* series = new QPieSeries();
    QChart* chart = new QChart();
    QVector<int>& class_ratio = (idx == 0) ? class_ratio_model1 : class_ratio_model2;
    QVector<float>& avg_scores = (idx == 0) ? avg_scores_model1 : avg_scores_model2;
    QChartView* chart_view = (idx == 0) ? ui->ratio_chart_model1 : ui->ratio_chart_model2;

    int START_POINT = 0;
    if(model_type == "Segmentation" || model_type == "Detection"){
        START_POINT = 1;
    }

    // Chart View Setting
    if(model_type != "OCR"){
        for(int i = START_POINT; i < nrt_ptr->get_model_class_num(); i++){
            QString class_name = nrt_ptr->get_model_class_name(i);
            series->append(class_name, 1);
        }
        series->setLabelsVisible(true);
        series->setLabelsPosition(QPieSlice::LabelInsideHorizontal);

        class_ratio.clear();
        class_ratio = QVector<int>(nrt_ptr->get_model_class_num(), 0);

        for(int i = START_POINT; i < nrt_ptr->get_model_class_num(); i++){
            series->slices().at(i-START_POINT)->setColor(COLOR_VECTOR[i - START_POINT]);
            series->slices().at(i-START_POINT)->setLabelColor(QColor(255, 255, 255));
            series->slices().at(i-START_POINT)->setLabelFont(QFont("Roboto", 11));
        }

        chart->addSeries(series);
        chart->legend()->hide();
        chart->layout()->setContentsMargins(0, 0, 0, 0);
        chart->setBackgroundRoundness(0);
        chart->setPlotArea(QRectF(0, 0, 180, 180));

        chart_view->setChart(chart);
        chart_view->setRenderHint(QPainter::Antialiasing);

        avg_scores.clear();
        avg_scores = QVector<float>(nrt_ptr->get_model_class_num(), 0);

        if(idx == 0){
            series_model1 = series;
            chart_model1 = chart;
        }
        else{
            series_model2 = series;
            chart_model2 = chart;
        }
    }
    else{
        class_ratio.clear();
        class_ratio = QVector<int>(1, 0);

        avg_scores.clear();
        avg_scores = QVector<float>(1, 0);
    }
}

void MainWindow::setUiForEnsemble(){
    movie->stop();
    ui->btn_select_ens_mode->setIcon(QIcon(":/icons/upload.png"));

    disconnect(futureWatcherEns.get(), SIGNAL(started()), this, SLOT(set_model_started()));
    disconnect(futureWatcherEns.get(), SIGNAL(finished()), this, SLOT(setUiForEnsemble()));

    ui->table_model2->show();

    if(m_nrt.use_count() > 0 && is_ready_for_inf(m_nrt.get()) && m_nrt_ensmble.use_count() > 0 && is_ready_for_inf(m_nrt_ensmble.get())){
        // Model treshold 설정
        on_btn_model_settings_clicked();
    }
    else{
        qDebug() << "NRT) Set Model Failed!";
        currentModelId = -1;
        ensembleModelId = -1;
        return;
    }

    ui->com_ensemble_options->setEnabled(true);

    model_ready_for_inf = true;
    ready_for_inference_check();

    return;
}

void MainWindow::setUiForSingle(){
    movie->stop();
    ui->btn_select_single_mode->setIcon(QIcon(":/icons/upload.png"));

    disconnect(futureWatcher.get(), SIGNAL(started()), this, SLOT(set_model_started()));
    disconnect(futureWatcher.get(), SIGNAL(finished()), this, SLOT(setUiForSingle()));

    if(m_nrt.use_count() > 0 && is_ready_for_inf(m_nrt.get())){
        // Threshold 설정
        on_btn_model_settings_clicked();
    }
    else{
        qDebug() << "NRT) Set Model Failed!";
        currentModelId = -1;
        return;
    }

    model_ready_for_inf = true;

    ready_for_inference_check();

    return;
}



/* inference를 할 준비가 되었는 지 확인 (meida mode가 확정 되었는지 + executor 생성 thread가 정상 완료 되었는 지) */
void MainWindow::setInfMode(int infMode){
    inf_mode_status = infMode;
}

bool MainWindow::is_ready_for_inf(NrtExe* nrt_ptr){
    if(nrt_ptr->get_model_status() == nrt::Status::STATUS_SUCCESS && nrt_ptr->get_executor_status() == nrt::Status::STATUS_SUCCESS)
        return true;
    else
        return false;
}

void MainWindow::ready_for_inference_check(){
    qDebug() << "Media mode: " << media_mode;

    if(model_ready_for_inf && media_mode != MEDIA_MODE_NONE){
        ui->btn_start_inference->setEnabled(true);
    }
    else{
        ui->btn_start_inference->setEnabled(false);
    }

    if(media_mode == MEDIA_MODE_CAM || media_mode == MEDIA_MODE_CAM_FOLDER){
        ui->btn_play->hide();
        ui->btn_pause->hide();
        ui->btn_stop->hide();
        ui->btn_video_stop_inf->hide();
        ui->btn_disconnect_camera->show();
    }
    else if(media_mode == MEDIA_MODE_VIDEO){
        ui->btn_play->show();
        ui->btn_pause->show();
        ui->btn_stop->show();
        ui->btn_video_stop_inf->hide();
        ui->btn_disconnect_camera->hide();
    }
    else{
        ui->btn_play->hide();
        ui->btn_pause->hide();
        ui->btn_stop->hide();
        ui->btn_video_stop_inf->hide();
        ui->btn_disconnect_camera->hide();
    }
}



/* inference를 시작하기 전에 save관련 세팅/shuffle repeat 등의 설정을 하는 함수들 */
void MainWindow::save_settings_dialog(){
    QFont roboto_10("Roboto", 10);
    QFont robotolight_10("Roboto Light", 10);

    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    QVBoxLayout* root_cont = new QVBoxLayout;

    QWidget* container_showtime = new QWidget;
    QHBoxLayout* input_showtime =  new QHBoxLayout(container_showtime);
    QLabel* lab_show_time  = new QLabel("Show Time (sec)");
    lab_show_time->setFont(roboto_10);
    QLineEdit* show_time_edit = new QLineEdit;
    show_time_edit->setFont(robotolight_10);
    show_time_edit->setText("1");
    show_time_edit->setValidator(new QDoubleValidator(TERM_MIN, TERM_MAX, 1, this));

    if(media_mode == MEDIA_MODE_IMAGE || media_mode == MEDIA_MODE_CAM_FOLDER){
        input_showtime->addWidget(lab_show_time);
        input_showtime->addWidget(show_time_edit);
        root_cont->addWidget(container_showtime);
    }

    QWidget* container_count = new QWidget;
    QVBoxLayout* vbox_count =  new QVBoxLayout(container_count);
    QCheckBox* cbx_count = new QCheckBox("Actual / Count Compare");
    cbx_count->setFont(roboto_10);
    QLabel* lab_count = new QLabel("Assign the actual number of objects for each class and \ncompare with the amount detected by the Object Deteciton model.");
    lab_count->setFont(robotolight_10);

    bool flag = false;
    if(m_nrt->get_model_type() == "Detection" || m_nrt->get_model_type() == "OCR"){
        flag = true;
    }
    else if(m_nrt_ensmble.use_count() > 0 && (m_nrt_ensmble->get_model_type() == "Detection" || m_nrt_ensmble->get_model_type() == "OCR")){
        flag = true;
    }
    if(flag){
        vbox_count->addWidget(cbx_count);
        vbox_count->addWidget(lab_count);
        root_cont->addWidget(container_count);
    }

    QRadioButton* save_no = new QRadioButton;
    save_no->setText("Don't Save Results");
    save_no->setFont(roboto_10);
    root_cont->addWidget(save_no);

    QCheckBox* shuffle = new QCheckBox;
    shuffle->setText("Random Shuffle");
    shuffle->setFont(robotolight_10);

    QCheckBox* repeat = new QCheckBox;
    repeat->setText("Repeat");
    repeat->setFont(robotolight_10);

    QWidget* container_no = new QWidget;
    QVBoxLayout *vbox_no = new QVBoxLayout(container_no);
    vbox_no->addWidget(shuffle);
    vbox_no->addWidget(repeat);
    vbox_no->setContentsMargins(15, 3, 15, 3);

    root_cont->addWidget(container_no);
    root_cont->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));

    QRadioButton* save_yes = new QRadioButton;
    save_yes->setText("Save Results");
    save_yes->setFont(roboto_10);
    root_cont->addWidget(save_yes);

    QLabel* frame_interval_label = new QLabel("Frame Save Interval");
    frame_interval_label->setFont(roboto_10);

    QCheckBox* every_frame = new QCheckBox;
    every_frame->setText("Save all frames");
    every_frame->setFont(robotolight_10);

    QCheckBox* cbx_frame_interval = new QCheckBox;
    cbx_frame_interval->setText("Save every 'N'th frame");
    cbx_frame_interval->setFont(robotolight_10);

    if(media_mode == MEDIA_MODE_IMAGE || media_mode == MEDIA_MODE_CAM_FOLDER){
        every_frame->setChecked(true);
    }
    else{
        cbx_frame_interval->setChecked(true);
    }

    QLineEdit* edit_interval = new QLineEdit;
    edit_interval->setFont(robotolight_10);
    edit_interval->setText("30");

    QHBoxLayout* hbox_set_frame_int = new QHBoxLayout;
    hbox_set_frame_int->addWidget(cbx_frame_interval);
    hbox_set_frame_int->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    hbox_set_frame_int->addWidget(edit_interval);

    QLabel* lab_image_name_format = new QLabel("Image Name Format");
    lab_image_name_format->setFont(roboto_10);

    QLineEdit* edit_image_name_format = new QLineEdit;
    edit_image_name_format->setFont(robotolight_10);
    edit_image_name_format->setReadOnly(true);

    QCheckBox* cbx_include_source = new QCheckBox;
    QString src_text;
    if(media_mode == MEDIA_MODE_CAM || media_mode == MEDIA_MODE_CAM_FOLDER){
        src_text = "Cam";
    }
    else if(media_mode == MEDIA_MODE_VIDEO){
        src_text = "Video";
    }
    else if(media_mode == MEDIA_MODE_IMAGE){
        src_text = "Image";
    }
    else{
        return;
    }
    cbx_include_source->setText("Include Source Information");
    cbx_include_source->setFont(robotolight_10);
//    cbx_include_source->setChecked(true);

    QCheckBox* cbx_include_class = new QCheckBox;
    cbx_include_class->setText("Include Predicted Class");
    cbx_include_class->setFont(robotolight_10);
    cbx_include_class->setChecked(true);

    QString current_date = QDateTime::currentDateTime().toString("yyyyMMddhhmmssz");
    edit_image_name_format->setText("OK_" + current_date + ".png");

    QLabel* lab_save_path = new QLabel("Save Folder");
    lab_save_path->setFont(roboto_10);
    QPushButton* new_save_path_btn = new QPushButton;
    new_save_path_btn->setIcon(QIcon(":/icons/folder.png"));
    new_save_path_btn->setIconSize(QSize(16, 16));
    QHBoxLayout* save_path_hbox = new QHBoxLayout;
    save_path_hbox->addWidget(lab_save_path);
    save_path_hbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));
    save_path_hbox->addWidget(new_save_path_btn);

    QLineEdit* edit_save_path = new QLineEdit;
    edit_save_path->setText(current_save_path);
    edit_save_path->setFont(robotolight_10);
    edit_save_path->setReadOnly(true);

    QWidget* container_yes = new QWidget;
    QVBoxLayout* vbox_yes = new QVBoxLayout(container_yes);
    if(media_mode != MEDIA_MODE_IMAGE){
        vbox_yes->addWidget(frame_interval_label);
        vbox_yes->addWidget(every_frame);
        vbox_yes->addLayout(hbox_set_frame_int);
    }
    vbox_yes->addWidget(lab_image_name_format);
    vbox_yes->addWidget(cbx_include_source);
    vbox_yes->addWidget(cbx_include_class);
    vbox_yes->addWidget(edit_image_name_format);
    vbox_yes->addLayout(save_path_hbox);
    vbox_yes->addWidget(edit_save_path);

    vbox_yes->setContentsMargins(15, 3, 15, 3);
    vbox_yes->setSpacing(10);

    root_cont->addWidget(container_yes);

    connect(save_no,
            QOverload<bool>::of(&QRadioButton::toggled),
            [&](bool checked)
            {
                save_yes->setDown(!checked);
                container_yes->setEnabled(!checked);
            });

    connect(save_yes,
            QOverload<bool>::of(&QRadioButton::toggled),
            [&](bool checked)
            {
                save_no->setDown(!checked);
                container_no->setEnabled(!checked);
            });

    connect(every_frame,
            QOverload<bool>::of(&QCheckBox::toggled),
            [&](bool checked)
            {
                cbx_frame_interval->setChecked(!checked);
                edit_interval->setEnabled(!checked);
            });

    connect(cbx_frame_interval,
            QOverload<bool>::of(&QCheckBox::toggled),
            [&](bool checked)
            {
                every_frame->setChecked(!checked);
                edit_interval->setEnabled(checked);
            });

    connect(edit_interval,
            QOverload<const QString&>::of(&QLineEdit::textChanged),
            [&](const QString& text)
            {
                int value;
                bool ok;
                value = text.toInt(&ok);
                if(!ok){
                    edit_interval->setText("");
                }
                else{
                    edit_interval->setText(QString::number(value));
                }
            });

    connect(cbx_include_source,
            QOverload<bool>::of(&QCheckBox::toggled),
            [&](bool checked)
            {
                QString new_text = "";
                if(checked){
                    new_text += src_text + "_";
                }

                if(cbx_include_class->isChecked()){
                    new_text += "OK_";
                }

                new_text += current_date + ".png";

                edit_image_name_format->setText(new_text);
            });

    connect(cbx_include_class,
            QOverload<bool>::of(&QCheckBox::toggled),
            [&](bool checked)
            {
                QString new_text = "";

                if(cbx_include_source->isChecked()){
                    new_text += src_text + "_";
                }

                if(checked){
                    new_text += "OK_";
                }

                new_text += current_date + ".png";

                edit_image_name_format->setText(new_text);
            });

    connect(new_save_path_btn,
           &QPushButton::clicked,
           [&]()
            {
                QDir dir(current_save_path);
                bool ok = true;
                if(!dir.exists()){
                    ok = dir.mkpath(current_save_path);
                }

                QString new_save_path = "";
                if(ok){
                    new_save_path = QFileDialog::getExistingDirectory(
                                this,
                                tr("Select new save folder."),
                                current_save_path);
                    if(new_save_path.isNull()){
                        return;
                    }
                }
                else{
                    qDebug() << "make dir failed";
                    return;
                }

                edit_save_path->setText(new_save_path);
            });

    save_yes->setChecked(true); // setChecked triggers a signal. (setDown does not)

    QDialogButtonBox*btnbox = new QDialogButtonBox(QDialogButtonBox::Ok,
                                                   Qt::Horizontal, &dialog);
    connect(btnbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    root_cont->addWidget(btnbox);

    dialog.setLayout(root_cont);

    if(dialog.exec() == QDialog::Accepted){
        if(save_yes->isChecked()){
            save_flag = true;

            // CAM 또는 VIDEO 모드에서, frame을 저장하는 간격 설정.
            if(media_mode != MEDIA_MODE_IMAGE && cbx_frame_interval->isChecked()){
                bool ok;
                frame_interval = edit_interval->text().toInt(&ok);
                if(!ok){
                    frame_interval = 2;
                }
            }
            else{
                frame_interval = 1;
            }

            // Inference 후 이미지를 저장할 때, 이미지 명칭에 포함하고 싶은 정보 설정
            src_prefix = cbx_include_source->isChecked() ? src_text : "";
            class_include_flag = cbx_include_class->isChecked() ? true : false;

            // Save 경로
            current_save_path = edit_save_path->text();
        }
        else{
            save_flag = false;

            random_shuffle_flag = shuffle->isChecked() ? true : false;
            repeat_flag = repeat->isChecked() ? true : false;
        }

        bool ok;
        show_time = show_time_edit->text().toDouble(&ok);
        if(!ok){
            show_time = 0;
        }

        if(flag){
            detect_count_flag = cbx_count->isChecked();

            if(!detect_count_flag){
                // table상에서 actual column을 숨겨야함.
                ui->table_model1->hideColumn(ACTUAL_COL);
                ui->table_model2->hideColumn(ACTUAL_COL);
            }
        }
        else{
            detect_count_flag = false;
        }
    }
}

void MainWindow::initJson(int evaluationSetId){
    std::unique_lock<std::mutex> lock(m_save_info._mutex);

    QJsonObject& title = m_save_info.title;
    QFile& file = m_save_info.json_file;

    int START_POINT = 0;
    if(inf_mode_status == INF_MODE_SINGLE && m_nrt.use_count() > 0){
        if(m_nrt->get_model_type() == "Detection" || m_nrt->get_model_type() == "Segmentation"){
            START_POINT = 1;
        }
    }
    else if(inf_mode_status == INF_MODE_ENSEMBLE){

    }

    title["evaluation_set_id"] = evaluationSetId;
    title["start_datetime"] = m_db->getEvaluationSetStartedOn(evaluationSetId, "main_thread");
    title["model1_id"] = currentModelId;
    title["model1_type"] = m_nrt->get_model_type();
//    title["model2_id"] = ensembleModelId;
//    title["model2_type"] = m_nrt_ensmble->get_model_type();
    title["num_of_images"] = 0;
    title["total_inf_time"] = 0;
    title["data"] = QJsonArray();

    QJsonObject image_stats;
    QJsonObject label_stats;
    for(int i = START_POINT; i < m_nrt->get_model_class_num(); i++){
        image_stats[m_nrt->get_model_class_name(i)] = 0;
        label_stats[m_nrt->get_model_class_name(i)] = 0;
    }

    m_save_info.json_doc.setObject(title);

    QString path = m_db->getEvaluationJsonPath(evaluationSetId);
    file.setFileName(path);
}

void MainWindow::on_btn_start_inference_clicked(){
    if(inf_mode_status == INF_MODE_SINGLE){
        single_realtime_setting();
    }
    else if(inf_mode_status == INF_MODE_ENSEMBLE){
        ensemble_realtime_setting();
    }
    else return;

    if(media_mode == MEDIA_MODE_CAM){
        if(m_timer.use_count() > 0){
            m_timer->stop();
        }
    }
    else if(media_mode == MEDIA_MODE_VIDEO){
        ui->com_video_list->setCurrentIndex(0);
        on_btn_stop_clicked();
    }

    // current save path 원상 복구
    QString year_month = QDate::currentDate().toString("yyyy-MM");
    QString year_month_day = QDate::currentDate().toString("yyyy-MM-dd");
    current_save_path = root_path + "/" + year_month + "/" + year_month_day;
    save_settings_dialog();

    if(save_flag){
        QString StartedOn = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");

        int evaluationSetId = m_db->InsertEvaluationSet(StartedOn, currentModelId, ensembleModelId);
        if(evaluationSetId == -1){
            qDebug() << "btn_start_inference) Failed to insert evaluation set";
            return;
        }
        {
            std::unique_lock<std::mutex> lock(m_save_info._mutex);
            m_save_info.evaluationSetId = evaluationSetId;
        }
        initJson(evaluationSetId);
    }
    else{
        // shuffle -> 현재는 image mode 인 경우에만 유효하게 작동.
        if(random_shuffle_flag && media_mode == MEDIA_MODE_IMAGE){
            std::random_device rd;
            std::mt19937 g(rd()); // random bit generator
            std::shuffle(inf_image_list.begin(), inf_image_list.end(), g);
        }

        // repeat -> 현재는 image mode 인 경우에만 유효하게 작동.
        if(repeat_flag && media_mode == MEDIA_MODE_IMAGE){
            result_show_flag = false;
        }
    }

    qDebug() << "Media mode: " << media_mode;
    if(media_mode == MEDIA_MODE_IMAGE){
        update_result_table_rate = 1;
        cur_inf_img_idx = 0;

        ui->btn_play->show();
        ui->btn_pause->show();
        ui->btn_stop->show();
    }
    else if(media_mode == MEDIA_MODE_CAM_FOLDER){
        update_result_table_rate = 1;

        new_img_buff.clear();

        for(auto path : file_sys_watcher->directories()){
            const QDir dir(path);

            for(auto dir_path : dir.entryList(QDir::NoDotAndDotDot | QDir::AllDirs)){
                QString sub_dir = dir.absoluteFilePath(dir_path);
                qDebug() << sub_dir;

                QFileInfo file_info(sub_dir);
                if(file_info.exists()){
                    file_sys_watcher->addPath(sub_dir);
                }
            }

            currentContentsMap[path] = dir.entryList(QDir::NoDotAndDotDot | QDir::AllDirs | QDir::Files, QDir::DirsFirst);
        }
        connect(file_sys_watcher, SIGNAL(directoryChanged(QString)), this, SLOT(directory_updated(QString)));

        if(m_timer.use_count() > 0){
            m_timer.reset();
        }
        m_timer = make_shared<QTimer>(this);
        connect(m_timer.get(), SIGNAL(timeout()), this, SLOT(showResult()));
        m_timer->setInterval(show_time * 1000);
        m_timer->start();
    }
    else if(media_mode == MEDIA_MODE_CAM){
        update_result_table_rate = 15;
    }
    else if(media_mode == MEDIA_MODE_VIDEO){
        update_result_table_rate = 30;

        ui->lab_time->show();
        ui->btn_video_stop_inf->show();
    }
    inferenced_images = 0;

    // Show Result 함수에서 inference를 수행하기 시작함.
    inference_flag = true;

    qDebug() << "Save flag: " << save_flag;
    qDebug() << "Inference flag: " << inference_flag;
    qDebug() << "Update result table rate: " << update_result_table_rate;
    qDebug() << "Source prefix: " << src_prefix;
    qDebug() << "Class include flag: " << class_include_flag;
    qDebug() << "Current save path: " << current_save_path;
    qDebug() << "Shuffle: " << random_shuffle_flag;
    qDebug() << "Repeat: " << repeat_flag;
    qDebug() << "Show time: " << show_time;
    qDebug() << "Detect count flag: " << detect_count_flag;

    ui->Settings_Realtime_Review_Stack->setCurrentWidget(ui->run_realtime_page);

    if(media_mode == MEDIA_MODE_CAM){
        if(m_timer.use_count() > 0){
            m_timer->setInterval(0);
            m_timer->start();
        }
    }
}

void MainWindow::ensemble_realtime_setting(){
    // Run - Inference 중 페이지에서의 model 정보
    QString model_info = "Name: " + m_nrt->get_model_name() + "\n";
    model_info += "Type: " + m_nrt->get_model_type();
    ui->lab_model1_info->setText(model_info);
    model_info = "Name: " + m_nrt_ensmble->get_model_name() + "\n";
    model_info += "Type: " + m_nrt_ensmble->get_model_type();
    ui->lab_model2_info->setText(model_info);

    for(int i = 0; i < 2; i++){
        NrtExe* nrt_ptr = (i==0) ? m_nrt.get() : m_nrt_ensmble.get();
        QLabel* lab_total = (i==0) ? ui->lab_total_1 : ui->lab_total_2;

        // Run - Inference 중 테이블 세팅
        QString model_type = nrt_ptr->get_model_type();
        if(model_type == "Classification" || model_type == "Anomaly"){
            ui->chart_container->show();
            lab_total->setText("Total Images");

            if(inf_mode_status == INF_MODE_ENSEMBLE && i == 1){
                lab_total->setText("Total Regions");
            }
        }
        else if(model_type == "Detection"){
            ui->chart_container->show();
            lab_total->setText("Total Boxes");
        }
        else if(model_type == "Segmentation"){
            ui->chart_container->show();
            lab_total->setText("Total Regions");
        }
        else if(model_type == "OCR"){
            ui->chart_container->hide();
        }
        else return;

        set_model_table(i);

        if(inf_mode_status == INF_MODE_ENSEMBLE && i == 0){
            int idx = ui->com_ensemble_options->currentIndex();

            // seg->cla, det->cla 에서는 첫번째 모델의 chart를 보여주지 않음.
            if(idx == 0  || idx == 1){
                ui->vertical_model1_stats->hide();
                ui->chart_container->layout()->addItem(new QSpacerItem(0, 0, QSizePolicy::Expanding, QSizePolicy::Minimum));

                class_ratio_model1.clear();
                class_ratio_model1 = QVector<int>(m_nrt->get_model_class_num(), 0);

                avg_scores_model1.clear();
                avg_scores_model1 = QVector<float>(m_nrt->get_model_class_num(), 0.0);
            }
            else{
                set_model_chart(i);
            }
        }
        else{
            set_model_chart(i);
        }
    }
}

void MainWindow::single_realtime_setting(){
    // Run - Inference 중 페이지에서의 model 정보
    QString model_info = "Name: " + m_nrt->get_model_name() + "\n";
    model_info += "Type: " + m_nrt->get_model_type();
    ui->lab_model1_info->setText(model_info);
    ui->lab_model2_info->setText("");

    // Run - Inference 중 테이블 세팅.
    QString model_type = m_nrt->get_model_type();
    if(model_type == "Classification" || model_type == "Anomaly"){
        ui->chart_container->show();
        ui->lab_total_1->setText("Total Images");
    }
    else if(model_type == "Detection"){
        ui->chart_container->show();
        ui->lab_total_1->setText("Total Boxes");
    }
    else if(model_type == "Segmentation"){
        ui->chart_container->show();
        ui->lab_total_1->setText("Total Regions");
    }
    else if(model_type == "OCR"){
        ui->chart_container->hide();
    }
    else return;

    ui->lab_total_2->hide();
    ui->edit_total_2->hide();
    ui->table_model2->hide();
    ui->ratio_chart_model2->hide();
    ui->horizontal_box_stats->setStyleSheet("QFrame#horizontal_box_stats{border:none;}");

    set_model_table(0);
    set_model_chart(0);
}



/* 실제 save를 수행하는 thread와 inference 종료 후 결과 json을 저장하는 함수들 */
void MainWindow::save_worker() {
    QSqlDatabase save_thread_db = QSqlDatabase::addDatabase("QSQLITE", "save_thread");
    save_thread_db.setDatabaseName(QString("%1%2").arg(qApp->applicationDirPath()).arg("/neuore.db"));
    save_thread_db.open();

    QDir dir(current_save_path);
    bool ok;
    if(!dir.exists()){
        ok = dir.mkpath(current_save_path);
        if(!ok){
            qDebug() << "save_woker) unable to make current save path";
            return;
        }
    }

    while (1) {
        std::unique_lock<std::mutex> lock(m_save_info._mutex);

        Mat_With_Name org_mwn, pred_mwn;
        vector<std::string> new_row;
        int image_id = -1;
        if(!m_save_info.org_buffer.empty()){
            save_worker_busy = true;

            org_mwn = m_save_info.org_buffer.front();
            m_save_info.org_buffer.pop();

            QString org_image_name = QString(org_mwn.name.c_str());
            QString org_image_path = current_save_path + "/" + org_image_name;
            qDebug() << org_image_path;

            cv::cvtColor(org_mwn.image, org_mwn.image, cv::COLOR_RGB2BGR);
            cv::imwrite(org_image_path.toLocal8Bit().constData(), org_mwn.image);

            image_id = m_db->InsertImage(org_image_path, org_image_name, org_mwn.image.rows, org_mwn.image.cols, "save_thread");
            if(image_id == -1){
                qDebug() << "Save thread) Failed to insert image";
                continue;
            }
        }

        int resultItemID = -1;
        if(!m_save_info.pred_buffer.empty()){
            save_worker_busy = true;

            pred_mwn = m_save_info.pred_buffer.front();
            m_save_info.pred_buffer.pop();

            QString pred_image_name = QString(pred_mwn.name.c_str());
            QString pred_image_path = current_save_path + "/" + pred_image_name;

            cv::cvtColor(pred_mwn.image, pred_mwn.image, cv::COLOR_RGB2BGR);
            cv::imwrite(pred_image_path.toLocal8Bit().constData(), pred_mwn.image);

            if(image_id == -1){
                qDebug() << "save_worker) not able to locate corresponding image id";
                continue;
            }

            resultItemID = m_db->InsertResultItem(pred_image_path, image_id, m_save_info.evaluationSetId, "save_thread");
        }

        if(!m_save_info.row_buffer.empty()){
            save_worker_busy = true;

            new_row = m_save_info.row_buffer.front();
            m_save_info.row_buffer.pop();

            QJsonObject& title = m_save_info.title;

            if(inf_mode_status == INF_MODE_SINGLE && m_nrt.use_count() > 0){
                title["num_of_images"] = title["num_of_images"].toInt() + 1;

                QString model_type = m_nrt->get_model_type();
                QJsonArray body_data = title["data"].toArray();
                QJsonObject result_obj;

                qDebug() << "Save thread) Saving evaluation result ";
                if(model_type == "Classification"){
                    result_obj["result_item_id"] = resultItemID;
                    result_obj["inf_time"] = new_row[0].c_str();
                    result_obj["pred_class"] = new_row[1].c_str();
                    result_obj["score"] = new_row[2].c_str();

                    body_data.append(result_obj);
                }
                else if(model_type == "Detection"){
                    int num_of_classes = m_nrt->get_model_class_num();
                    result_obj["result_item_id"] = resultItemID;
                    result_obj["inf_time"] = new_row[0].c_str();
                    result_obj["pred_class"] = new_row[1].c_str();
                    int idx = 2;
                    for(int i = 1; i < num_of_classes; i++){
                        QString class_name = m_nrt->get_model_class_name(i);
                        result_obj[class_name + "_box_count"] = new_row[idx].c_str();
                        result_obj[class_name + "_class_mean_score"] = new_row[idx + num_of_classes].c_str();
                        idx ++;
                    }
                    result_obj["total_box_count"] = new_row[1 + num_of_classes].c_str();
                    result_obj["score"] = new_row[1 + (num_of_classes * 2)].c_str();

                    body_data.append(result_obj);
                }
                else if(model_type == "Segmentation"){
                    int num_of_classes = m_nrt->get_model_class_num();
                    result_obj["result_item_id"] = resultItemID;
                    result_obj["inf_time"] = new_row[0].c_str();

                    int idx = 1;
                    for(int i = 1; i < num_of_classes; i++){
                        QString class_name = m_nrt->get_model_class_name(i);
                        result_obj[class_name + "_pixel_rate"] = new_row[idx].c_str();
                        result_obj[class_name + "_class_mean_score"] = new_row[idx + num_of_classes - 1].c_str();
                        idx ++;
                    }
                    result_obj["score"] = new_row[idx].c_str();
                    idx++;
                    result_obj["pred_class"] = new_row[idx].c_str();

                    body_data.append(result_obj);
                }
                else if(model_type == "OCR"){
                    result_obj["result_item_id"] = resultItemID;
                    result_obj["inf_time"] = new_row[0].c_str();
                    result_obj["pred_class"] = new_row[1].c_str();
                    result_obj["score"] = new_row[2].c_str();
                    body_data.append(result_obj);
                }

                title["data"] = body_data;
            }
        }

        if(m_save_info.org_buffer.empty() && m_save_info.pred_buffer.empty() && m_save_info.row_buffer.empty()){
            save_worker_busy = false;
        }
    }
}

bool MainWindow::saveEvaluationJson()
{
    std::unique_lock<std::mutex> lock(m_save_info._mutex);

    if(!m_save_info.json_file.open(QIODevice::WriteOnly | QIODevice::Truncate)){
        qDebug() << "saveEvaluationJson) Failed to open json file.";
        return false;
    }

    if(inf_mode_status == INF_MODE_SINGLE && m_nrt.use_count() > 0){
        QJsonObject& title = m_save_info.title;
        QString model_type = m_nrt->get_model_type();

        if(title["num_of_images"].toInt() == 0){
            return false;
        }

        int START_POINT = -1;
        QString key_str = "";
        if(model_type == "Segmentation" || model_type == "Detection"){
            START_POINT = 1;
            key_str = "box_region_stats";
        }
        else if(model_type == "Classification" || model_type == "Anomaly"){
            START_POINT = 0;
            key_str = "image_stats";
        }
        else if(model_type == "OCR"){
            START_POINT = 0;
            key_str = "box_region_stats";
        }
        else return false;

        QString class_str = "";

        QJsonObject class_ratio_stats;
        QJsonObject avg_score_stats;
        for(int i = START_POINT; i < m_nrt->get_model_class_num(); i++){
            QString class_name = m_nrt->get_model_class_name(i);
            class_ratio_stats[class_name] = class_ratio_model1[i];
            if(class_ratio_model1[i] != 0){
                avg_score_stats[class_name] = QString::number(avg_scores_model1[i] / (float)class_ratio_model1[i], 'f', 2);
            }
            else{
                avg_score_stats[class_name] = "0";
            }

            if(!class_str.isEmpty()){
                class_str += ":";
            }
            class_str += class_name;
        }
        title[key_str] = class_ratio_stats;
        title["avg_score_per_class"] = avg_score_stats;
        title["end_datetime"] = QDateTime::currentDateTime().toString("yyyy-MM-dd hh:mm:ss.zzz");
        title["classes"] = class_str;
    }

    m_save_info.json_doc.setObject(m_save_info.title);
    m_save_info.json_file.write(m_save_info.json_doc.toJson());
    m_save_info.json_file.close();
    return true;
}



/* Inference 중 chart와 table의 값을 update하는 함수들 -> thread로 분리 필요 */
void MainWindow::update_pie_chart(int idx, int START_POINT){
    NrtExe* nrt_ptr = (idx==0) ? m_nrt.get() : m_nrt_ensmble.get();
    QPieSeries* series = (idx==0) ? series_model1 : series_model2;
    QVector<int> class_ratio = (idx==0) ? class_ratio_model1 : class_ratio_model2;

    int total = std::accumulate(class_ratio.begin(), class_ratio.end(), 0);

    for(int i = START_POINT; i < class_ratio.length(); i++){
        float ratio = 1;
        if(total != 0){
            ratio = (float)class_ratio[i] / total;
        }
        series->slices().at(i - START_POINT)->setValue(ratio);
//        series->slices().at(i - START_POINT)->setLabel(nrt_ptr->get_model_class_name(i) + "\n" + QString::number(class_ratio[i]));
        series->slices().at(i - START_POINT)->setLabel(QString::number(class_ratio[i]));
    }
}

void MainWindow::update_results_table(int idx, int START_POINT){
    NrtExe* nrt_ptr = (idx==0) ? m_nrt.get() : m_nrt_ensmble.get();
    QTableWidget* table = (idx==0) ? ui->table_model1 : ui->table_model2;
    QVector<float> avg_scores = (idx ==0) ? avg_scores_model1 : avg_scores_model2;
    QVector<int> class_ratio = (idx==0) ? class_ratio_model1 : class_ratio_model2;

    for(int i = START_POINT; i < avg_scores.length(); i++){
        float avg_score;
        if(class_ratio[i] == 0)
            avg_score = 0;
        else
            avg_score = avg_scores[i] / class_ratio[i];

        if(table->item(i - START_POINT, AVG_SCORE_COL)){
            if(nrt_ptr->get_model_type() == "Anomaly"){
                table->item(i - START_POINT, AVG_SCORE_COL)->setText(QString::number(avg_score, 'f', 2));
            }
            else{
                table->item(i - START_POINT, AVG_SCORE_COL)->setText(QString::number(avg_score, 'f', 2) + "%");
            }
        }
    }
}

void MainWindow::on_btn_settings_clicked(){
    if(!inference_flag){
        return;
    }

    QFont roboto_10("Roboto", 10);
    QFont robotolight_10("Roboto Light", 10);

    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    QVBoxLayout* root_cont = new QVBoxLayout;

    QWidget* container_showtime = new QWidget;
    QHBoxLayout* input_showtime =  new QHBoxLayout(container_showtime);
    QLabel* lab_show_time  = new QLabel("Show Time (sec)");
    lab_show_time->setFont(roboto_10);
    QLineEdit* show_time_edit = new QLineEdit;
    show_time_edit->setFont(robotolight_10);
    show_time_edit->setText("1");
    show_time_edit->setValidator(new QDoubleValidator(TERM_MIN, TERM_MAX, 1, this));

    if(media_mode == MEDIA_MODE_IMAGE || media_mode == MEDIA_MODE_CAM_FOLDER){
        input_showtime->addWidget(lab_show_time);
        input_showtime->addWidget(show_time_edit);
        root_cont->addWidget(container_showtime);
        root_cont->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
    }

    QWidget* container_count = new QWidget;
    QVBoxLayout* vbox_count =  new QVBoxLayout(container_count);
    QCheckBox* cbx_count = new QCheckBox("Actual / Count Compare");
    cbx_count->setFont(roboto_10);
    QLabel* lab_count = new QLabel("You can assign the actual number of objects for each class and \ncompare with the amount detected by the Object Deteciton model.");
    lab_count->setFont(robotolight_10);

    if(m_nrt.use_count() > 0 && m_nrt->get_model_type() == "Detection"){
        vbox_count->addWidget(cbx_count);
        vbox_count->addWidget(lab_count);
        root_cont->addWidget(container_count);
        root_cont->addSpacerItem(new QSpacerItem(10, 10, QSizePolicy::Minimum, QSizePolicy::Expanding));
    }

    QDialogButtonBox*btnbox = new QDialogButtonBox(QDialogButtonBox::Ok | QDialogButtonBox::Cancel,
                                                   Qt::Horizontal, &dialog);
    connect(btnbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    connect(btnbox, SIGNAL(rejected()), &dialog, SLOT(reject()));
    root_cont->addWidget(btnbox);

    dialog.setLayout(root_cont);

    if(dialog.exec() == QDialog::Accepted){
        qDebug() << "!";
    }
    else{
        return;
    }
}

void MainWindow::on_btn_model_settings_clicked(){
    if (m_timer.use_count() > 0 && m_timer->isActive()) {
        m_timer->stop();
    }

    QFont roboto_10("Roboto", 10);
    QFont robotolight_10("Roboto Light", 10);

    QDialog dialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);
    QVBoxLayout * entire_cont = new QVBoxLayout;

    QHBoxLayout* root_cont = new QHBoxLayout;

    QTableWidget* prob_thres;
    QTableWidget* size_thres;
    QTableWidget* prob_thres2;
    QTableWidget* size_thres2;

    for(int i = 0; i < 2; i ++){
        QVBoxLayout* vbox = new QVBoxLayout;

        NrtExe* nrt_ptr;
        QTableWidget* prob_table = new QTableWidget;
        QTableWidget* size_table = new QTableWidget;

        if(i==0){
            nrt_ptr = m_nrt.get();
        }
        else{
            if(inf_mode_status == INF_MODE_ENSEMBLE){
                nrt_ptr = m_nrt_ensmble.get();
            }
            else{
                continue;
            }
        }

        QString model_type = nrt_ptr->get_model_type();

        QFormLayout* model_form = new QFormLayout;
        model_form->addRow(new QLabel("Name: "), new QLabel(nrt_ptr->get_model_name()));
        model_form->addRow(new QLabel("Type: "), new QLabel(nrt_ptr->get_model_type()));
        model_form->addRow(new QLabel("Platform: "), new QLabel(nrt_ptr->get_model_training_type()));
        model_form->addRow(new QLabel("Search Level: "), new QLabel(nrt_ptr->get_model_search_level()));
        model_form->addRow(new QLabel("Inference Level: "), new QLabel(nrt_ptr->get_model_inference_level()));
        vbox->addLayout(model_form);
        vbox->addSpacing(10);

        vbox->addWidget(new QLabel("Prob threshold"));

        prob_table->setRowCount(nrt_ptr->get_model_class_num());
        prob_table->setColumnCount(2);
        QStringList header_labels = {"Class", "Prob Threshold"};
        prob_table->setHorizontalHeaderLabels(header_labels);

        for(int i = 0; i < nrt_ptr->get_model_class_num(); i++){
            QTableWidgetItem* class_name = new QTableWidgetItem;
            class_name->setText(nrt_ptr->get_model_class_name(i));
            class_name->setTextAlignment(Qt::AlignCenter);
            prob_table->setItem(i, 0, class_name);
            prob_table->item(i, 0)->setFlags(prob_table->item(i, 0)->flags() & ~Qt::ItemIsEditable);

            QTableWidgetItem* prob = new QTableWidgetItem;
            int cur_prob_thres = 0;
            if(i < nrt_ptr->prob_threshold.length()){
                cur_prob_thres = nrt_ptr->prob_threshold[i];
            }
            prob->setText(QString::number(cur_prob_thres) + "%");
            prob->setTextAlignment(Qt::AlignCenter);
            prob_table->setItem(i, 1, prob);
        }

        // table view ui 조정
        prob_table->verticalHeader()->hide();
        prob_table->horizontalHeader()->show();
        prob_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
        prob_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
        verticalResizeTable(prob_table);

        vbox->addWidget(prob_table);
        vbox->addSpacing(10);

        connect(prob_table,
                QOverload<const int, const int>::of(&QTableWidget::cellChanged),
                [prob_table](const int row, const int col)
                {
                    if(col == 1){
                        QString input = prob_table->item(row, col)->text();
                        input = input.remove(QRegExp("[^\\d]"));

                        float value = 0;
                        bool ok;
                        value = input.toFloat(&ok);
                        if(!ok){
                            qDebug() << "Failed to convert to int";
                            value = 0;
                        }
                        prob_table->item(row, col)->setText(QString::number(value, 'f', 2) + QString("%"));
                    }
                });

        if(model_type == "Segmentation" || model_type == "Detection"){
            vbox->addWidget(new QLabel("Size threshold"));
            size_table->setRowCount(nrt_ptr->get_model_class_num());
            size_table->setColumnCount(4);
            QStringList header_labels = {"Class", "Height", "AND/OR", "Width"};
            size_table->setHorizontalHeaderLabels(header_labels);

            for(int i = 0; i < nrt_ptr->get_model_class_num(); i++){
                QTableWidgetItem* class_name = new QTableWidgetItem;
                class_name->setText(nrt_ptr->get_model_class_name(i));
                class_name->setTextAlignment(Qt::AlignCenter);
                size_table->setItem(i, 0, class_name);
                size_table->item(i, 0)->setFlags(size_table->item(i, 0)->flags() & ~Qt::ItemIsEditable);

                QTableWidgetItem* width = new QTableWidgetItem;
                int cur_width = 0;
                if(i < nrt_ptr->size_threshold.length()){
                    cur_width = nrt_ptr->size_threshold[i].second;
                }
                width->setText(QString::number(cur_width));
                width->setTextAlignment(Qt::AlignCenter);
                size_table->setItem(i, 1, width);

                QTableWidgetItem* conj = new QTableWidgetItem;
                QString cur_conj = "AND";
                if(i < nrt_ptr->size_thres_conjunction.length()){
                    cur_conj = nrt_ptr->size_thres_conjunction[i];
                }
                conj->setText(cur_conj);
                conj->setTextAlignment(Qt::AlignCenter);
                size_table->setItem(i, 2, conj);
                size_table->item(i, 2)->setFlags(size_table->item(i, 0)->flags() & ~Qt::ItemIsEditable);

                QTableWidgetItem* height = new QTableWidgetItem;
                int cur_height = 0;
                if(i < nrt_ptr->size_threshold.length()){
                    cur_height = nrt_ptr->size_threshold[i].first;
                }
                height->setText(QString::number(cur_height));
                height->setTextAlignment(Qt::AlignCenter);
                size_table->setItem(i, 3, height);

                // table view ui 조정
                size_table->verticalHeader()->hide();
                size_table->horizontalHeader()->show();
                size_table->horizontalHeader()->setSectionResizeMode(QHeaderView::Stretch);
                size_table->horizontalHeader()->setSectionResizeMode(1, QHeaderView::ResizeToContents);
                size_table->horizontalHeader()->setSectionResizeMode(2, QHeaderView::ResizeToContents);
                size_table->horizontalHeader()->setSectionResizeMode(3, QHeaderView::ResizeToContents);
                verticalResizeTable(size_table);
            }

            vbox->addWidget(size_table);

            connect(size_table,
                    QOverload<const int, const int>::of(&QTableWidget::cellChanged),
                    [size_table](const int row, const int col)
                    {
                        // Height, Width
                        if(col == 1 || col == 3){
                            QString input = size_table->item(row, col)->text();
                            input = input.remove(QRegExp("[^\\d]"));

                            int value = 0;
                            bool ok;
                            value = input.toInt(&ok);
                            if(!ok){
                                qDebug() << "Failed to convert to int";
                                value = 0;
                            }
                            size_table->item(row, col)->setText(QString::number(value));
                        }
                    });

            connect(size_table,
                    QOverload<const int, const int>::of(&QTableWidget::cellClicked),
                    [size_table](const int row, const int col)
                    {
                        // AND, OR
                        if(col == 2){
                            QString cur_txt = size_table->item(row, col)->text();
                            size_table->item(row, col)->setText((cur_txt == "AND" ? "OR" : "AND"));
                        }
                    });
        }

        if(i==0){
            prob_thres = prob_table;
            size_thres = size_table;
        }
        else{
            if(inf_mode_status == INF_MODE_ENSEMBLE && m_nrt_ensmble.use_count() > 0){
                prob_thres2 = prob_table;
                size_thres2 = size_table;
            }
            else{
                continue;
            }
        }
        vbox->addSpacerItem(new QSpacerItem(0, 0, QSizePolicy::Minimum, QSizePolicy::Expanding));
        root_cont->addLayout(vbox);

        if(i == 0){
            root_cont->addSpacing(15);
        }
    }
    entire_cont->addLayout(root_cont);

    QDialogButtonBox*btnbox = new QDialogButtonBox(QDialogButtonBox::Ok,
                                                   Qt::Horizontal, &dialog);
    connect(btnbox, SIGNAL(accepted()), &dialog, SLOT(accept()));
    entire_cont->addWidget(btnbox);
    dialog.setLayout(entire_cont);

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

        if(inf_mode_status == INF_MODE_ENSEMBLE && m_nrt_ensmble.use_count() > 0){
            m_nrt_ensmble->prob_threshold.clear();
            for(int i = 0; i < prob_thres2->rowCount(); i++){
                QString prob = prob_thres2->item(i, 1)->text().remove(QChar('%'));
                bool ok;
                float prob_val = prob.toFloat(&ok);
                if(!ok){
                    prob_val = 0.0;
                }
                m_nrt_ensmble->prob_threshold.append(prob_val);
            }

            m_nrt_ensmble->size_threshold.clear();
            for(int i = 0; i < size_thres2->rowCount(); i++){
                QString h_thres = size_thres2->item(i, 1)->text();
                bool ok;
                int h_thres_val = h_thres.toInt(&ok);
                if(!ok){
                    h_thres_val = 0;
                }

                QString w_thres = size_thres2->item(i, 3)->text();
                int w_thres_val = w_thres.toInt(&ok);
                if(!ok){
                    w_thres_val = 0;
                }

                m_nrt_ensmble->size_threshold.append(qMakePair(h_thres_val, w_thres_val));

                QString conj = size_thres2->item(i, 2)->text();
                m_nrt_ensmble->size_thres_conjunction.append(conj);
            }
        }
    }

    if (m_timer.use_count() > 0) {
        m_timer->setInterval(0);
        m_timer->start();
    }
}

void MainWindow::model_table_item_clicked(int row, int col){
    if(col == CLASS_COL || col == COUNT_COL || col == AVG_SCORE_COL){
        return;
    }

    if(m_timer.use_count() >0 && m_timer->isActive()){
        m_timer->stop();
    }

    // color col
    if(col == COLOR_COL){
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

            if(inf_mode_status == INF_MODE_SINGLE){
                ui->table_model1->setCellWidget(row, COLOR_COL, cWidget);
            }
            else{
                ui->table_model2->setCellWidget(row, COLOR_COL, cWidget);
            }

            COLOR_VECTOR[row] = new_color;

            if(inf_mode_status == INF_MODE_SINGLE){
                series_model1->slices().at(row)->setColor(new_color);
            }
            else{
                series_model2->slices().at(row)->setColor(new_color);
            }
        }
    }

    if(m_timer.use_count() > 0){
        m_timer->start();
    }
}

void MainWindow::model_table_item_changed(int row, int col){
    if(col == ACTUAL_COL){
        int input;
        bool ok;
        if(inf_mode_status == INF_MODE_SINGLE){
            input = ui->table_model1->item(row, ACTUAL_COL)->text().toInt(&ok);
            if(!ok){
                ui->table_model1->item(row, ACTUAL_COL)->setText("0");
            }
        }
        else{
            input = ui->table_model2->item(row, ACTUAL_COL)->text().toInt(&ok);
            if(!ok){
                ui->table_model2->item(row, ACTUAL_COL)->setText("0");
            }
        }
    }
}

void MainWindow::on_btn_full_screen_viewer_clicked(){
    if(full_screen_dialog->isVisible()){
        return;
    }
    full_screen_dialog = new QDialog(this, Qt::WindowTitleHint | Qt::WindowCloseButtonHint);

    QVBoxLayout* root_cont = new QVBoxLayout;
    root_cont->addWidget(lab_full_screen);
    full_screen_dialog->setLayout(root_cont);

    full_screen_dialog->setMinimumSize(QSize(800, 600));
    full_screen_dialog->installEventFilter(this);
    full_screen_dialog->show();
}



/* infernce 종료 후 */
void MainWindow::show_result_image_table(int evaluationSetId){
    QString evalJsonPath = m_db->getEvaluationJsonPath(evaluationSetId);
    qDebug() << "json path: " << evalJsonPath;

    on_btn_review_clicked();

    // 해당하는 json 파일을 읽어와야 함.
    QFile file;
    QJsonDocument json_doc;
    QJsonObject root_obj;

    file.setFileName(evalJsonPath);
    if(!file.open(QIODevice::ReadOnly)){
        qDebug() << "Failed to open json file.";
        on_btn_run_clicked();
    }

    QByteArray load_data = file.readAll();
    json_doc = QJsonDocument::fromJson(load_data);
    root_obj = json_doc.object();
    file.close();

    // start time
    QString start_time = root_obj["start_datetime"].toString();
    ui->lab_eval_start_time->setText(start_time);

    // end time
    QString end_time = root_obj["end_datetime"].toString();
    ui->lab_eval_end_time->setText(end_time);

    // model names
    int model1_id = root_obj["model1_id"].toInt();
    QString model_name = m_db->getModelName(model1_id);
    QString model_type = m_db->getModelType(model1_id);
    ui->lab_eval_model_names->setText(model_name + " ("  + model_type + ")");

    // Classes
    QStringList classes = root_obj["classes"].toString().split(":");

    if(!root_obj["data"].isArray()){
        on_btn_run_clicked();
        qDebug() << "show result image table) not array";
        return;
    }
    QJsonArray data = root_obj["data"].toArray();

    // Header Setting
    QStringList hor_header_label;
    int column_count = 0;
    if(model_type == "Segmentation"){
        hor_header_label = QStringList({"Name", "Pred Class", "Score", "Inf Time"});
        for(int i = 0; i < classes.size(); i++){
            QString area_ratio_lab = classes[i] + " Area Ratio";
            hor_header_label.append(area_ratio_lab);
        }
        column_count = 4 + classes.size();

        score_col = 2;
        inf_col = 3;
    }
    else if(model_type == "Detection"){
        hor_header_label = QStringList({"Name", "Pred Class", "Box Count", "Score", "Inf Time"});
        column_count = 5;

        score_col = 3;
        inf_col = 4;
    }
    else if(model_type == "Classification"){
        hor_header_label = QStringList({"Name", "Pred Class", "Score", "Inf Time"});
        column_count = 4;

        score_col = 2;
        inf_col = 3;
    }
    else if(model_type == "OCR"){
        hor_header_label = QStringList({"Name", "Pred String", "Char Count", "Score", "Inf Time"});
        column_count = 5;

        score_col = 3;
        inf_col = 4;
    }
    else if(model_type == "Anomaly"){
        hor_header_label = QStringList({"Name", "Pred Class", "Score", "Inf Time"});
        column_count = 4;

        score_col = 2;
        inf_col = 3;
    }
    else{
        on_btn_run_clicked();
        return;
    }

    int num_of_images = 0;
    ui->result_images_table->setColumnCount(column_count);
    ui->result_images_table->verticalHeader()->hide();
    ui->result_images_table->setHorizontalHeaderLabels(hor_header_label);
    ui->result_images_table->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    if(model_type != "Segmentation"){
        ui->result_images_table->horizontalHeader()->setSectionResizeMode(0, QHeaderView::Stretch);
    }

    org_images.clear();
    result_images.clear();
    for(int i = 0; i < data.size(); i++){
        if(!data[i].isObject()){
            on_btn_run_clicked();
            qDebug() << "show result image table) not object";
            return;
        }
        QJsonObject data_obj = data[i].toObject();

        int result_item_id = data_obj["result_item_id"].toInt();
        int org_id = m_db->getImageId(result_item_id);
        if(org_id == -1 || result_item_id ==-1){
            qDebug() << "no image";
            continue;
        }

        QString result_item_path = m_db->getResultImagePath(result_item_id);
        QString org_path = m_db->getImagePath(org_id);

        cv::Mat image = cv::imread(org_path.toLocal8Bit().constData(), cv::IMREAD_COLOR);
        qDebug() << org_path;
        cv::Mat result_image = cv::imread(result_item_path.toLocal8Bit().constData(), cv::IMREAD_COLOR);
        qDebug() << result_item_path;

        if(image.empty() || result_image.empty()){
            qDebug() << "image read fail";
            continue;
        }

        org_images.append(image);
        result_images.append(result_image);

        ui->result_images_table->insertRow(ui->result_images_table->rowCount());
        num_of_images++;

        int col_idx = 0;

        // image name
        QString image_name = m_db->getImageName(org_id);
        if(image_name.isEmpty()) return;
        QTableWidgetItem* name_item = new QTableWidgetItem;
        name_item->setText(image_name);
        name_item->setTextAlignment(Qt::AlignCenter);
        ui->result_images_table->setItem(ui->result_images_table->rowCount() - 1, col_idx, name_item);
        col_idx++;

        // pred class
        QString pred_class = data_obj["pred_class"].toString();
        QTableWidgetItem* class_item = new QTableWidgetItem;
        class_item->setText(pred_class);
        class_item->setTextAlignment(Qt::AlignCenter);
        ui->result_images_table->setItem(ui->result_images_table->rowCount() - 1, col_idx, class_item);
        col_idx++;

        // count (box, ocr)
        if(model_type == "Detection" || model_type == "OCR"){
            QString count;
            if(model_type == "Detection"){
                count = data_obj["total_box_count"].toString();
            }
            else{
                count = QString::number(pred_class.size());
            }

            QTableWidgetItem* count_item = new QTableWidgetItem;
            count_item->setText(count);
            count_item->setTextAlignment(Qt::AlignCenter);
            col_idx++;
        }

        // score
        QString score = data_obj["score"].toString();
        QTableWidgetItem* score_item = new QTableWidgetItem;
        score_item->setText(score);
        score_item->setTextAlignment(Qt::AlignCenter);
        ui->result_images_table->setItem(ui->result_images_table->rowCount() - 1, col_idx, score_item);
        col_idx++;

        // inf time
        QString inf_time = data_obj["inf_time"].toString();
        QTableWidgetItem* inf_item = new QTableWidgetItem;
        inf_item->setText(inf_time);
        inf_item->setTextAlignment(Qt::AlignCenter);
        ui->result_images_table->setItem(ui->result_images_table->rowCount() - 1, col_idx, inf_item);
        col_idx++;

        // seg - pixel rate
        for(int i = 0; i < classes.size(); i++){
            QString class_name = classes[i];
            QString area_ratio = data_obj[class_name + "_pixel_rate"].toString();

            QTableWidgetItem* area_item = new QTableWidgetItem;
            area_item->setText(area_ratio);
            area_item->setTextAlignment(Qt::AlignCenter);
            ui->result_images_table->setItem(ui->result_images_table->rowCount() - 1, col_idx, area_item);
            col_idx++;
        }
    }

    // table model2 hide
    ui->lab_total_2->hide();
    ui->edit_total_2->hide();
    ui->table_model2->hide();
    ui->ratio_chart_model2->hide();
    ui->horizontal_box_stats->setStyleSheet("QFrame#horizontal_box_stats{border:none;}");

    // table model1 set class and avg score
    ui->table_model1->hideColumn(COLOR_COL);
    ui->table_model1->hideColumn(ACTUAL_COL);
    ui->table_model1->hideColumn(COUNT_COL);
    ui->table_model1->clearContents();
    ui->table_model1->setRowCount(0);

    // Table Avg Score
    QJsonObject score_obj = root_obj["avg_score_per_class"].toObject();

    // Pie chart
    QJsonObject stats_obj;
    int total = 0;
    if(model_type == "Segmentation" || model_type == "Detection"){
        stats_obj = root_obj["box_region_stats"].toObject();

        QString txt = (model_type=="Segmentation") ? "Total Regions" : "Total Boxes";
        ui->lab_total_1->setText(txt);
    }
    else if(model_type == "Classification" || model_type == "Anomaly"){
        stats_obj = root_obj["image_stats"].toObject();
        ui->lab_total_1->setText("Total Images");
    }
    else if(model_type == "OCR"){
        stats_obj = root_obj["box_region_stats"].toObject();
        ui->lab_total_1->setText("Total Boxes");
    }
    else{
        on_btn_run_clicked();
        return;
    }

    for(int i = 0; i < classes.size(); i++){
        QString class_name = classes[i];

        QString class_avg_score = score_obj[class_name].toString();

        QTableWidgetItem* class_item = new QTableWidgetItem;
        class_item->setText(class_name);
        class_item->setTextAlignment(Qt::AlignCenter);

        QTableWidgetItem* avgscore_item = new QTableWidgetItem;
        avgscore_item->setText(class_avg_score);
        avgscore_item->setTextAlignment(Qt::AlignCenter);

        ui->table_model1->insertRow(ui->table_model1->rowCount());
        ui->table_model1->setItem(ui->table_model1->rowCount() - 1, CLASS_COL, class_item);
        ui->table_model1->setItem(ui->table_model1->rowCount() - 1, AVG_SCORE_COL, avgscore_item);

        if(!stats_obj[class_name].isUndefined()){
            total += stats_obj[class_name].toInt();
        }
    }

    series_model1 = new QPieSeries;

    chart_model1 = new QChart;

    for(int i = 0; i < classes.size(); i++){
        QString class_name = classes[i];

        int class_stats;
        if(!stats_obj[class_name].isUndefined()){
            class_stats = stats_obj[class_name].toInt();
        }
        else{
            class_stats = 0;
        }
        float ratio;
        if(total != 0){
            ratio = (float)class_stats / (float)total;
        }
        else{
            ratio = 0.0;
        }

        std::random_device rd;
        std::mt19937 gen(rd());
        std::uniform_int_distribution<int> dis(0, 255);

        series_model1->append(class_name + " " + QString::number(class_stats), ratio);

        series_model1->setLabelsVisible(true);
        series_model1->setLabelsPosition(QPieSlice::LabelInsideHorizontal);

        series_model1->slices().at(i)->setColor(QColor(dis(gen), dis(gen), dis(gen)));
        series_model1->slices().at(i)->setLabelColor(QColor(255, 255, 255));
        series_model1->slices().at(i)->setLabelFont(QFont("Roboto", 10));
    }
    ui->edit_total_1->setText(QString::number(total));

    chart_model1->addSeries(series_model1);
    chart_model1->legend()->hide();
    chart_model1->layout()->setContentsMargins(0, 0, 0, 0);
    chart_model1->setBackgroundRoundness(0);
    chart_model1->setPlotArea(QRectF(0, 0, 180, 180));
    ui->ratio_chart_model1->setChart(chart_model1);
    ui->ratio_chart_model1->setRenderHint(QPainter::Antialiasing);

    if(ui->result_images_table->rowCount() > 0){
        ui->result_images_table->selectRow(0);
        ui->result_images_table->scrollToTop();
    }

    btn_show_pred_connection = connect(btn_show_prediction, &QPushButton::clicked, [&](){
        on_result_images_table_itemSelectionChanged();
    });
}

void MainWindow::on_result_images_table_itemSelectionChanged(){
    if(ui->result_images_table->selectionModel()->hasSelection()){
        int row = ui->result_images_table->selectionModel()->currentIndex().row();

        if(row >= result_images.length() || row >= org_images.length()){
            return;
        }

        cur_result_img_idx = row;

        cv::Mat img;
        if(show_pred_flag){
            img = result_images[row].clone();
        }
        else{
            img = org_images[row].clone();
        }

        cv::cvtColor(img, img, cv::COLOR_RGB2BGR);
        QImage m_qimage((const unsigned char*)img.data, img.cols, img.rows, img.step, QImage::Format_RGB888);
        QPixmap m_qpixmap = QPixmap::fromImage(m_qimage);
        ui->lab_show_res->setPixmap(m_qpixmap.scaled(ui->lab_show_res->width(), ui->lab_show_res->height(), Qt::KeepAspectRatio));
        lab_full_screen->setPixmap(m_qpixmap.scaled(lab_full_screen->width(), lab_full_screen->height(), Qt::KeepAspectRatio));

        QString pred_class = ui->result_images_table->item(row, 1)->text();
        QString score = ui->result_images_table->item(row, score_col)->text();
        QString inf = ui->result_images_table->item(row, inf_col)->text();

        ui->edit_show_class->setText(pred_class);
        ui->edit_show_inf->setText(inf);
        ui->edit_show_score->setText(score);
    }
}



void MainWindow::verticalResizeTable(QTableView* table_view){
    // vertical resize table widget to contents
    int totalRowHeight = 0;

    // visible row height
    int count = table_view->verticalHeader()->count();
    for(int i = 0; i < count; i++){
        if(!table_view->verticalHeader()->isSectionHidden(i)){
            totalRowHeight +=  table_view->verticalHeader()->sectionSize(i);
        }
    }

    //scrollbar visibility
    if(!table_view->horizontalScrollBar()->isHidden()){
        totalRowHeight += table_view->horizontalScrollBar()->height();
    }

    table_view->setMinimumHeight(totalRowHeight);
    table_view->setMaximumHeight(totalRowHeight);
}
