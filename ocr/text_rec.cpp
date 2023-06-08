#include "ocr/text_rec.h"


using namespace cv;
TextRecognizer::TextRecognizer(string rootPath)
{
    string model_path_s = rootPath + "/weights/ch_PP-OCRv3_rec_infer.onnx";
    wstring model_path_es(model_path_s.begin(), model_path_s.end());
    const ORTCHAR_T* model_path = model_path_es.c_str();



    //string path =  rootPath +  "/Users/ianwong/Desktop/Screenshot-with-picture-edit/ocr/weights/ch_PP-OCRv3_rec_infer.onnx";
    //const ORTCHAR_T* model_path = L"E:/IanWong/JOB/Screenshot-with-picture-edit/ocr/weights/ch_PP-OCRv3_rec_infer.onnx";
    //const ORTCHAR_T* model_path = L"./weights/ch_PP-OCRv3_rec_infer.onnx";
    //std::wstring widestr = std::wstring(model_path.begin(), model_path.end());
	//OrtStatus* status = OrtSessionOptionsAppendExecutionProvider_CUDA(sessionOptions, 0);
	sessionOptions.SetGraphOptimizationLevel(ORT_ENABLE_BASIC);
    ort_session = new Session(env,model_path, sessionOptions);
	size_t numInputNodes = ort_session->GetInputCount();
	size_t numOutputNodes = ort_session->GetOutputCount();
	AllocatorWithDefaultOptions allocator;
	for (int i = 0; i < numInputNodes; i++)
	{
        AllocatedStringPtr intput_name_Ptr = ort_session->GetInputNameAllocated(i, allocator);
        input_names.push_back(intput_name_Ptr.get());
		Ort::TypeInfo input_type_info = ort_session->GetInputTypeInfo(i);
		auto input_tensor_info = input_type_info.GetTensorTypeAndShapeInfo();
		auto input_dims = input_tensor_info.GetShape();
		input_node_dims.push_back(input_dims);
	}
	for (int i = 0; i < numOutputNodes; i++)
	{
        AllocatedStringPtr output_name_Ptr = ort_session->GetOutputNameAllocated(i, allocator);

        output_names.push_back(output_name_Ptr.get());
		Ort::TypeInfo output_type_info = ort_session->GetOutputTypeInfo(i);
		auto output_tensor_info = output_type_info.GetTensorTypeAndShapeInfo();
		auto output_dims = output_tensor_info.GetShape();
		output_node_dims.push_back(output_dims);
	}

    string word_path = rootPath + "/weights/rec_word_dict.txt";
    wstring word_path_es(word_path.begin(), word_path.end());
    const ORTCHAR_T* word_dict_path = word_path_es.c_str();


    //const ORTCHAR_T* word_dict_path = L"E:/IanWong/JOB/Screenshot-with-picture-edit/ocr/weights/rec_word_dict.txt";
    ifstream ifs(word_dict_path);
	string line;
	while (getline(ifs, line))
	{
        //std::cout << line << endl;
        this->alphabet.push_back(line);
	}
	this->alphabet.push_back(" ");
	names_len = this->alphabet.size();
}

Mat TextRecognizer::preprocess(Mat srcimg)
{
	Mat dstimg;
	int h = srcimg.rows;
	int w = srcimg.cols;
	const float ratio = w / float(h);
	int resized_w = int(ceil((float)this->inpHeight * ratio));
//	if (ceil(this->inpHeight*ratio) > this->inpWidth)
//	{
//		resized_w = this->inpWidth;

//	}
	
	resize(srcimg, dstimg, Size(resized_w, this->inpHeight), INTER_LINEAR);
	return dstimg;
}

void TextRecognizer::normalize_(Mat img)
{
	//    img.convertTo(img, CV_32F);
	int row = img.rows;
	int col = img.cols;
    this->inpHeight = row;
    this->inpWidth = col;
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

string TextRecognizer::predict_text(Mat cv_image)
{
    //std::cout << "predict_text" << endl;
	Mat dstimg = this->preprocess(cv_image);

//    imshow("1", cv_image);
//    waitKey(0);
	this->normalize_(dstimg);

	
	array<int64_t, 4> input_shape_{ 1, 3, this->inpHeight, this->inpWidth };

	auto allocator_info = MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
	Value input_tensor_ = Value::CreateTensor<float>(allocator_info, input_image_.data(), input_image_.size(), input_shape_.data(), input_shape_.size());

	// 开始推理


    const char* input_namess[] = {"x"};
    const char* output_names[] = {"softmax_5.tmp_0"};
    vector<Value> ort_outputs = ort_session->Run(RunOptions{ nullptr }, input_namess, &input_tensor_, 1, output_names, 1);

    //vector<Value> ort_outputs = ort_session->Run(RunOptions{ nullptr }, &input_names[0], &input_tensor_, 1, output_names.data(), output_names.size());   // 开始推理
	
	const float* pdata = ort_outputs[0].GetTensorMutableData<float>();


	
	int i = 0, j = 0;
	int h = ort_outputs.at(0).GetTensorTypeAndShapeInfo().GetShape().at(2);
	int w = ort_outputs.at(0).GetTensorTypeAndShapeInfo().GetShape().at(1);
	preb_label.resize(w);
	for (i = 0; i < w; i++)
	{
		int one_label_idx = 0;
		float max_data = -10000;
		for (j = 0; j < h; j++)
		{
			float data_ = pdata[i*h + j];
			if (data_ > max_data)
			{
				max_data = data_;
				one_label_idx = j;
			}
		}
		preb_label[i] = one_label_idx;
	}

	vector<int> no_repeat_blank_label;
    string res ;
    //std::cout << "end ================" << w << endl;
	for (size_t elementIndex = 0; elementIndex < w; ++elementIndex)
	{
        if (preb_label[elementIndex] != 0 )
		{

            if(elementIndex > 0 && (preb_label[elementIndex - 1] != preb_label[elementIndex])){

                string r = alphabet[preb_label[elementIndex]-1];
                res += r;

                no_repeat_blank_label.push_back(preb_label[elementIndex] - 1);
            }

		}
	}
    return res;

//	int len_s = no_repeat_blank_label.size();

//	string plate_text;
//	for (i = 0; i < len_s; i++)
//	{

//        plate_text += alphabet[no_repeat_blank_label[i]];
//	}
//	return plate_text;
}
