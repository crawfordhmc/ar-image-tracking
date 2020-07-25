#pragma once

#include <opencv2/opencv.hpp>

struct CameraPose {
	cv::Mat rvec;
	cv::Mat tvec;
	bool tracked;
};