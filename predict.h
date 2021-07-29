#ifndef PREDICT_H
#define PREDICT_H

#include <shared_include.h>
#include <nrt.h>
#include <nrtexe.h>
#include <ui_mainwindow.h>

nrt::NDBuffer get_img_buffer(cv::Mat ORG_IMG, NrtExe* m_nrt);
QVector<nrt::NDBuffer> seg_execute (nrt::NDBuffer image_buff, std::chrono::duration<double, std::milli> &inf_time, NrtExe* m_nrt);

#endif // PREDICT_H
