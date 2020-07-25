#pragma once

#include "Tracker.h"

class KLT_Tracker : public Tracker {

public:

	KLT_Tracker();
	~KLT_Tracker();

	CameraPose initialise(const std::string& targetFile, const cv::Mat& frame, const CameraCalibration& calibration);
	CameraPose update(const cv::Mat& frame, const CameraCalibration& calibration);
	cv::Size targetSize();

private:

	cv::Size targetImageSize;
	std::vector<cv::Point2f> prevPoints;
	std::vector<cv::Point3f> targetPoints;
	std::vector<cv::Point2f> targetPoints2D;
	cv::Mat prevFrame;
	std::string tFile;
	int frameNum;

};