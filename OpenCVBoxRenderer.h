#pragma once

#include "Renderer.h"

class OpenCVBoxRenderer : public Renderer {

public:
	OpenCVBoxRenderer(const cv::Size& targetSize);
	~OpenCVBoxRenderer();

	Event render(const CameraCalibration& calibration, const CameraPose& pose, const cv::Mat& image);

private:
	int noKey;
	std::vector<cv::Point3f> box;
};