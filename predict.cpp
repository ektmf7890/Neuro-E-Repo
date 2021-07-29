#include "predict.h"

nrt::Status status;

int batch_size = 1;

nrt::NDBuffer get_img_buffer(cv::Mat ORG_IMG, NrtExe* nrt_ptr) {
    nrt::Shape input_image_shape = nrt_ptr->get_model_input_shape(0);
    nrt::DType input_dtype = nrt_ptr-> get_model_input_dtype(0);
    nrt::InterpolationType interpolty = nrt_ptr->get_model_interpolty(0);

    int batch_size = 1;

    int input_h = ORG_IMG.rows;
    int input_w = ORG_IMG.cols;
    int input_c = ORG_IMG.channels();
    if (input_c != 3) {
        qDebug() << "The input image must be a three channel BGR image!";
        return nrt::NDBuffer();
    }
    int input_image_byte_size = input_h * input_w * input_c;

    nrt::NDBuffer image_buff(nrt::Shape(batch_size, input_h, input_w, input_c), input_dtype);
    for (int j = 0; j < batch_size; j++) {
        unsigned char* image_buff_ptr = image_buff.get_at_ptr<unsigned char>(j);
        if (ORG_IMG.rows != input_h || ORG_IMG.cols != input_w) {
            qDebug() << "Images in a batch must all be the same size!";
            return nrt::NDBuffer();
        }
        std::copy(ORG_IMG.data, ORG_IMG.data + input_image_byte_size, image_buff_ptr);
    }

    nrt::NDBuffer roi_info;
    nrt::NDBuffer mask_info;
    double h_ratio = (double)nrt_ptr->org_height / ORG_IMG.rows;
    double w_ratio = (double)nrt_ptr->org_width / ORG_IMG.cols;
    if(!nrt_ptr->m_roi_info.empty()){
        const int* roi_info_ptr = nrt_ptr->m_roi_info.get_at_ptr<int>();

        nrt_ptr->resized_roi_x = (double)roi_info_ptr[2] / w_ratio;
        nrt_ptr->resized_roi_y = (double)roi_info_ptr[3] / h_ratio;
        nrt_ptr->resized_roi_w = (double)roi_info_ptr[5] / w_ratio;
        nrt_ptr->resized_roi_h = (double)roi_info_ptr[4] / h_ratio;

        roi_info = nrt::NDBuffer::make_roi_info(ORG_IMG.rows, ORG_IMG.cols, nrt_ptr->resized_roi_x, nrt_ptr->resized_roi_y, nrt_ptr->resized_roi_h, nrt_ptr->resized_roi_w);
    }

    if(!nrt_ptr->m_mask.empty()){
        cv::Mat mask = nrt_ptr->m_mask.clone();

        int mask_height = (double)mask.rows / h_ratio;
        int mask_width = (double)mask.cols / w_ratio;
        int mask_byte_size = mask_height * mask_width * 1;

        cv::resize(mask, mask, cv::Size(mask_width, mask_height), 0, 0, cv::INTER_LINEAR);

        mask_info = nrt::NDBuffer(nrt::Shape(mask_height, mask_width, 1), nrt::DTYPE_UINT8);
        unsigned char* mask_info_ptr = mask_info.get_at_ptr<unsigned char>();
        std::copy(mask.data, mask.data + mask_byte_size, mask_info_ptr);
    }

    if(!roi_info.empty() && !mask_info.empty()){
        nrt::NDBuffer roimasked_img_buff;
        status = nrt::set_roi_mask(image_buff, roimasked_img_buff, roi_info, mask_info);
        if(status != nrt::STATUS_SUCCESS){
            qDebug() << "set roi mask failed: " << nrt::get_last_error_msg();
            return nrt::NDBuffer();
        }

        QString model_type = nrt_ptr->get_model_type();
        if(model_type == "Segmentation" || (model_type == "OCR" && nrt_ptr->ROT_IDX != -1)){
            return roimasked_img_buff;
        }

        nrt::NDBuffer resized_img_buffer;
        status = nrt::resize(roimasked_img_buff, resized_img_buffer, input_image_shape, interpolty);
        if(status != nrt::STATUS_SUCCESS) {
            qDebug() << "resize failed";
            return nrt::NDBuffer();
        }

        return resized_img_buffer;
    }

    else{
        QString model_type = nrt_ptr->get_model_type();
        if(model_type == "Segmentation" || (model_type == "OCR" && nrt_ptr->ROT_IDX != -1)){
            return image_buff;
        }

        nrt::NDBuffer resized_img_buffer;
        status = nrt::resize(image_buff, resized_img_buffer, input_image_shape, interpolty);
        if(status != nrt::STATUS_SUCCESS) {
            qDebug() << "resize failed";
            return nrt::NDBuffer();
        }

        return resized_img_buffer;
    }
}

QVector<nrt::NDBuffer> seg_execute (nrt::NDBuffer image_buff, std::chrono::duration<double, std::milli> &inf_time, NrtExe* nrt_ptr){
    nrt::NDBuffer resized_image_buff;
    nrt::NDBuffer image_patch_buff;
    nrt::NDBuffer patch_info;

    nrt::NDBufferList outputs;
    nrt::NDBuffer merged_pred_output;
    nrt::NDBuffer merged_prob_output;

    nrt::Shape input_image_shape = nrt_ptr->get_model_input_shape(0);
    nrt::InterpolationType interpolty = nrt_ptr->get_model_interpolty(0);

    bool patch_mode = nrt_ptr->is_model_patch_mode();
    float scale_factor = nrt_ptr->get_model_scale_factor();

    if(patch_mode){
        status = nrt::resize(image_buff, resized_image_buff, scale_factor, interpolty);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "resize failed.  : " << QString(nrt::get_last_error_msg());
            return QVector<nrt::NDBuffer>();
        }

        status = nrt::extract_patches_to_target_shape(resized_image_buff, input_image_shape, image_patch_buff, patch_info);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "extract_patches_to_target_shape failed.  : " << QString(nrt::get_last_error_msg());
            return QVector<nrt::NDBuffer>();
        }

        auto start = std::chrono::high_resolution_clock::now();
        outputs = nrt_ptr->execute(image_patch_buff);
        auto end = std::chrono::high_resolution_clock::now();
        inf_time = end - start;

        if(outputs.get_count() == 0) {
            qDebug() << "Execute failed" << QString(nrt::get_last_error_msg());
            return QVector<nrt::NDBuffer>();
        }

        if(nrt_ptr->PRED_IDX != -1){
            status = nrt::merge_patches_to_orginal_shape(outputs.get_at(nrt_ptr->PRED_IDX), patch_info, merged_pred_output);
            if (status != nrt::STATUS_SUCCESS) {
                qDebug() << "merge_patches_to_orginal_shape failed. (pred output) : " << QString(nrt::get_last_error_msg());
                return QVector<nrt::NDBuffer>();
            }
        }
        else{
            qDebug() << "No prediction map available.";
            return QVector<nrt::NDBuffer>();
        }

        if(nrt_ptr->PROB_IDX != -1) {
            status = nrt::merge_patches_to_orginal_shape(outputs.get_at(nrt_ptr->PROB_IDX), patch_info, merged_prob_output);
            if (status != nrt::STATUS_SUCCESS) {
                qDebug() << "merge_patches_to_orginal_shape failed. (prob output) : " << QString(nrt::get_last_error_msg());
                return QVector<nrt::NDBuffer>();;
            }
        }
        else{
            qDebug() << "No prob map available.";
            return QVector<nrt::NDBuffer>();
        }
    }

    else {

        status = nrt::resize(image_buff, resized_image_buff, input_image_shape, interpolty);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "resize failed.  : " << QString(nrt::get_last_error_msg());
            return QVector<nrt::NDBuffer>();
        }

        auto start = std::chrono::high_resolution_clock::now();
        outputs = nrt_ptr->execute(resized_image_buff);
        auto end = std::chrono::high_resolution_clock::now();
        inf_time = end- start;

        if(outputs.get_count() == 0) {
            qDebug() << "Execute failed" << QString(nrt::get_last_error_msg());
        }

        if(nrt_ptr->PRED_IDX != -1)
            merged_pred_output = outputs.get_at(nrt_ptr->PRED_IDX);
        else{
            qDebug() << "No prediction map available.";
            return QVector<nrt::NDBuffer>();
        }

        if(nrt_ptr->PROB_IDX != -1) {
            merged_prob_output = outputs.get_at(nrt_ptr->PROB_IDX);
        }
        else{
            qDebug() << "No prob map available.";
            return QVector<nrt::NDBuffer>();
        }
    }
    return {merged_pred_output, merged_prob_output};
}

