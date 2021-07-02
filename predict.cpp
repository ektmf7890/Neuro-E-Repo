#include "predict.h"

nrt::Status status;

int batch_size = 1;

nrt::NDBuffer get_img_buffer(cv::Mat ORG_IMG, NrtExe* m_nrt) {
    nrt::Shape input_image_shape = m_nrt->get_model_input_shape(0);
    nrt::DType input_dtype =m_nrt-> get_model_input_dtype(0);
    nrt::InterpolationType interpolty = m_nrt->get_model_interpolty(0);

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

    if(m_nrt->get_model_type() == "Segmentation")
        return image_buff;

    nrt::NDBuffer resized_img_buffer;
    status = nrt::resize(image_buff, resized_img_buffer, input_image_shape, interpolty);
    if(status != nrt::STATUS_SUCCESS) {
        qDebug() << "resize failed";
        return nrt::NDBuffer();
    }

    return resized_img_buffer;
}

nrt::NDBuffer seg_execute (nrt::NDBuffer image_buff, std::chrono::duration<double, std::milli> &inf_time, NrtExe* m_nrt){
    nrt::NDBuffer resized_image_buff;
    nrt::NDBuffer image_patch_buff;
    nrt::NDBuffer patch_info;

    nrt::NDBufferList outputs;
    nrt::NDBuffer merged_pred_output;
//    nrt::NDBuffer merged_prob_output;

    nrt::Shape input_image_shape = m_nrt->get_model_input_shape(0);
    nrt::InterpolationType interpolty = m_nrt->get_model_interpolty(0);

    bool patch_mode = m_nrt->is_model_patch_mode();
    float scale_factor = m_nrt->get_model_scale_factor();

    if(patch_mode){
        qDebug() << "Patch Mode";

        qDebug() << "Resize";
        status = nrt::resize(image_buff, resized_image_buff, scale_factor, interpolty);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "resize failed.  : " << QString(nrt::get_last_error_msg());
            return nrt::NDBuffer();
        }
//        nrt::Shape resized_buf_shape = resized_image_buff.get_shape();
//        qDebug() << "Resized image buffer: [";
//        for(int i=0; i < resized_buf_shape.num_dim; i++) {
//            qDebug() << resized_buf_shape.dims[i] << " ";
//        }

        qDebug() << "Extract";
        status = nrt::extract_patches_to_target_shape(resized_image_buff, input_image_shape, image_patch_buff, patch_info);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "extract_patches_to_target_shape failed.  : " << QString(nrt::get_last_error_msg());
            return nrt::NDBuffer();
        }

        // image_patch_buff dimensions: [num_of_patches, h, w, c]
//        nrt::Shape image_patch_buff_shape = image_patch_buff.get_shape();
//        qDebug() << "Patched image buffer: [";
//        for(int i=0; i < image_patch_buff_shape.num_dim; i++) {
//            qDebug() << image_patch_buff_shape.dims[i] << " ";
//        }
//        qDebug() << "]";

        qDebug() << "Execute";
        auto start = std::chrono::high_resolution_clock::now();
        outputs = m_nrt->execute(image_patch_buff);
        auto end = std::chrono::high_resolution_clock::now();
        inf_time = end - start;

        if(outputs.get_count() == 0) {
            qDebug() << "Execute failed" << QString(nrt::get_last_error_msg());
            return nrt::NDBuffer();
        }
        /*
        nrt::NDBuffer new_prob_output;
        if(prob_output_shape.num_dim == 4){
            int num_of_patches = prob_output_shape.dims[0];
            int height = prob_output_shape.dims[1];
            int width = prob_output_shape.dims[3];
            int channels = prob_output_shape.dims[2];

            new_prob_output = nrt::NDBuffer(nrt::Shape(num_of_patches, height, width, channels), prob_output.get_dtype());

            for(int i = 0; i < num_of_patches; i++){
                for(int j = 0; j < height; j++){
                    for(int k = 0; k < channels; k++) {
                        for(int l = 0; l < width; l++){
                            float val = *prob_output.get_at_ptr<float>(i, j, k, l);
                            float* new_prob_output_ptr = new_prob_output.get_at_ptr<float>(i, j, l, k);
                            *new_prob_output_ptr = val;
                        }
                    }
                }
            }
        }
        else{
            qDebug() << "prob_output size is invalid.";
            return vector<nrt::NDBuffer>();
        }
        nrt::Shape new_prob_output_shape = new_prob_output.get_shape();
        qDebug() << "New prob_output shape: [";
        for(int i=0; i < new_prob_output_shape.num_dim; i++) {
            qDebug() << QString::number(new_prob_output_shape.dims[i]) << " ";
        }
        qDebug() << "]";*/


        if(m_nrt->PRED_IDX != -1){
            qDebug() << "Merge patches to original shape(pred output)";
            status = nrt::merge_patches_to_orginal_shape(outputs.get_at(m_nrt->PRED_IDX), patch_info, merged_pred_output);
            if (status != nrt::STATUS_SUCCESS) {
                qDebug() << "merge_patches_to_orginal_shape failed. (pred output) : " << QString(nrt::get_last_error_msg());
                return nrt::NDBuffer();
            }
        }
        else{
            qDebug() << "No prediction map available.";
            return nrt::NDBuffer();
        }

        /*
        if(PROB_IDX != -1) {
            qDebug() << "Merge patches to original shape(prob output)";
            status = nrt::merge_patches_to_orginal_shape(outputs.get_at(PROB_IDX), patch_info, merged_prob_output);
            if (status != nrt::STATUS_SUCCESS) {
                qDebug() << "merge_patches_to_orginal_shape failed. (prob output) : " << QString(nrt::get_last_error_msg());
                return vector<nrt::NDBuffer>();;
            }
        }*/
    }

    else {
        qDebug() << "Non Patch Mode";

        qDebug() << "Resize";
        status = nrt::resize(image_buff, resized_image_buff, input_image_shape, interpolty);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "resize failed.  : " << QString(nrt::get_last_error_msg());
            return nrt::NDBuffer();
        }

        qDebug() << "Execute";
        auto start = std::chrono::high_resolution_clock::now();
        outputs = m_nrt->execute(resized_image_buff);
        auto end = std::chrono::high_resolution_clock::now();
        inf_time = end- start;

        if(outputs.get_count() == 0) {
            qDebug() << "Execute failed" << QString(nrt::get_last_error_msg());
        }

        if(m_nrt->PRED_IDX != -1)
            merged_pred_output = outputs.get_at(m_nrt->PRED_IDX);
        else{
            qDebug() << "No prediction map available.";
            return nrt::NDBuffer();
        }
        /*
        if (PROB_IDX != -1)
            merged_prob_output = outputs.get_at(PROB_IDX);
        */
    }
    return merged_pred_output;
}

