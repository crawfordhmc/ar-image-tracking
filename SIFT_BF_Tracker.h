#pragma once

#include "Tracker.h"

class SIFT_BF_Tracker : public Tracker {

public:

	SIFT_BF_Tracker();
	~SIFT_BF_Tracker();

	CameraPose initialise(const std::string& targetFile, const cv::Mat& frame, const CameraCalibration& calibration);
	CameraPose update(const cv::Mat& frame, const CameraCalibration& calibration);
	cv::Size targetSize();

private:

	cv::Ptr<cv::FeatureDetector> detector;
	cv::Ptr<cv::DescriptorMatcher> matcher;
	cv::Size targetImageSize;
	std::vector<cv::KeyPoint> targetKeyPoints;
	cv::Mat targetDescriptors;
	cv::Rect roi;
	int frameNum;

};