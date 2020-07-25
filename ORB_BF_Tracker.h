#pragma once

#include "Tracker.h"

class ORB_BF_Tracker : public Tracker {

public:

	ORB_BF_Tracker();
	~ORB_BF_Tracker();

	CameraPose initialise(const std::string& targetFile, const cv::Mat& frame, const CameraCalibration& calibration);
	CameraPose update(const cv::Mat& frame, const CameraCalibration& calibration);
	cv::Size targetSize();

private:

	cv::Ptr<cv::FeatureDetector> detector;
	cv::Ptr<cv::DescriptorMatcher> matcher;
	cv::Size targetImageSize;

	std::vector<cv::KeyPoint> targetKeyPoints;
	cv::Mat targetDescriptors;

};