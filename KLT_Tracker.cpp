#include "KLT_Tracker.h"
#include "SIFT_BF_Tracker.h"

KLT_Tracker::KLT_Tracker() : Tracker(), targetImageSize(), prevPoints(), targetPoints(), targetPoints2D(), prevFrame(), tFile(), frameNum(0) {

}

KLT_Tracker::~KLT_Tracker() {

}

CameraPose KLT_Tracker::initialise(const std::string& targetFile, const cv::Mat& frame, const CameraCalibration& calibration) {
	
	// Save target file to private class member for re-initialization
	tFile = targetFile;
	frameNum++;

	// Use a SIFT tracker to determine camera pose
	SIFT_BF_Tracker tempTracker;
	CameraPose pose = tempTracker.initialise(targetFile, frame, calibration);
	targetImageSize = tempTracker.targetSize();

	// Project the target corners into the image
	std::vector<cv::Point3f> targetCorners3d(4);
	std::vector<cv::Point2f> targetCorners2d(4);
	std::vector<cv::Point2f> imageCorners(4);

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
	cv::projectPoints(targetCorners3d, pose.rvec, pose.tvec, calibration.K, calibration.d, imageCorners);

	// And then a homography from the image to the target image
	cv::Mat H = cv::findHomography(imageCorners, targetCorners2d, cv::noArray(), 0);

	// Find Shi-Tomasi corners
	cv::Mat gray(frame.size(), CV_8UC1);
	cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY);
	cv::goodFeaturesToTrack(gray, imageCorners, 1000, 0.01, 3);
	
	// See which ones are in the target, and remember them for tracking
	cv::perspectiveTransform(imageCorners, targetCorners2d, H);

	for (int i = 0; i < imageCorners.size(); ++i) {
		bool inTarget =
			targetCorners2d[i].x >= 0 &&
			targetCorners2d[i].x < targetImageSize.width &&
			targetCorners2d[i].y >= 0 &&
			targetCorners2d[i].y < targetImageSize.height;

		if (inTarget) {
			prevPoints.push_back(imageCorners[i]);
			targetPoints2D.push_back(targetCorners2d[i]);
			targetPoints.push_back(cv::Point3f(targetCorners2d[i].x, targetCorners2d[i].y, 0));
		}
	}

	prevFrame = gray;
	return pose;

}

CameraPose KLT_Tracker::update(const cv::Mat& frame, const CameraCalibration& calibration) {

	CameraPose pose;
	pose.tracked = false;

	frameNum++;
	std::vector<cv::Point2f> nextPoints;
	std::vector<uchar> status;
	std::vector<float> err;

	// Convert current frame to greyscale
	cv::Mat thisFrame(frame.size(), CV_8UC1);
	cv::cvtColor(frame, thisFrame, cv::COLOR_BGR2GRAY);

	// Track features from the last frame using Lucas-Kande
	cv::calcOpticalFlowPyrLK(prevFrame, thisFrame, prevPoints, nextPoints, status, err);

	// Find Homography using RANSAC to exclude outliers
	std::vector<int> mask(targetPoints2D.size(), 0);
	cv::Mat H = cv::findHomography(targetPoints2D, nextPoints, mask, cv::RANSAC);

	std::vector<cv::Point3f> objectPoints;
	std::vector<cv::Point2f> imagePoints;
	for (size_t i = 0; i < targetPoints.size(); ++i) {
		if (status[i] && mask[i]) {
			objectPoints.push_back(targetPoints[i]);
			imagePoints.push_back(nextPoints[i]);
		}
	}

	// If at least 4 points have been successfully tracked and match the homography
	if (objectPoints.size() >= 4) {

		// Use their (remembered) target locations to estimate pose, 3dtarget
		pose.tracked = cv::solvePnP(objectPoints, imagePoints, calibration.K, calibration.d, pose.rvec, pose.tvec);

	} else {

		// Reinitialise tracking from the target image
		std::cout << "Tracking failed at frame " << frameNum << ", re-initializing" << std::endl;
		targetPoints.clear();
		targetPoints2D.clear();
		prevPoints.clear();
		return initialise(tFile, frame, calibration);
	}

	return pose;
}

cv::Size KLT_Tracker::targetSize() {
	return targetImageSize;
}