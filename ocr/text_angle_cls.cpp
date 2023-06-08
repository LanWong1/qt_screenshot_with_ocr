#include "ocr/text_angle_cls.h"


using namespace cv;
TextClassifier::TextClassifier(string rootPath)
{
    ///cout << "===========================" << rootPath;
    //string model_path = "ocr/weights/ch_ppocr_mobile_v2.0_cls_train.onnx";

    string model_path_s = rootPath + "/weights/ch_ppocr_mobile_v2.0_cls_train.onnx";
    wstring model_path_es(model_path_s.begin(), model_path_s.end());
    const ORTCHAR_T* model_path = model_path_es.c_str();

    //const ORTCHAR_T* model_path = L"E:/IanWong/JOB/Screenshot-with-picture-edit/ocr/weights/ch_ppocr_mobile_v2.0_cls_train.onnx";
    //const ORTCHAR_T* model_path = L"./weights/ch_ppocr_mobile_v2.0_cls_train.onnx";
    //std::wstring widestr = std::wstring(model_path.begin(), model_path.end());
	//OrtStatus* status = OrtSessionOptionsAppendExecutionProvider_CUDA(sessionOptions, 0);
	sessionOptions.SetGraphOptimizationLevel(ORT_ENABLE_BASIC);
    ort_session = new Session(env, model_path, sessionOptions);
	size_t numInputNodes = ort_session->GetInputCount();
	size_t numOutputNodes = ort_session->GetOutputCount();
	AllocatorWithDefaultOptions allocator;
	for (int i = 0; i < numInputNodes; i++)
	{
        //input_names.push_back(ort_session->GetInputName(i, allocator));
        AllocatedStringPtr intput_name_Ptr = ort_session->GetInputNameAllocated(i, allocator);

        input_names.push_back(intput_name_Ptr.get());

		Ort::TypeInfo input_type_info = ort_session->GetInputTypeInfo(i);
		auto input_tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
		auto input_dims = input_tensor_info.GetShape();
		input_node_dims.push_back(input_dims);
	}
	for (int i = 0; i < numOutputNodes; i++)
	{
        //output_names.push_back(ort_session->GetOutputName(i, allocator));
        AllocatedStringPtr output_name_Ptr = ort_session->GetOutputNameAllocated(i, allocator);

        output_names.push_back(output_name_Ptr.get());
		Ort::TypeInfo output_type_info = ort_session->GetOutputTypeInfo(i);
		auto output_tensor_info = output_type_info.GetTensorTypeAndShapeInfo();
		auto output_dims = output_tensor_info.GetShape();
		output_node_dims.push_back(output_dims);
	}
	num_out = output_node_dims[0][1];
}

Mat TextClassifier::preprocess(Mat srcimg)
{
	Mat dstimg;
	int h = srcimg.rows;
	int w = srcimg.cols;
	const float ratio = w / float(h);
	int resized_w = int(ceil((float)this->inpHeight * ratio));
	if (ceil(this->inpHeight*ratio) > this->inpWidth)
	{
		resized_w = this->inpWidth;
	}

	resize(srcimg, dstimg, Size(resized_w, this->inpHeight), INTER_LINEAR);
	return dstimg;
}

void TextClassifier::normalize_(Mat img)
{
	//    img.convertTo(img, CV_32F);
	int row = img.rows;
	int col = img.cols;
	this->input_image_.resize(this->inpHeight * this->inpWidth * img.channels());
	for (int c = 0; c < 3; c++)
	{
		for (int i = 0; i < row; i++)
		{
			for (int j = 0; j < inpWidth; j++)
			{
				if (j < col)
				{
					float pix = img.ptr<uchar>(i)[j * 3 + c];
					this->input_image_[c * row * inpWidth + i * inpWidth + j] = (pix / 255.0 - 0.5) / 0.5;
				}
				else
				{
					this->input_image_[c * row * inpWidth + i * inpWidth + j] = 0;
				}
			}
		}
	}
}

int TextClassifier::predict(Mat cv_image)
{
	Mat dstimg = this->preprocess(cv_image);
	this->normalize_(dstimg);
	array<int64_t, 4> input_shape_{ 1, 3, this->inpHeight, this->inpWidth };

	auto allocator_info = MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
	Value input_tensor_ = Value::CreateTensor<float>(allocator_info, input_image_.data(), input_image_.size(), input_shape_.data(), input_shape_.size());

	// 开始推理
    const char* input_namess[] = {"x"};
    const char* output_names[] = {"save_infer_model/scale_0.tmp_1"};
    vector<Value> ort_outputs = ort_session->Run(RunOptions{ nullptr }, input_namess, &input_tensor_, 1, output_names, 1);
    //vector<Value> ort_outputs = ort_session->Run(RunOptions{ nullptr }, &input_names[0], &input_tensor_, 1, output_names.data(), output_names.size());   // 开始推理
	const float* pdata = ort_outputs[0].GetTensorMutableData<float>();
	
	int max_id = 0;
	float max_prob = -1;
	for (int i = 0; i < num_out; i++)
	{
		if (pdata[i] > max_prob)
		{
			max_prob = pdata[i];
			max_id = i;
		}
	}

	return max_id;
}
