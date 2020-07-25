#pragma once

#include "CameraPose.h"
#include "CameraCalibration.h"

class Tracker {
public:

	Tracker() = default;
	Tracker(const Tracker&) = delete;
	virtual ~Tracker() = default;

	Tracker& operator=(const Tracker&) = delete;

	virtual CameraPose initialise(const std::string& targetFile, const cv::Mat& frame, const CameraCalibration& calibration) = 0;
	virtual CameraPose update(const cv::Mat& frame, const CameraCalibration& calibration) = 0;
	virtual cv::Size targetSize() = 0;

};