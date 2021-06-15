#ifndef USBCAM_H
#define USBCAM_H
#include "shared_include.h"
#include <QLabel>
#include <QImage>
#include <QPixmap>
#include <QPair>

#include <opencv2/highgui.hpp>
#include <opencv2/imgproc.hpp>
using namespace cv;

class UsbCam
{
public:
    UsbCam();
    ~UsbCam();
    cv::Mat m_frame;

public slots:
    bool isExist();

    void selectCam();
    void setVideoCapture(int videoIdx);
    void setCam(int videoIdx, int width, int height, int fps);

    void setResult();
    void playCam();
    void pauseCam();
    void stopCam();

private slots:
    void usbCamError(int errNum);

    void changeResList();

    void setWidth(int width);
    void setHeight(int height);
    void setFps(int fps);
    void setResoultion(int width, int height, int fps);

private:
    QList<QPair<int, int>> resValue;

    bool m_camera_work = false;
    cv::VideoCapture m_cap;
};


#endif // USBCAM_H
