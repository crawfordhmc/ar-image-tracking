#include "OpenCVBoxRenderer.h"

OpenCVBoxRenderer::OpenCVBoxRenderer(const  cv::Size& targetSize) {
	cv::namedWindow("Render");
	noKey = cv::waitKey(10);

	float w = targetSize.width;
	float h = targetSize.height;
	float d = (w + h) / 4;
	box.resize(8);
	box[0] = cv::Point3f(0, 0, 0);
	box[1] = cv::Point3f(0, 0, -d);
	box[2] = cv::Point3f(0, h, 0);
	box[3] = cv::Point3f(0, h, -d);
	box[4] = cv::Point3f(w, 0, 0);
	box[5] = cv::Point3f(w, 0, -d);
	box[6] = cv::Point3f(w, h, 0);
	box[7] = cv::Point3f(w, h, -d);

}

OpenCVBoxRenderer::~OpenCVBoxRenderer() {
	cv::destroyWindow("Render");
}

Event OpenCVBoxRenderer::render(const CameraCalibration& calibration, const CameraPose& pose, const cv::Mat& image) {
	
	cv::Mat display(image);
	
	if (pose.tracked) {
		std::vector<cv::Point2f> proj(8);
		cv::projectPoints(box, pose.rvec, pose.tvec, calibration.K, calibration.d, proj);

		cv::line(display, proj[0], proj[1], cv::Scalar(255, 0, 0), 5, cv::LineTypes::LINE_AA);
		cv::line(display, proj[0], proj[2], cv::Scalar(0, 255, 0), 5, cv::LineTypes::LINE_AA);
		cv::line(display, proj[0], proj[4], cv::Scalar(0, 0, 255), 5, cv::LineTypes::LINE_AA);
		cv::line(display, proj[1], proj[3], cv::Scalar(255, 255, 255), 5, cv::LineTypes::LINE_AA);
		cv::line(display, proj[1], proj[5], cv::Scalar(255, 255, 255), 5, cv::LineTypes::LINE_AA);
		cv::line(display, proj[2], proj[3], cv::Scalar(255, 255, 255), 5, cv::LineTypes::LINE_AA);
		cv::line(display, proj[2], proj[6], cv::Scalar(255, 255, 255), 5, cv::LineTypes::LINE_AA);
		cv::line(display, proj[3], proj[7], cv::Scalar(255, 255, 255), 5, cv::LineTypes::LINE_AA);
		cv::line(display, proj[4], proj[5], cv::Scalar(255, 255, 255), 5, cv::LineTypes::LINE_AA);
		cv::line(display, proj[4], proj[6], cv::Scalar(255, 255, 255), 5, cv::LineTypes::LINE_AA);
		cv::line(display, proj[5], proj[7], cv::Scalar(255, 255, 255), 5, cv::LineTypes::LINE_AA);
		cv::line(display, proj[6], proj[7], cv::Scalar(255, 255, 255), 5, cv::LineTypes::LINE_AA);
		cv::line(display, proj[1], proj[3], cv::Scalar(0, 0, 0), 2, cv::LineTypes::LINE_AA);
		cv::line(display, proj[1], proj[5], cv::Scalar(0, 0, 0), 2, cv::LineTypes::LINE_AA);
		cv::line(display, proj[2], proj[3], cv::Scalar(0, 0, 0), 2, cv::LineTypes::LINE_AA);
		cv::line(display, proj[2], proj[6], cv::Scalar(0, 0, 0), 2, cv::LineTypes::LINE_AA);
		cv::line(display, proj[3], proj[7], cv::Scalar(0, 0, 0), 2, cv::LineTypes::LINE_AA);
		cv::line(display, proj[4], proj[5], cv::Scalar(0, 0, 0), 2, cv::LineTypes::LINE_AA);
		cv::line(display, proj[4], proj[6], cv::Scalar(0, 0, 0), 2, cv::LineTypes::LINE_AA);
		cv::line(display, proj[5], proj[7], cv::Scalar(0, 0, 0), 2, cv::LineTypes::LINE_AA);
		cv::line(display, proj[6], proj[7], cv::Scalar(0, 0, 0), 2, cv::LineTypes::LINE_AA);
	}

	cv::imshow("Render", display);
	int keyPressed = cv::waitKey(10);
	
	if (keyPressed == noKey) {
		return Event::NONE;
	} else if (keyPressed == ' ') {
		cv::imwrite("snap.png", display);
		return Event::SNAP;
	} else if (keyPressed == 'Q' || keyPressed == 'q') {
		return Event::QUIT;
	}

	return Event::NONE;
}
