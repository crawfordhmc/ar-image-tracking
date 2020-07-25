#include "SIFT_FLANN_Tracker.h"
#include <opencv2/xfeatures2d/nonfree.hpp>

SIFT_FLANN_Tracker::SIFT_FLANN_Tracker() :
	Tracker(), detector(cv::xfeatures2d::SIFT::create()), matcher(cv::FlannBasedMatcher::create()),
	targetKeyPoints(), targetDescriptors() {

}

SIFT_FLANN_Tracker::~SIFT_FLANN_Tracker() {

}

CameraPose SIFT_FLANN_Tracker::initialise(const std::string& targetFile, const cv::Mat& frame, const CameraCalibration& calibration) {

	cv::Mat target = cv::imread(targetFile);
	targetImageSize = target.size();

	// Detect features in the target image
	detector->detectAndCompute(target, cv::noArray(), targetKeyPoints, targetDescriptors);

	// Construct the matching structure
	matcher->add(targetDescriptors);
	matcher->train();

	// Since we are matching not tracking, can just use update now
	return update(frame, calibration);
}

CameraPose SIFT_FLANN_Tracker::update(const cv::Mat& frame, const CameraCalibration& calibration) {

	// Detect features in the frame
	std::vector<cv::KeyPoint> frameKeyPoints;
	cv::Mat frameDescriptors;
	detector->detectAndCompute(frame, cv::noArray(), frameKeyPoints, frameDescriptors);

	// Match to the target
	std::vector < std::vector<cv::DMatch>> rawMatches;
	matcher->knnMatch(frameDescriptors, rawMatches, 2);

	// Filter for ambiguity
	std::vector<cv::Point2f> targetPoints;
	std::vector<cv::Point2f> framePoints;
	for (auto& match : rawMatches) {
		if (match[0].distance < 0.8 * match[1].distance) {
			targetPoints.push_back(targetKeyPoints[match[0].trainIdx].pt);
			framePoints.push_back(frameKeyPoints[match[0].queryIdx].pt);
		}
	}


	CameraPose cp;
	cp.tracked = false;

	if (targetPoints.size() < 4) {
		return cp;
	}

	// Find Homography using RANSAC to exclude outliers
	std::vector<int> mask(targetPoints.size(), 0);
	cv::Mat H = cv::findHomography(targetPoints, framePoints, mask, cv::RANSAC);

	std::vector<cv::Point3f> objectPoints;
	std::vector<cv::Point2f> imagePoints;
	for (size_t i = 0; i < targetPoints.size(); ++i) {
		if (mask[i]) {
			objectPoints.push_back(cv::Point3f(targetPoints[i].x, targetPoints[i].y, 0));
			imagePoints.push_back(framePoints[i]);
		}
	}


	if (objectPoints.size() >= 4) {
		cp.tracked = cv::solvePnP(objectPoints, imagePoints, calibration.K, calibration.d, cp.rvec, cp.tvec);
	}

	return cp;
}

cv::Size SIFT_FLANN_Tracker::targetSize() {
	return targetImageSize;
}