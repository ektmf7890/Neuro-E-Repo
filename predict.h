#ifndef PREDICT_H
#define PREDICT_H

#include <shared_include.h>
#include <nrt.h>
#include <nrtexe.h>
#include <ui_mainwindow.h>

nrt::NDBuffer cla_get_resized_img_buffer(cv::Mat ORG_IMG);
vector<std::string> cla_get_cls_prob_vec (nrt::NDBuffer outputs);
std::string cla_get_pred_cls (nrt::NDBuffer outputs);
cv::Mat cla_get_pred_img(nrt::NDBuffer outputs, cv::Mat ORG_IMG);

#endif // PREDICT_H
