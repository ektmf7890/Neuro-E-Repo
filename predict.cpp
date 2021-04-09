#include "predict.h"

nrt::Status status;

extern int PROB_IDX, PRED_IDX, CAM_IDX, ANO_IDX;

int batch_size = 1;

/*
input: ORG_IMG
output: resized_img_buffer
*/
nrt::NDBuffer cla_get_resized_img_buffer(cv::Mat ORG_IMG) {
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

    nrt::NDBuffer resized_img_buffer;
    status = nrt::resize(image_buff, resized_img_buffer, input_image_shape, interpolty);
    if(status != nrt::STATUS_SUCCESS) {
        qDebug() << "resize failed";
        return nrt::NDBuffer();
    }

    return resized_img_buffer;
}

/*
input: classification outputs
output: cla_prob_vec
*/
vector<std::string> cla_get_cls_prob_vec (nrt::NDBuffer outputs) {
    vector<float> cls_prob_vec(get_model_class_num(), 0);

    if(PROB_IDX != -1){
        output_prob = outputs.get_at(PROB_IDX);
        nrt::Shape output_prob_shape = output_prob.get_shape();

        for(int i=0; i < output_prob_shape.dims[0]; i++) {
            qDebug() << "probability value for each class : ";
            for (int j=0; j < output_prob_shape.dims[1]; j++) {
                float prob = *output_prob.get_at_ptr<float>(i, j);
                cla_prob_vec[j] = prob;
                qDebug() << get_model_class_name(i) << " - " << prob;
            }
        }
    }

    return cls_prob_vec;
}

/*
input: classification outputs
output: predicted class
*/
std::string cla_get_pred_cls (nrt::NDBuffer outputs) {
    std::string pred_cls = "";

    nrt::NDBuffer prob_thres = get_model_prob_threshold();
    if(PRED_IDX != -1) {
        output_pred = outputs.get_at(PRED_IDX);

        // Implement getting threshold value from ui dynamically
        // Add a threshold column in csv

        /*
        int pred_cla_idx;
        for (int img_idx = 0; img_idx < output_pred_shape.dims[0]; img_idx++) {
            pred_cla_idx = *output_pred.get_at_ptr<int>(img_idx);
            qDebug() << "Prediction class index(Not thresholded): " << QString::number(pred_cla_idx);
        }
        pred_cla_name = get_model_class_name(pred_cla_idx);

        int num_idx = ui->tableWidget_class->item(pred_cla_idx, NAME_COL+1)->text().size() - 1;
        float cur_prob_thres = ui->tableWidget_class->item(pred_cla_idx, NAME_COL+1)->text().left(num_idx).toFloat();
        qDebug() << ui->tableWidget_class->item(pred_cla_idx, NAME_COL+1)->text().left(num_idx) << cur_prob_thres;

        if ((cla_prob_vec[pred_cla_idx] * 100) < cur_prob_thres)
            pred_cla_name.toStdString() = String("Unknown");

        ui->edit_show_class->setText(QString(pred_cla_name));
        result_cla = pred_cla_name.toStdString();
        */
        if (prob_thres.empty()){
            float prob_thres_val = 0.8;
            status = nrt::prob_map_threshold(output_prob, prob_thres_val, thresholded_pred);
        }
        else{
            status = nrt::prob_map_threshold(output_prob, prob_thres, thresholded_pred);
        }

        if(status != nrt::STATUS_SUCCESS){
            qDebug() << "prob map threshold fail";
            return result_cls;
        }

        nrt::Shape thresholded_pred_shape = thresholded_pred.get_shape();

        for(int i=0; i<thresholded_pred_shape.dims[0]; i++){
            int cls_idx = *thresholded_pred.get_at_ptr<int>(i);
            pred_cls = (cls_idx < 0 ? get_model_class_name(cls_idx) : "Unknown" );
        }
    }

    return result_cls;
}

/*
input: classification outputs & ORG_IMG
output: PRED_IMG (CAM img overlayed on ORG_IMG)
*/
cv::Mat cla_get_pred_img(nrt::NDBuffer outputs, cv::Mat ORG_IMG){
    cv::Mat PRED_IMG;
    if (CAM_IDX != -1) {
        nrt::NDBuffer output_cam;
        nrt::NDBuffer cam_colormap;

        output_cam = outputs.get_at(CAM_IDX);
        nrt::Status status = nrt::convert_to_colormap(output_cam, cam_colormap);
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

            cv::resize(cam_colormap_mat, cam_colormap_mat, cv::Size(ORG_IMG.cols, ORG_IMG.rows), 0, 0, cv::INTER_LINEAR);
            cv::addWeighted(cam_colormap_mat, 0.4, ORG_IMG, 0.6, 0.0, PRED_IMG);
        }
    }
    return PRED_IMG;
}



/**
 * @brief
 * @param cv::Mat ORG_IMG
 * @return nrt::NDBuffer image_patch_buff
 */
nrt::NDBuffer seg_make_image_buff (cv::Mat ORG_IMG) {
    nrt::Shape input_image_shape = get_model_input_shape(0);
    nrt::DType input_dtype = m_model_ptr->get_input_dtype(0);
    nrt::InterpolationType interpolty = m_model_ptr->get_InterpolationType(0);

    bool patch_mode = is_model_patch_mode();
    float scale_factor = get_model_scale_factor();

    // max patch size
    int input_h = ORG_IMG.rows;
    int input_w = ORG_IMG.cols;
    int input_c = ORG_IMG.channels();
    int input_img_byte_size = input_h * input_w * input_c * batch_size;

    qDebug() << "Make imgae_buff";
    nrt::NDBuffer image_buff(nrt::Shape(batch_size, input_h, input_w, input_c), input_dtype);

    qDebug() << "Copy";
    for (int j = 0; j < batch_size; j++) {
        unsigned char* image_buff_ptr = image_buff.get_at_ptr<unsigned char>(j);
        if (ORG_IMG.rows != input_h || ORG_IMG.cols != input_w) {
            qDebug() << "Images in a batch must all be the same size!";
            return -1;
        }
        std::copy(ORG_IMG.data, ORG_IMG.data + input_img_byte_size, image_buff_ptr);
    }

    return image_buff;
}

nrt::NDBuffer seg_get_merged_outputs (cv::Mat ORG_IMG){
    nrt::NDBuffer image_buff = seg_make_image_buff(ORG_IMG);

    nrt::NDBuffer resized_image_buff;
    nrt::NDBuffer patched_image_buff;
    nrt::NDBuffer patch_info;
    nrt::NDBufferList outputs;
    nrt::NDBuffer merged_output;

    nrt::Shape input_image_shape = get_model_input_shape(0);
    nrt::InterpolationType interpolty = m_model_ptr->get_InterpolationType(0);

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

        qDebug() << "Extract";
        status = nrt::extract_patches_to_target_shape(resized_image_buff, input_image_shape, patched_image_buff, patch_info);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "extract_patches_to_target_shape failed.  : " << QString(nrt::get_last_error_msg());
            return nrt::NDBuffer();
        }

        qDebug() << "Execute";
        outputs = execute(patched_image_buff);
        if(outputs.get_count() == 0) {
            qDebug() << "Execute failed"; << QString(nrt::get_last_error_msg());
        }

        qDebug() << "Merge patches to original shape";
        status = nrt::merge_patches_to_orginal_shape(outputs.get_at(PRED_IDX), patch_info, merged_output);
        if (status != nrt::STATUS_SUCCESS) {
            qDebug() << "merge_patches_to_orginal_shape failed.  : " << QString(nrt::get_last_error_msg());
            return nrt::NDBuffer();
        }
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
        outputs = execute(resized_image_buff);
        if(outputs.get_count() == 0) {
            qDebug() << "Execute failed"; << QString(nrt::get_last_error_msg());
        }

        merged_output = outputs.get_at(0);
    }

    return merged_output;
}



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
