#define _CRT_SECURE_NO_WARNINGS
#include <iostream>
#include <opencv2/imgproc.hpp>
#include <opencv2/highgui.hpp>
//#include <cuda_provider_factory.h>
#include <onnxruntime_cxx_api.h>

//using namespace cv;
using namespace std;
using namespace Ort;

class TextDetector
{
public:
    TextDetector(string rootPath);
    vector< vector<cv::Point2f> > detect(cv::Mat& srcimg);
    void draw_pred(cv::Mat& srcimg, vector< vector<cv::Point2f> > results);
    cv::Mat get_rotate_crop_image(const cv::Mat& frame, vector<cv::Point2f> vertices);
    cv::Mat GetRotateCropImage(const cv::Mat &srcimage, std::vector<cv::Point2f> box);
private:
	float binaryThreshold;
	float polygonThreshold;
	float unclipRatio;
	int maxCandidates;
    const int longSideThresh = 3;//minBox
	const int short_size = 736;
	const float meanValues[3] = { 0.485, 0.456, 0.406 };
	const float normValues[3] = { 0.229, 0.224, 0.225 };
    float contourScore(const cv::Mat& binary, const vector<cv::Point>& contour);
    void unclip(const vector<cv::Point2f>& inPoly, vector<cv::Point2f> &outPoly);
    vector< vector<cv::Point2f> > order_points_clockwise(vector< vector<cv::Point2f> > results);
    cv::Mat preprocess(cv::Mat srcimg);
	vector<float> input_image_;
    void normalize_(cv::Mat img);

	Session *net;
	Env env = Env(ORT_LOGGING_LEVEL_ERROR, "DBNet");
	SessionOptions sessionOptions = SessionOptions();
	vector<char*> input_names;
	vector<char*> output_names;
}; 
