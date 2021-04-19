#include "usbcam.h"

UsbCam::UsbCam()
{
    resValue.append(qMakePair(320, 240));
    resValue.append(qMakePair(640, 480));
    resValue.append(qMakePair(720, 480));
    resValue.append(qMakePair(800, 600));
    resValue.append(qMakePair(1280, 720));
    resValue.append(qMakePair(1920, 1080));
    resValue.append(qMakePair(2048, 1080));
    resValue.append(qMakePair(4096, 2160));
}

UsbCam::~UsbCam()
{

}

void UsbCam::usbCamError(int errNum)
{
    m_camera_work = false;
    QMessageBox *err_msg = new QMessageBox;
    err_msg->setFixedSize(600, 400);

    switch (errNum)
    {
    // cap open error
    case 1:
        qDebug() << "USB CAM ERROR 1";
        err_msg->critical(0,"Error", "CAM isn't opened.\nPlease select another CAM.");
        break;
    // fps: -1  error
    case 2:
        qDebug() << "USB CAM ERROR 2";
        m_cap.release();
        err_msg->critical(0,"Error", "CAM didn't work.\nPlease select another CAM.");
        break;
    // setResult error
    case 3:
        qDebug() << "USB CAM ERROR 3";
        m_frame.release();
        err_msg->critical(0,"Error", "CAM didn't work.\nPlease select another CAM.");
        break;
    }
}

bool UsbCam::isExist()
{
    return m_camera_work;
}

void UsbCam::selectCam() {
    qDebug() << "Detecting USB CAM..";
    QList<QCameraInfo> cameras = QCameraInfo::availableCameras();
    qDebug() << QCameraInfo::availableCameras().count() << "CAMs are detected..";
    if (cameras.count() < 1) {
        QMessageBox *err_msg = new QMessageBox;
        err_msg->setFixedSize(600, 400);
        err_msg->critical(0,"Error", "There are no availablke CAMs.");
        return;
    }

    QDialog *selectDlg = new QDialog;
    selectDlg->setWindowTitle("CAM List");
    selectDlg->setFixedSize(400, 300);

    QVBoxLayout *dlgVLayout = new QVBoxLayout;

    QLabel *cHeader = new QLabel("CAM List", selectDlg);
    dlgVLayout->addWidget(cHeader);

    QListWidget *camList = new QListWidget;
    int idx = 0;
    for (const QCameraInfo &cameraInfo : cameras) {
        QListWidgetItem *camItem = new QListWidgetItem;
        camItem->setText(cameraInfo.description());
        camList->insertItem(idx, camItem);
        idx++;
    }
    dlgVLayout->addWidget(camList);

    QLabel *rHeader = new QLabel("Resolution", selectDlg);
    QHBoxLayout *whHLayout = new QHBoxLayout;
    QLabel *whHeader = new QLabel("  - Width x Height", selectDlg);
    QComboBox *resCombobox = new QComboBox;
    QStringList resList;
    for (QPair<int, int> &res : resValue)
        resList.append(QString::number(res.first) + QString(" * ") + QString::number(res.second));
    resCombobox->addItems(resList);
    whHLayout->setStretchFactor(whHeader, 1);
    whHLayout->setStretchFactor(resCombobox, 1);
    QHBoxLayout *fpsHLayout = new QHBoxLayout;
    QLabel *fpsHeader = new QLabel("  - FPS (0 < fps <= 100)", selectDlg);
    QLineEdit *fpsLineEdit = new QLineEdit;
    fpsLineEdit->setText("30");
    fpsLineEdit->setValidator(new QIntValidator(1, 100, selectDlg));
    fpsLineEdit->setSizePolicy(QSizePolicy::Preferred , QSizePolicy::Preferred);
    fpsHLayout->setStretchFactor(fpsHeader, 1);
    fpsHLayout->setStretchFactor(fpsLineEdit, 1);

    whHLayout->addWidget(whHeader);
    whHLayout->addWidget(resCombobox);
    fpsHLayout->addWidget(fpsHeader);
    fpsHLayout->addWidget(fpsLineEdit);
    dlgVLayout->addWidget(rHeader);
    dlgVLayout->addLayout(whHLayout);
    dlgVLayout->addLayout(fpsHLayout);

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
        idx = camList->currentRow();

        // Index that must be sent to the video cap object is listed in /dev/vedio*
        std::string device_path = cameras.at(idx).deviceName().toStdString();
        std::size_t i = device_path.find_last_of("/");
        std::string device_name = device_path.substr(i+1, device_path.length());

        std::regex re("[^0-9]*([0-9]+)");
        std::smatch match;
        std::string device_num;
        if (std::regex_match(device_name, match, re)){
            for (size_t i = 0; i < match.size(); i++) {
                device_num = match[i];
             }
        }

        int device_idx = -1;
        try {
            device_idx = std::stoi(device_num);
        } catch (const std::exception& expn) {
            std::cout << expn.what() << ": Out of integer's range\n";
        } catch (...) {
            std::cout << ": Unknown error\n";
        }

        qDebug() << "User Select:" << cameras.at(idx).description();
        int w = resValue.at(resCombobox->currentIndex()).first;
        int h = resValue.at(resCombobox->currentIndex()).second;
        int fps = fpsLineEdit->text().toInt();
        qDebug() << "W: " << w << ", " << "H: " << h << ", " << "FPS: " << fps;
        setCam(device_idx, w, h, fps);
    }
    else
        return;
}


// videoIdx: GStreamer pipeline stream?
void UsbCam::setVideoCapture(int videoIdx) {
    // NEED DEVELOP //
    //cv::VideoCaptureAPIs videoCapAPI = cv::CAP_ANY;
    cv::VideoCaptureAPIs videoCapAPI = cv::CAP_V4L2;

    // VideoCapture (const String &filename, int apiPreference=CAP_ANY)
    m_cap.open(videoIdx, videoCapAPI);
}

void UsbCam::setWidth(int width) {
    if (m_cap.isOpened())
        m_cap.set(cv::CAP_PROP_FRAME_WIDTH, width);
    qDebug() << "Width: " << m_cap.get(cv::CAP_PROP_FRAME_WIDTH);
}

void UsbCam::setHeight(int height) {
    if (m_cap.isOpened())
        m_cap.set(cv::CAP_PROP_FRAME_HEIGHT, height);
    qDebug() << "Height: " << m_cap.get(cv::CAP_PROP_FRAME_HEIGHT);
}

void UsbCam::setFps(int fps) {
    if (m_cap.isOpened())
        m_cap.set(cv::CAP_PROP_FPS, fps);
    qDebug() << "FPS: " << m_cap.get(cv::CAP_PROP_FPS);
}

void UsbCam::setResoultion(int width, int height, int fps) {
    setWidth(width);
    setHeight(height);
    setFps(fps);
}

void UsbCam::setCam(int videoIdx, int width, int height, int fps) {
    try {
        setVideoCapture(videoIdx);
        if (!m_cap.isOpened()) {
            throw 1;
        }
        setResoultion(width, height, fps);
        if (m_cap.get(cv::CAP_PROP_FPS) == -1) {
            throw 2;
        }
        m_camera_work = true;

        QMessageBox setCamInfo;
        setCamInfo.setWindowTitle("CAM Information");
        setCamInfo.setText(QString("Your CAM provide spec.\n  - Width : %1\n  - Height : %2\n  - Fps: %3").arg(m_cap.get(cv::CAP_PROP_FRAME_WIDTH)).arg(m_cap.get(cv::CAP_PROP_FRAME_HEIGHT)).arg(m_cap.get(cv::CAP_PROP_FPS)));
        setCamInfo.exec();
        qDebug() << "Provide Spec - Width:" << m_cap.get(cv::CAP_PROP_FRAME_WIDTH) << "Height:" << m_cap.get(cv::CAP_PROP_FRAME_HEIGHT) << "FPS: " << m_cap.get(cv::CAP_PROP_FPS) << endl;
    }
    catch (int errNum) {
        usbCamError(errNum);
    }
}

void UsbCam::playCam() {
    if (m_cap.isOpened()) {
        m_camera_work = true;
    }
}

void UsbCam::pauseCam() {
}

void UsbCam::stopCam() {
    // cv::VideoCapture m_cap;
    if (m_cap.isOpened()) {
        m_camera_work = false;
        m_cap.release();
    }
}

void UsbCam::setResult() {
    try {
        if (!m_cap.isOpened())
            throw 3;
        if (m_cap.get(cv::CAP_PROP_FPS) == -1)
            throw 3;

        m_cap >> m_frame;
        if (m_frame.empty())
            throw 3;

        m_camera_work = true;
    }
    catch (int errNum) {
        usbCamError(3);
    }
}
