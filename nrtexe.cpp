/*
Using Neuro-R version 2.2.3
*/

#include <nrtexe.h>

using namespace std;
using namespace cv;

BoundingBox convert_to_bounding_box(const int* bcyxhw_ptr, const double h_ratio, const double w_ratio) {
    BoundingBox bbox;
    bbox.batch_index  = bcyxhw_ptr[0];
    bbox.class_number = bcyxhw_ptr[1];
    bbox.box_center_Y = bcyxhw_ptr[2];
    bbox.box_center_X = bcyxhw_ptr[3];
    bbox.box_height   = bcyxhw_ptr[4];
    bbox.box_width    = bcyxhw_ptr[5];

    bbox.box_center_Y = (double)bbox.box_center_Y / h_ratio;
    bbox.box_center_X = (double)bbox.box_center_X / w_ratio;
    bbox.box_height = (double)bbox.box_height / h_ratio;
    bbox.box_width = (double)bbox.box_width / w_ratio;

    return bbox;
}

bool bbox_cmp(const BoundingBox & a, const BoundingBox & b) {
    return (a.box_center_X < b.box_center_X);
}

NrtExe::NrtExe(){};
NrtExe::~NrtExe(){};

int NrtExe::get_gpu_num() {
    return nrt::Device::get_num_gpu_devices();
}

bool NrtExe::get_gpu_status() {
    return (m_device.devtype == nrt::DevType::DEVICE_CUDA_GPU);
}

bool NrtExe::set_gpu(int gpuIdx) {
    m_device = nrt::Device::get_gpu_device(gpuIdx);
    if (m_device.devtype == nrt::DevType::DEVICE_CUDA_GPU)
        qDebug() << "NRT) Set GPU" << gpuIdx << ":" << get_gpu_name();
    return (m_device.devtype == nrt::DevType::DEVICE_CUDA_GPU);
}

QString NrtExe::get_gpu_name() {
    return QString(m_device.get_device_name());
}

bool NrtExe::set_cpu(){
    m_device = nrt::Device::get_cpu_device();
    if(m_device.devtype == nrt::DevType::DEVICE_CPU){
        qDebug() << "NRT) Set CPU";
    }
    return (m_device.devtype == nrt::DevType::DEVICE_CPU);
}

QString NrtExe::set_model(QString modelPath, bool fp16_flag) {
    // Load Model File
    if (m_model_ptr.use_count() > 0)
        m_model_ptr.reset();

    // QString to wchar_t*
    wchar_t mModelPath[512];
    modelPath.toWCharArray(mModelPath);
    mModelPath[modelPath.length()] = L'\0';

    m_model_ptr = make_shared<nrt::Model>(mModelPath);

    if (m_model_ptr->get_status() != nrt::STATUS_SUCCESS) {
        qDebug() << "Model first initialization failed.  : " << QString(nrt::get_last_error_msg()) << endl;
        return QString("");
    }

    nrt::ModelType modelType = m_model_ptr->get_model_type();
    m_model_ptr.reset();

    if (modelType == nrt::CLASSIFICATION) {
        m_model_ptr = make_shared<nrt::Model>(mModelPath, nrt::Model::MODELIO_OUT_CAM);
    }
    else if (modelType == nrt::SEGMENTATION || modelType == nrt::DETECTION) {
        m_model_ptr = make_shared<nrt::Model>(mModelPath, nrt::Model::MODELIO_OUT_PROB);
    }
    else if(modelType == nrt::OCR) {
        m_model_ptr = make_shared<nrt::Model>(mModelPath);
    }
    else if (modelType == nrt::ANOMALY){
        m_model_ptr = make_shared<nrt::Model>(mModelPath, nrt::Model::MODELIO_OUT_ANOMALY_SCORE);
    }
    else {
        return QString("");
    }
    if (m_model_ptr->get_status() != nrt::STATUS_SUCCESS) {
        qDebug() << "Model second initialization failed.  : " << QString(nrt::get_last_error_msg()) << endl;
        return QString("");
    }

    PROB_IDX = -1; PRED_IDX = -1; CAM_IDX = -1; ANO_IDX = -1, ROT_IDX = -1; BOX_IDX = -1; // BOX 는 anomaly에서만 쓰임.
    int num_outputs = m_model_ptr->get_num_outputs();
    qDebug() << "NRT) Output Flags";
    if(modelType == nrt::ANOMALY){
        for(int i = 0; i< num_outputs; i++){
            QString output_name = m_model_ptr->get_output_name(i);
            if(output_name == "output_anomaly_score"){
                ANO_IDX = i;
                qDebug() << " - ANO IDX:" << ANO_IDX;
            }
            else if(output_name == "output_anomaly_prediction"){
                PRED_IDX = i;
                qDebug() << " - PRED IDX:" << PRED_IDX;
            }
            else if(output_name == "output_boxes_bcyxhw"){
                BOX_IDX = i;
                qDebug() << " - BOX IDX:" << BOX_IDX;
            }
        }
    }
    else{
        for (int i = 0; i < num_outputs; i++) {
            nrt::Model::ModelIOFlag output_flag = m_model_ptr->get_output_flag(i);
            if (output_flag == nrt::Model::MODELIO_OUT_PRED) {
                PRED_IDX = i;
                qDebug() << " - PRED_IDX:" << PRED_IDX;
            }
            else if (output_flag == nrt::Model::MODELIO_OUT_PROB) {
                PROB_IDX = i;
                qDebug() << " - PROB_IDX:" << PROB_IDX;
            }
            else if (output_flag == nrt::Model::MODELIO_OUT_CAM) {
                CAM_IDX = i;
                qDebug() << " - CAM_IDX:" << CAM_IDX;
            }
            else if (output_flag == nrt::Model::MODELIO_OUT_ROTATION_DEGREE) {
                ROT_IDX = i;
                qDebug() << " - ROT IDX:" << ROT_IDX;
            }
        }
    }

    // Create Executor
    qDebug() << " - fp16 flag:" << fp16_flag;
    qDebug() << "** Creating Executor.. It takes a few seconds.. **";
    int batch_size = 1;
    nrt::ExecutorConfig executor_config;
//    Specifies whether to use fp16(or Half Precision). The default value is false.
//    When set to true, if the device can use FP16 in the executor, the internal float operation is set to FP16 to improve the operation speed.
    executor_config.set_fp16flag(fp16_flag);

    if (m_executor_ptr.use_count() > 0)
        m_executor_ptr.reset();
    m_executor_ptr = make_shared<nrt::Executor>(*m_model_ptr.get(), m_device, batch_size, executor_config);

    if (m_executor_ptr->get_status() != nrt::STATUS_SUCCESS) {
        qDebug() << "Executor initialization failed. : " << QString(nrt::get_last_error_msg()) << endl;
        return QString("");
    }
    qDebug() << "NRT) Executor Created";

    set_roi_mask_info();

    return modelPath;
}

nrt::NDBuffer NrtExe::get_mask_info() {
    return m_model_ptr->get_mask_info();
}

void NrtExe::set_roi_mask_info(){
    nrt::NDBuffer roi_info = m_model_ptr->get_roi_info();
    nrt::NDBuffer mask_info = m_model_ptr->get_mask_info();
    nrt::NDBuffer org_size = m_model_ptr->get_preprocessing_input_size();

    if(!org_size.empty()){
        const int* org_size_ptr = org_size.get_at_ptr<int>();
        org_height = org_size_ptr[0];
        org_width = org_size_ptr[1];
    }
    else{
        org_height = -1;
        org_width = -1;
    }

    if(!roi_info.empty()){
        int* roi_info_ptr = roi_info.get_at_ptr<int>();
        qDebug() << "\tOriginal Height : " << roi_info_ptr[0];
        qDebug() << "\tOriginal Width : " << roi_info_ptr[1];
        qDebug() << "\tROI X : " << roi_info_ptr[2];
        qDebug() << "\tROI Y : " << roi_info_ptr[3];
        qDebug() << "\tROI Height : " << roi_info_ptr[4];
        qDebug() << "\tROI Width : " << roi_info_ptr[5];

        m_roi_info = roi_info;
    }

    if(!mask_info.empty()){
        qDebug() << "Found mask_info";
        auto mask_info_shp = mask_info.get_shape();
        qDebug() << "mask_info_shp  [";
        for (int j = 0; j < mask_info_shp.num_dim; j++)
            qDebug() << mask_info_shp.dims[j] << " ";
        qDebug() << "]";

        // 이후, input되는 이미지 사이즈 비율에 맞추어 mask를 resize 할 수 있도록 mat으로 변환하여 저장.
        cv::Mat m(mask_info_shp.dims[0], mask_info_shp.dims[1], CV_8UC1, (void*)(mask_info.get_data_ptr<unsigned char>()));
        m.copyTo(m_mask);
    }
}


int NrtExe::get_model_status() {
    return (m_model_ptr.use_count() > 0) ? m_model_ptr->get_status() : -1;
}

QString NrtExe::get_model_name() {
    if (get_model_status() != nrt::STATUS_SUCCESS)
        return QString("");
    return QString(m_model_ptr->get_model_name());
}

QString NrtExe::get_model_type() {
    if (get_model_status() != nrt::STATUS_SUCCESS)
        return QString("");

    nrt::ModelType modelType = m_model_ptr->get_model_type();
    if (modelType == nrt::NONE)
        return QString("None");
    else if (modelType == nrt::CLASSIFICATION)
        return QString("Classification");
    else if (modelType == nrt::SEGMENTATION)
        return QString("Segmentation");
    else if (modelType == nrt::DETECTION)
        return QString("Detection");
    else if (modelType == nrt::OCR)
        return QString("OCR");
    else if (modelType == nrt::ANOMALY)
        return QString("Anomaly");
    else
        return QString("Type Error");
}

QString NrtExe::get_model_training_type() {
    if (get_model_status() != nrt::STATUS_SUCCESS)
        return QString("");
    return QString(m_model_ptr->get_training_type());
}

QString NrtExe::get_model_search_level() {
    if (get_model_status() != nrt::STATUS_SUCCESS)
        return QString("");
    return (m_model_ptr->get_training_search_space_level() == -1) ? QString("Fast") : (QString("Lv") + QString::number(m_model_ptr->get_training_search_space_level()+1));
}

QString NrtExe::get_model_inference_level() {
    if (get_model_status() != nrt::STATUS_SUCCESS)
        return QString("");
    return (QString("Lv") + QString::number(m_model_ptr->get_training_inference_time_level()+1));
}

int NrtExe::get_model_class_num() {
    if (get_model_status() != nrt::STATUS_SUCCESS)
        return 0;
    return m_model_ptr->get_num_classes();
}

QString NrtExe::get_model_class_name(int class_idx) {
    if (get_model_status() != nrt::STATUS_SUCCESS)
        return QString("");
    return m_model_ptr->get_class_name(class_idx);
}

nrt::NDBuffer NrtExe::get_model_prob_threshold() {
    if (get_model_status() != nrt::STATUS_SUCCESS)
        return nrt::NDBuffer();
    return m_model_ptr->get_prob_threshold();
}

nrt::NDBuffer NrtExe::get_model_size_threshold() {
    if (get_model_status() != nrt::STATUS_SUCCESS)
        return nrt::NDBuffer();
    return m_model_ptr->get_size_threshold();
}

nrt::Shape NrtExe::get_model_input_shape(int idx) {
    if (get_model_status() != nrt::STATUS_SUCCESS)
        return nrt::Shape();
    return m_model_ptr->get_input_shape(idx);
}

nrt::DType NrtExe::get_model_input_dtype(int idx){
    if (get_model_status() != nrt::STATUS_SUCCESS)
        return nrt::DType();
    return m_model_ptr->get_input_dtype(idx);
}

nrt::InterpolationType NrtExe::get_model_interpolty(int idx){
    if (get_model_status() != nrt::STATUS_SUCCESS)
        return nrt::InterpolationType::INTER_LINEAR;
    return m_model_ptr->get_InterpolationType(idx);
}

float NrtExe::get_model_scale_factor() {
    if (get_model_status() != nrt::STATUS_SUCCESS)
        return -1;
    return m_model_ptr->get_scale_factor();
}

int NrtExe::get_model_output_num() {
    if(get_model_status() != nrt::STATUS_SUCCESS)
        return -1;
    return m_model_ptr->get_num_outputs();
}

bool NrtExe::is_model_patch_mode() {
    if (get_model_status() != nrt::STATUS_SUCCESS)
        return false;
    return m_model_ptr->is_patch_mode(0);
}

int NrtExe::get_executor_status(){
    return (m_executor_ptr.use_count() > 0) ? m_executor_ptr->get_status() : -1;
}

nrt::NDBufferList NrtExe::execute(nrt::NDBuffer resized_img_buffer) {
    nrt::NDBufferList outputs;

    nrt::Status status = m_executor_ptr->execute(resized_img_buffer, outputs);
    if(status != nrt::STATUS_SUCCESS){
        qDebug() << "prediction failed";
        return nrt::NDBufferList();
    }

    return outputs;
}
