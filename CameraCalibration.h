#pragma once

#include <opencv2/opencv.hpp>
#include <vector>

struct CameraCalibration {
	cv::Mat K;
	std::vector<double> d;
	cv::Size imageSize;

	CameraCalibration();
	CameraCalibration(const CameraCalibration& camCal);
	~CameraCalibration();

	CameraCalibration& operator=(const CameraCalibration& camCal);

	bool read(const std::string& filename);
};