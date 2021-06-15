#ifndef NRTEXE_H
#define NRTEXE_H

#include "shared_include.h"
#include <nrt.h>
#include <sqlitedb.h>

struct BoundingBox {
    int batch_index;
    int box_center_X;
    int box_center_Y;
    int box_width;
    int box_height;
    int class_number;
};

BoundingBox convert_to_bounding_box(const int* bcyxhw_ptr, const double h_ratio, const double w_ratio);
bool bbox_cmp(const BoundingBox & a, const BoundingBox & b);

class NrtExe
{
private:
    nrt::Device m_device;
    shared_ptr<nrt::Executor> m_executor_ptr;
    shared_ptr<nrt::Model> m_model_ptr;

public:
    NrtExe();
    ~NrtExe();

    int get_gpu_num();
    bool get_gpu_status();
    bool set_gpu(int gpuIdx);
    QString get_gpu_name();

    QString set_model(QString modelPath, bool fp16_flag);
    int get_model_status();
    QString get_model_name();
    QString get_model_type();
    QString get_model_training_type();
    QString get_model_search_level();
    QString get_model_inference_level();
    int get_model_class_num();
    QString get_model_class_name(int class_idx);
    nrt::NDBuffer get_model_prob_threshold();
    nrt::NDBuffer get_model_size_threshold();
    nrt::Shape get_model_input_shape(int idx);
    nrt::DType get_model_input_dtype(int idx);
    nrt::InterpolationType get_model_interpolty(int idx);
    float get_model_scale_factor();
    int get_model_output_num();
    bool is_model_patch_mode();

    int get_executor_status();

    nrt::NDBufferList execute(nrt::NDBuffer resized_img_buffer);

    int PROB_IDX = -1, PRED_IDX = -1, CAM_IDX = -1, ANO_IDX = -1, ROT_IDX=-1;
    std::vector<float> class_threshold;
};


#endif // NRTEXE_H
