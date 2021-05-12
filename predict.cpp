#include "predict.h"

nrt::Status status;

extern int PROB_IDX, PRED_IDX, CAM_IDX, ANO_IDX;

int batch_size = 1;

nrt::NDBuffer get_img_buffer(cv::Mat ORG_IMG) {
    nrt::Shape input_image_shape = get_model_input_shape(0);
    nrt::DType input_dtype = get_model_input_dtype(0);
    nrt::InterpolationType interpolty = get_model_interpolty(0);

    int batch_size = 1;

    int input_h = ORG_IMG.rows;
    int input_w = ORG_IMG.cols;
    int input_c = ORG_IMG.channels();
    if (input_c != 3) {
        qDebug() << "The input image must be a three channel BGR image!";
        return nrt::NDBuffer();
    }
    int input_image_byte_size = input_h * input_w * input_c;

    qDebug() << "Make image_buff";
    nrt::NDBuffer image_buff(nrt::Shape(batch_size, input_h, input_w, input_c), input_dtype);

    qDebug() << "Copy";
    for (int j = 0; j < batch_size; j++) {
        unsigned char* image_buff_ptr = image_buff.get_at_ptr<unsigned char>(j);
        if (ORG_IMG.rows != input_h || ORG_IMG.cols != input_w) {
            qDebug() << "Images in a batch must all be the same size!";
            return nrt::NDBuffer();
        }
        std::copy(ORG_IMG.data, ORG_IMG.data + input_image_byte_size, image_buff_ptr);
    }

    if(get_model_type() == "Segmentation")
        return image_buff;

    nrt::NDBuffer resized_img_buffer;
    status = nrt::resize(image_buff, resized_img_buffer, input_image_shape, interpolty);
    if(status != nrt::STATUS_SUCCESS) {
        qDebug() << "resize failed";
        return nrt::NDBuffer();
    }

    return resized_img_buffer;
}

nrt::NDBuffer seg_execute (nrt::NDBuffer image_buff, std::chrono::duration<double, std::milli> &inf_time){
    nrt::NDBuffer resized_image_buff;
    nrt::NDBuffer image_patch_buff;
    nrt::NDBuffer patch_info;

    nrt::NDBufferList outputs;
    nrt::NDBuffer merged_pred_output;
//    nrt::NDBuffer merged_prob_output;

    nrt::Shape input_image_shape = get_model_input_shape(0);
    nrt::InterpolationType interpolty = get_model_interpolty(0);

    bool patch_mode = is_model_patch_mode();
    float scale_factor = get_model_scale_factor();

    if(patch_mode){
        qDebug() << "Patch Mode";

        qDebug() << "Resize";
        status = nrt::resize(image_buff, resized_image_buff, scale_factor, interpolty);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "resize failed.  : " << QString(nrt::get_last_error_msg());
            return nrt::NDBuffer();
        }
        nrt::Shape resized_buf_shape = resized_image_buff.get_shape();
        qDebug() << "Resized image buffer: [";
        for(int i=0; i < resized_buf_shape.num_dim; i++) {
            qDebug() << resized_buf_shape.dims[i] << " ";
        }

        qDebug() << "Extract";
        status = nrt::extract_patches_to_target_shape(resized_image_buff, input_image_shape, image_patch_buff, patch_info);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "extract_patches_to_target_shape failed.  : " << QString(nrt::get_last_error_msg());
            return nrt::NDBuffer();
        }

        // image_patch_buff dimensions: [num_of_patches, h, w, c]
        nrt::Shape image_patch_buff_shape = image_patch_buff.get_shape();
        qDebug() << "Patched image buffer: [";
        for(int i=0; i < image_patch_buff_shape.num_dim; i++) {
            qDebug() << image_patch_buff_shape.dims[i] << " ";
        }
        qDebug() << "]";

        qDebug() << "Execute";
        auto start = std::chrono::high_resolution_clock::now();
        outputs = execute(image_patch_buff);
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


        if(PRED_IDX != -1){
            qDebug() << "Merge patches to original shape(pred output)";
            status = nrt::merge_patches_to_orginal_shape(outputs.get_at(PRED_IDX), patch_info, merged_pred_output);
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
        status = nrt::resize(image_buff, resized_image_buff, scale_factor, interpolty);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "resize failed.  : " << QString(nrt::get_last_error_msg());
            return nrt::NDBuffer();
        }

        qDebug() << "Execute";
        auto start = std::chrono::high_resolution_clock::now();
        outputs = execute(resized_image_buff);
        auto end = std::chrono::high_resolution_clock::now();
        inf_time = start - end;

        if(outputs.get_count() == 0) {
            qDebug() << "Execute failed" << QString(nrt::get_last_error_msg());
        }

        if(PRED_IDX != -1)
            merged_pred_output = outputs.get_at(PRED_IDX);
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

/*
int predict(Mat org_img) {
    nrt::Status status;

    nrt::Shape input_image_shape = m_model_ptr->get_input_shape(0);
    nrt::DType input_dtype = m_model_ptr->get_input_dtype(0);
    nrt::InterpolationType interpolty = m_model_ptr->get_InterpolationType(0);

    nrt::NDBuffer resized_image_buff;
    nrt::NDBuffer patched_image_buff;
    nrt::NDBuffer patch_info;

    int batch_size = 1;

    outputs.clear();
    QString model_type = get_model_type();

    int input_h = org_img.rows;
    int input_w = org_img.cols;
    int input_c = org_img.channels();
    if (input_c != 3) {
        qDebug() << "The input image must be a three channel BGR image!";
        return -1;
    }
    int input_image_byte_size = input_h * input_w * input_c;

    bool patch_mode = m_model_ptr->is_patch_mode(0);
    float scale_factor = m_model_ptr->get_scale_factor();

    qDebug() << "Make imgae_buff";
    nrt::NDBuffer image_buff(nrt::Shape(batch_size, input_h, input_w, input_c), input_dtype);

    qDebug() << "Copy";
    for (int j = 0; j < batch_size; j++) {
        unsigned char* image_buff_ptr = image_buff.get_at_ptr<unsigned char>(j);
        if (org_img.rows != input_h || org_img.cols != input_w) {
            qDebug() << "Images in a batch must all be the same size!";
            return -1;
        }
        std::copy(org_img.data, org_img.data + input_image_byte_size, image_buff_ptr);
    }

    if (patch_mode) {
        qDebug() << "Patch Mode";

        nrt::NDBuffer merged_output;
        qDebug() << "Resize";
        status = nrt::resize(image_buff, resized_image_buff, scale_factor, interpolty);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "resize failed.  : " << QString(nrt::get_last_error_msg());
            return -1;
        }
        qDebug() << "Extract";
        status = nrt::extract_patches_to_target_shape(resized_image_buff, input_image_shape, patched_image_buff, patch_info);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "extract_patches_to_target_shape failed.  : " << QString(nrt::get_last_error_msg());
            return -1;
        }
        qDebug() << "Execute";
        status = m_executor_ptr->execute(patched_image_buff, outputs);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "prediction failed.  : " << QString(nrt::get_last_error_msg());
            return -1;
        }

        int num_outputs = m_model_ptr->get_num_outputs();
        int pred_idx = -1;
        for (int i = 0; i < num_outputs; i++) {
            nrt::Model::ModelIOFlag output_flag = m_model_ptr->get_output_flag(i);
            if (output_flag == nrt::Model::MODELIO_OUT_PRED)
                pred_idx = i;
        }
        if (pred_idx == -1)
            return 0;
//        auto shape = outputs.get_at(pred_idx).get_shape();
//        std::cout << "outputs [ ";
//        for (int i = 0; i < shape.num_dim; i++)
//            std::cout << shape.dims[i] << " ";
//        std::cout << " ]" << endl;

//        shape = patch_info.get_shape();
//        std::cout << "patch_info [ ";
//        for (int i = 0; i < shape.num_dim; i++)
//            std::cout << shape.dims[i] << " ";
//        std::cout << " ]" << endl;
//        int * pinfo_ptr = patch_info.get_at_ptr<int>(0);
//        std::cout << "\t0 [ ";
//        for (int i = 0; i < shape.dims[1]; i++) {
//            std::cout << pinfo_ptr[i] << " ";
//        }
//        std::cout << " ]" << endl;
//        pinfo_ptr = patch_info.get_at_ptr<int>(1);
//        std::cout << "\t1 [ ";
//        for (int i = 0; i < shape.dims[1]; i++) {
//            std::cout << pinfo_ptr[i] << " ";
//        }
//        std::cout << " ]" << endl;
        status = nrt::merge_patches_to_orginal_shape(outputs.get_at(pred_idx), patch_info, merged_output);

//        shape = merged_output.get_shape();
//        std::cout << "merged_output [ ";
//        for (int i = 0; i < shape.num_dim; i++)
//            std::cout << shape.dims[i] << " ";
//        std::cout << " ]" << endl;

        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "merge_patches_to_orginal_shape failed.  : " << QString(nrt::get_last_error_msg());
            return -1;
        }

        outputs.clear();
        qDebug() << "append";
        outputs.append(merged_output);
    }
    else {
        qDebug() << "None Patch Mode";
        status = nrt::resize(image_buff, resized_image_buff, input_image_shape, interpolty);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "resize failed.  : " << QString(nrt::get_last_error_msg());
            return -1;
        }

        outputs.clear();
        status = m_executor_ptr->execute(resized_image_buff, outputs);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "prediction failed.  : " << QString(nrt::get_last_error_msg());
            return -1;
        }
        if (outputs.get_count() < 1 || outputs.get_at(0).get_shape().num_dim !=3)
            return -1;

        auto org_image_shape = image_buff.get_shape();
        auto output_shape = outputs.get_at(0).get_shape();
        if ((model_type == "Segmentation") && ((org_image_shape.dims[1] != output_shape.dims[1]) || (org_image_shape.dims[2] != output_shape.dims[2]))) {
            nrt::Shape target_shape(output_shape.dims[0], org_image_shape.dims[1], org_image_shape.dims[2]);
            nrt::NDBuffer new_pred_output(target_shape, nrt::DTYPE_UINT8, true);

            int img_cnt = output_shape.dims[0];
            for (int cur_img = 0; cur_img < img_cnt; cur_img++) {
                unsigned char* output_ptr = outputs.get_at(0).get_at_ptr<unsigned char>(cur_img);
                unsigned char* target_ptr = new_pred_output.get_at_ptr<unsigned char>(cur_img);
                cv::Mat predmat(output_shape.dims[2], output_shape.dims[1], CV_8U, (void*)output_ptr);
                cv::Mat tgmat(org_image_shape.dims[1], org_image_shape.dims[2], CV_8U, (void*)target_ptr);

                cv::resize(predmat, tgmat, cv::Size(org_image_shape.dims[2], org_image_shape.dims[1]), 0, 0, cv::INTER_LINEAR);
            }

            outputs.clear();
            outputs.append(new_pred_output);
        }
    }
    qDebug() << "Predict Finish";

    return 0;
}
*/

