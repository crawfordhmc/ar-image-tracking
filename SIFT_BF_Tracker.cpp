#include "SIFT_BF_Tracker.h"
#include <opencv2/xfeatures2d/nonfree.hpp>

SIFT_BF_Tracker::SIFT_BF_Tracker() :
Tracker(), detector(cv::xfeatures2d::SIFT::create()), matcher(cv::BFMatcher::create()),
targetKeyPoints(), targetDescriptors(), roi(0, 0, 0, 0), frameNum(0) {

}

SIFT_BF_Tracker::~SIFT_BF_Tracker() {

}

CameraPose SIFT_BF_Tracker::initialise(const std::string& targetFile , const cv::Mat& frame, const CameraCalibration& calibration) {

	cv::Mat target = cv::imread(targetFile);
	targetImageSize = target.size();
	// Set bounding box initially to whole frame size
	roi.width = frame.size().width;
	roi.height = frame.size().height;

	// Detect features in the target image
	detector->detectAndCompute(target, cv::noArray(), targetKeyPoints, targetDescriptors);

	// Construct the matching structure
	matcher->add(targetDescriptors);
	matcher->train();

	// Since we are matching not tracking, can just use update now
	return update(frame, calibration);
}

CameraPose SIFT_BF_Tracker::update(const cv::Mat& frame, const CameraCalibration& calibration) {
	
	// Apply bounding box
	cv::Mat restricter(frame.size(), CV_8UC1, cv::Scalar::all(0));
	restricter(roi).setTo(cv::Scalar::all(255));

	// Code to display bounded frame
	/*if (++frameNum >= 2) {
		cv::imshow(std::to_string(frameNum), restricter);
		cv::waitKey(0);
		cv::destroyWindow(std::to_string(frameNum));
	}*/

	// Detect features in the frame
	std::vector<cv::KeyPoint> frameKeyPoints;
	cv::Mat frameDescriptors;
	
	detector->detectAndCompute(frame, restricter, frameKeyPoints, frameDescriptors);
	
	// Match to the target
	std::vector < std::vector<cv::DMatch>> rawMatches;
	matcher->knnMatch(frameDescriptors, rawMatches, 2);
	
	// Filter for ambiguity
	std::vector<cv::Point2f> targetPoints;
	std::vector<cv::Point2f> framePoints;
	for (auto& match : rawMatches) {
		if (match[0].distance < 0.8*match[1].distance) {
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

	// Set up points for bounding box in next frame
	std::vector<cv::Point2f> imageCorners(4);
	std::vector<cv::Point3f> targetCorners3d(4);
	std::vector<cv::Point2f> targetCorners2d(4);

	float w = targetImageSize.width;
	float h = targetImageSize.height;

	targetCorners3d[0] = cv::Point3f(0, 0, 0);
	targetCorners3d[1] = cv::Point3f(w, 0, 0);
	targetCorners3d[2] = cv::Point3f(w, h, 0);
	targetCorners3d[3] = cv::Point3f(0, h, 0);
	targetCorners2d[0] = cv::Point2f(0, 0);
	targetCorners2d[1] = cv::Point2f(w, 0);
	targetCorners2d[2] = cv::Point2f(w, h);
	targetCorners2d[3] = cv::Point2f(0, h);

	// Find 2D image co-ordinates of the target corners
	cv::projectPoints(targetCorners3d, cp.rvec, cp.tvec, calibration.K, calibration.d, imageCorners);

	// Update bounding box from frame corners
	float leftX = imageCorners[0].x;
	float topY = imageCorners[0].y;
	for (int i = 1; i < 4; ++i) {
		leftX = (imageCorners[i].x < leftX) ? imageCorners[i].x : leftX;
		topY = (imageCorners[i].y < topY) ? imageCorners[i].y : topY;
	}

	float width = imageCorners[0].x - leftX;
	float height = imageCorners[0].y - topY;
	for (int i = 1; i < 4; ++i) {
		width = (imageCorners[i].x - leftX > width) ? imageCorners[i].x - leftX : width;
		height = (imageCorners[i].y - topY > height) ? imageCorners[i].y - topY : height;
	}

	// Extend bounding box to allow for movement
	float wFactor = width / 10;
	float hFactor = height / 10;
	roi.x = (0 < leftX - wFactor) ? leftX - wFactor : 0;
	roi.y = (0 < topY - hFactor) ? topY - hFactor : 0;
	roi.width = (frame.size().width > roi.x + width + 2*wFactor) ? width + 2*wFactor : frame.size().width - roi.x;
	roi.height = (frame.size().height > roi.y + height + 2*hFactor) ? height + 2*hFactor : frame.size().height - roi.y;

	return cp;
}

cv::Size SIFT_BF_Tracker::targetSize() {
	return targetImageSize;
}