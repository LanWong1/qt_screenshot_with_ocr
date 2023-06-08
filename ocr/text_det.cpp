#include"ocr/text_det.h"


using namespace cv;
TextDetector::TextDetector(string rootPath)
{
	this->binaryThreshold = 0.3;
	this->polygonThreshold = 0.5;
	this->unclipRatio = 1.6;
	this->maxCandidates = 1000;
    string model_path_s = rootPath + "/weights/ch_PP-OCRv3_det_infer.onnx";
    wstring model_path_es(model_path_s.begin(), model_path_s.end());
    const ORTCHAR_T* model_path = model_path_es.c_str();
    //const ORTCHAR_T* model_path = L"E:/IanWong/JOB/Screenshot-with-picture-edit/ocr/weights/ch_PP-OCRv3_det_infer.onnx";
    //const ORTCHAR_T* model_path = L"./weights/ch_PP-OCRv3_det_infer.onnx";
    //std::wstring widestr = std::wstring(model_path.begin(), model_path.end());
	//OrtStatus* status = OrtSessionOptionsAppendExecutionProvider_CUDA(sessionOptions, 0);  ////gpu
	sessionOptions.SetGraphOptimizationLevel(ORT_ENABLE_BASIC);
    net = new Session(env, model_path, sessionOptions);
	size_t numInputNodes = net->GetInputCount();
	size_t numOutputNodes = net->GetOutputCount();
	AllocatorWithDefaultOptions allocator;
	for (int i = 0; i < numInputNodes; i++)
	{
        AllocatedStringPtr intput_name_Ptr = net->GetInputNameAllocated(i, allocator);
        //cout << "input name" << intput_name_Ptr.get();
        input_names.push_back(intput_name_Ptr.get());
	}
	for (int i = 0; i < numOutputNodes; i++)
	{
        AllocatedStringPtr output_name_Ptr = net->GetInputNameAllocated(i, allocator);
        output_names.push_back(output_name_Ptr.get());
	}
}

Mat TextDetector::preprocess(Mat srcimg)
{
	Mat dstimg;
	cvtColor(srcimg, dstimg, COLOR_BGR2RGB);
	int h = srcimg.rows;
	int w = srcimg.cols;
	float scale_h = 1;
	float scale_w = 1;
	if (h < w)
	{
		scale_h = (float)this->short_size / (float)h;
		float tar_w = (float)w * scale_h;
		tar_w = tar_w - (int)tar_w % 32;
		tar_w = max((float)32, tar_w);
		scale_w = tar_w / (float)w;
	}
	else
	{
		scale_w = (float)this->short_size / (float)w;
		float tar_h = (float)h * scale_w;
		tar_h = tar_h - (int)tar_h % 32;
		tar_h = max((float)32, tar_h);
		scale_h = tar_h / (float)h;
	}
	resize(dstimg, dstimg, Size(int(scale_w*dstimg.cols), int(scale_h*dstimg.rows)), INTER_LINEAR);

	return dstimg;
}

void TextDetector::normalize_(Mat img)
{
	//    img.convertTo(img, CV_32F);
	int row = img.rows;
	int col = img.cols;
	this->input_image_.resize(row * col * img.channels());
	for (int c = 0; c < 3; c++)
	{
		for (int i = 0; i < row; i++)
		{
			for (int j = 0; j < col; j++)
			{
				float pix = img.ptr<uchar>(i)[j * 3 + c];
				this->input_image_[c * row * col + i * col + j] = (pix / 255.0 - this->meanValues[c]) / this->normValues[c];
			}
		}
	}
}

vector< vector<Point2f> > TextDetector::detect(Mat& srcimg)
{

	int h = srcimg.rows;
	int w = srcimg.cols;
	Mat dstimg = this->preprocess(srcimg);

	this->normalize_(dstimg);

	array<int64_t, 4> input_shape_{ 1, 3, dstimg.rows, dstimg.cols };
	
	auto allocator_info = MemoryInfo::CreateCpu(OrtDeviceAllocator, OrtMemTypeCPU);
	Value input_tensor_ = Value::CreateTensor<float>(allocator_info, input_image_.data(), input_image_.size(), input_shape_.data(), input_shape_.size());
    OrtValue* output_tensor = NULL;

    const char* input_namess[] = {"x"};
    const char* output_names[] = {"sigmoid_0.tmp_0"};
    vector<Value> ort_outputs = net->Run(RunOptions{ nullptr }, input_namess, &input_tensor_, 1, output_names, 1);

	const float* floatArray = ort_outputs[0].GetTensorMutableData<float>();
	int outputCount = 1;
	for(int i=0; i < ort_outputs.at(0).GetTensorTypeAndShapeInfo().GetShape().size(); i++)
	{
		int dim = ort_outputs.at(0).GetTensorTypeAndShapeInfo().GetShape().at(i);
		outputCount *= dim;
	}

	Mat binary(dstimg.rows, dstimg.cols, CV_32FC1);
	memcpy(binary.data, floatArray, outputCount * sizeof(float));

	// Threshold
	Mat bitmap;
	threshold(binary, bitmap, binaryThreshold, 255, THRESH_BINARY);

	// Scale ratio
	float scaleHeight = (float)(h) / (float)(binary.size[0]);
	float scaleWidth = (float)(w) / (float)(binary.size[1]);
	// Find contours
	vector< vector<Point> > contours;
	bitmap.convertTo(bitmap, CV_8UC1);
	findContours(bitmap, contours, RETR_LIST, CHAIN_APPROX_SIMPLE);

	// Candidate number limitation
	size_t numCandidate = min(contours.size(), (size_t)(maxCandidates > 0 ? maxCandidates : INT_MAX));
	vector<float> confidences;
	vector< vector<Point2f> > results;
	for (size_t i = 0; i < numCandidate; i++)
	{
		vector<Point>& contour = contours[i];

		// Calculate text contour score
		if (contourScore(binary, contour) < polygonThreshold)
			continue;

		// Rescale
        vector<Point> contourScaled;
        contourScaled.reserve(contour.size());
		for (size_t j = 0; j < contour.size(); j++)
		{
			contourScaled.push_back(Point(int(contour[j].x * scaleWidth),
				int(contour[j].y * scaleHeight)));
		}

		// Unclip
		RotatedRect box = minAreaRect(contourScaled);
		float longSide = std::max(box.size.width, box.size.height);
		if (longSide < longSideThresh) 
		{
			continue;
		}

		// minArea() rect is not normalized, it may return rectangles with angle=-90 or height < width
		const float angle_threshold = 60;  // do not expect vertical text, TODO detection algo property
		bool swap_size = false;
		if (box.size.width < box.size.height)  // horizontal-wide text area is expected
			swap_size = true;
		else if (fabs(box.angle) >= angle_threshold)  // don't work with vertical rectangles
			swap_size = true;
		if (swap_size)
		{
			swap(box.size.width, box.size.height);
			if (box.angle < 0)
				box.angle += 90;
			else if (box.angle > 0)
				box.angle -= 90;
		}

		Point2f vertex[4];
		box.points(vertex);  // order: bl, tl, tr, br
		vector<Point2f> approx;
		for (int j = 0; j < 4; j++)
			approx.emplace_back(vertex[j]);
		vector<Point2f> polygon;
		unclip(approx, polygon);

		box = minAreaRect(polygon);
		longSide = std::max(box.size.width, box.size.height);
		if (longSide < longSideThresh+2)
		{
			continue;
		}
		results.push_back(polygon);
	}
	confidences = vector<float>(contours.size(), 1.0f);
	return results;
	/*vector< vector<Point2f> > order_points = this->order_points_clockwise(results);
	return order_points;*/
}

vector< vector<Point2f> > TextDetector::order_points_clockwise(vector< vector<Point2f> > results)
{
	vector< vector<Point2f> > order_points(results);
	for (int i = 0; i < results.size(); i++)
	{
		float max_sum_pts = -10000;
		float min_sum_pts = 10000;
		float max_diff_pts = -10000;
		float min_diff_pts = 10000;

		int max_sum_pts_id = 0;
		int min_sum_pts_id = 0;
		int max_diff_pts_id = 0;
		int min_diff_pts_id = 0;
		for (int j = 0; j < 4; j++)
		{
			const float sum_pt = results[i][j].x + results[i][j].y;
			if (sum_pt > max_sum_pts)
			{
				max_sum_pts = sum_pt;
				max_sum_pts_id = j;
			}
			if (sum_pt < min_sum_pts)
			{
				min_sum_pts = sum_pt;
				min_sum_pts_id = j;
			}

			const float diff_pt = results[i][j].y - results[i][j].x;
			if (diff_pt > max_diff_pts)
			{
				max_diff_pts = diff_pt;
				max_diff_pts_id = j;
			}
			if (diff_pt < min_diff_pts)
			{
				min_diff_pts = diff_pt;
				min_diff_pts_id = j;
			}
		}
		order_points[i][0].x = results[i][min_sum_pts_id].x;
		order_points[i][0].y = results[i][min_sum_pts_id].y;
		order_points[i][2].x = results[i][max_sum_pts_id].x;
		order_points[i][2].y = results[i][max_sum_pts_id].y;

		order_points[i][1].x = results[i][min_diff_pts_id].x;
		order_points[i][1].y = results[i][min_diff_pts_id].y;
		order_points[i][3].x = results[i][max_diff_pts_id].x;
		order_points[i][3].y = results[i][max_diff_pts_id].y;
	}
	return order_points;
}

void TextDetector::draw_pred(Mat& srcimg, vector< vector<Point2f> > results)
{
	for (int i = 0; i < results.size(); i++)
	{
		for (int j = 0; j < 4; j++)
		{
			circle(srcimg, Point((int)results[i][j].x, (int)results[i][j].y), 2, Scalar(0, 0, 255), -1);
			if (j < 3)
			{
				line(srcimg, Point((int)results[i][j].x, (int)results[i][j].y), Point((int)results[i][j + 1].x, (int)results[i][j + 1].y), Scalar(0, 255, 0));
			}
			else
			{
				line(srcimg, Point((int)results[i][j].x, (int)results[i][j].y), Point((int)results[i][0].x, (int)results[i][0].y), Scalar(0, 255, 0));
			}
		}
	}
}

float TextDetector::contourScore(const Mat& binary, const vector<Point>& contour)
{
	Rect rect = boundingRect(contour);
	int xmin = max(rect.x, 0);
	int xmax = min(rect.x + rect.width, binary.cols - 1);
	int ymin = max(rect.y, 0);
	int ymax = min(rect.y + rect.height, binary.rows - 1);

	Mat binROI = binary(Rect(xmin, ymin, xmax - xmin + 1, ymax - ymin + 1));

	Mat mask = Mat::zeros(ymax - ymin + 1, xmax - xmin + 1, CV_8U);
	vector<Point> roiContour;
	for (size_t i = 0; i < contour.size(); i++) {
		Point pt = Point(contour[i].x - xmin, contour[i].y - ymin);
		roiContour.push_back(pt);
	}
	vector<vector<Point>> roiContours = { roiContour };
	fillPoly(mask, roiContours, Scalar(1));
	float score = mean(binROI, mask).val[0];
	return score;
}

void TextDetector::unclip(const vector<Point2f>& inPoly, vector<Point2f> &outPoly)
{
	float area = contourArea(inPoly);
	float length = arcLength(inPoly, true);
	float distance = area * unclipRatio / length;

	size_t numPoints = inPoly.size();
	vector<vector<Point2f>> newLines;
	for (size_t i = 0; i < numPoints; i++)
	{
		vector<Point2f> newLine;
		Point pt1 = inPoly[i];
		Point pt2 = inPoly[(i - 1) % numPoints];
		Point vec = pt1 - pt2;
		float unclipDis = (float)(distance / norm(vec));
		Point2f rotateVec = Point2f(vec.y * unclipDis, -vec.x * unclipDis);
		newLine.push_back(Point2f(pt1.x + rotateVec.x, pt1.y + rotateVec.y));
		newLine.push_back(Point2f(pt2.x + rotateVec.x, pt2.y + rotateVec.y));
		newLines.push_back(newLine);
	}

	size_t numLines = newLines.size();
	for (size_t i = 0; i < numLines; i++)
	{
		Point2f a = newLines[i][0];
		Point2f b = newLines[i][1];
		Point2f c = newLines[(i + 1) % numLines][0];
		Point2f d = newLines[(i + 1) % numLines][1];
		Point2f pt;
		Point2f v1 = b - a;
		Point2f v2 = d - c;
		float cosAngle = (v1.x * v2.x + v1.y * v2.y) / (norm(v1) * norm(v2));

		if (fabs(cosAngle) > 0.7)
		{
			pt.x = (b.x + c.x) * 0.5;
			pt.y = (b.y + c.y) * 0.5;
		}
		else
		{
			float denom = a.x * (float)(d.y - c.y) + b.x * (float)(c.y - d.y) +
				d.x * (float)(b.y - a.y) + c.x * (float)(a.y - b.y);
			float num = a.x * (float)(d.y - c.y) + c.x * (float)(a.y - d.y) + d.x * (float)(c.y - a.y);
			float s = num / denom;

			pt.x = a.x + s * (b.x - a.x);
			pt.y = a.y + s * (b.y - a.y);
		}
		outPoly.push_back(pt);
	}
}

Mat TextDetector::get_rotate_crop_image(const Mat& frame, vector<Point2f> vertices)
{
	Rect rect = boundingRect(Mat(vertices));
	Mat crop_img = frame(rect);

	const Size outputSize = Size(rect.width, rect.height);

	vector<Point2f> targetVertices{ Point2f(0, outputSize.height),Point2f(0, 0), Point2f(outputSize.width, 0), Point2f(outputSize.width, outputSize.height)};
	//vector<Point2f> targetVertices{ Point2f(0, 0), Point2f(outputSize.width, 0), Point2f(outputSize.width, outputSize.height), Point2f(0, outputSize.height) };

	for (int i = 0; i < 4; i++)
	{
		vertices[i].x -= rect.x;
		vertices[i].y -= rect.y;
	}
	
	Mat rotationMatrix = getPerspectiveTransform(vertices, targetVertices);
	Mat result;
	warpPerspective(crop_img, result, rotationMatrix, outputSize, cv::BORDER_REPLICATE);
    return result;
}

Mat TextDetector::GetRotateCropImage(const cv::Mat &srcimage, std::vector<Point2f> box) {
  cv::Mat image;
  srcimage.copyTo(image);
  std::vector<Point2f> points = box;

  float x_collect[4] = {box[0].x, box[1].x, box[2].x, box[3].x};
  float y_collect[4] = {box[0].y, box[1].y, box[2].y, box[3].y};
  float left = int(*std::min_element(x_collect, x_collect + 4));
  float right = int(*std::max_element(x_collect, x_collect + 4));
  float top = int(*std::min_element(y_collect, y_collect + 4));
  float bottom = int(*std::max_element(y_collect, y_collect + 4));

  cv::Mat img_crop;
  image(cv::Rect(left, top, right - left, bottom - top)).copyTo(img_crop);

  for (int i = 0; i < points.size(); i++) {
    points[i].x -= left;
    points[i].y -= top;
  }

  int img_crop_width = int(sqrt(pow(points[0].x - points[1].x, 2) +
                                pow(points[0].y - points[1].y, 2)));
  int img_crop_height = int(sqrt(pow(points[0].x - points[3].x, 2) +
                                 pow(points[0].y - points[3].y, 2)));

  cv::Point2f pts_std[4];
  pts_std[0] = cv::Point2f(0., 0.);
  pts_std[1] = cv::Point2f(img_crop_width, 0.);
  pts_std[2] = cv::Point2f(img_crop_width, img_crop_height);
  pts_std[3] = cv::Point2f(0.f, img_crop_height);

  cv::Point2f pointsf[4];
  pointsf[0] = cv::Point2f(points[0].x, points[0].y);
  pointsf[1] = cv::Point2f(points[1].x, points[1].y);
  pointsf[2] = cv::Point2f(points[2].x, points[2].y);
  pointsf[3] = cv::Point2f(points[3].x, points[3].y);

  cv::Mat M = cv::getPerspectiveTransform(pointsf, pts_std);

  cv::Mat dst_img;
  cv::warpPerspective(img_crop, dst_img, M,
                      cv::Size(img_crop_width, img_crop_height),
                      cv::BORDER_REPLICATE);

  if (float(dst_img.rows) >= float(dst_img.cols) * 1.5) {
    cv::Mat srcCopy = cv::Mat(dst_img.rows, dst_img.cols, dst_img.depth());
    cv::transpose(dst_img, srcCopy);
    cv::flip(srcCopy, srcCopy, 0);
    return srcCopy;
  } else {
    return dst_img;
  }
}


