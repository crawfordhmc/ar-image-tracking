// ARSkeleton.cpp : This file contains the 'main' function. Program execution begins and ends there.
//

#include <iostream>
#include <opencv2/opencv.hpp>
#include <memory>

#include "CameraCalibration.h"
#include "Renderer.h"
#include "OpenCVBoxRenderer.h"

#include "Tracker.h"
#include "ChessboardTracker.h"
#include "SIFT_BF_Tracker.h"
#include "SIFT_FLANN_Tracker.h"
#include "ORB_BF_Tracker.h"
#include "KLT_Tracker.h"

#include "Timer.h"

void printHelpMessage(std::ostream& out) {
	out << "Usage: ARSkeleton <video source> <calibration file> <target> <tracker>" << std::endl;
	out << "  <video source> is either a device index (0 for the first camera) or a filename" << std::endl;
	out << "  <calibration file> contains intrinsic parameters" << std::endl;
	out << "  <target> gives chessboard dimensions, or an image of the target to track" << std::endl;
	out << "  <tracker> is the method used to track the target and estimate the pose:" << std::endl;
	out << "    - 'Chessboard' uses chessboard tracking" << std::endl;
	out << "    - 'SIFT_BF' uses SIFT features with BruteForce matching in each frame" << std::endl;
	out << "    - 'SIFT_FLANN' uses SIFT features with k-NN matching" << std::endl;
	out << "    - 'ORB_BF' uses ORB features with BruteForce matching in each frame" << std::endl;
	out << "    - 'KLT' uses KLT tracking from frame to frame" << std::endl;
	out << "Examples:" << std::endl;
	out << "  ARSkeleton 0 calib.txt card.jpg SIFT_BF" << std::endl;
	out << "  ARSkeleton recording.avi calib.txt chessboard.txt CHESSBOARD" << std::endl;
}

bool makeTracker(std::string trackerName, std::unique_ptr<Tracker>& tracker) {
	for (auto& c : trackerName) {
		c = toupper(c);
	}

	if (trackerName == "SIFT_BF") {
		std::cout << "Brute force SIFT tracking enabled" << std::endl;
		tracker.reset(new SIFT_BF_Tracker());
	} else if (trackerName == "KLT") {
		std::cout << "KLT tracking enabled" << std::endl;
		tracker.reset(new KLT_Tracker);
	} else if (trackerName == "ORB_BF") {
		std::cout << "Brute force ORB tracking enabled" << std::endl;
		tracker.reset(new ORB_BF_Tracker);
	} else if (trackerName == "SIFT_FLANN") {
		std::cout << "FLANN-based SIFT tracking enabled" << std::endl;
		tracker.reset(new SIFT_FLANN_Tracker);
	} else if (trackerName == "CHESSBOARD") {
		std::cout << "Chessboard tracking enabled" << std::endl;
		tracker.reset(new ChessboardTracker());
	} else {
		return false;
	}

	return true;
}

int main(int argc, char *argv[]) {

	if (argc != 5) {
		printHelpMessage(std::cerr);
		return -1;
	}

	// First argument is the video source a file (string) or camera (int)
	cv::VideoCapture cap;
	try {
		// Try to convert argument to int
		int index = std::stoi(argv[1]);
		// If it succeeds, we have a number
		cap.open(index);
	} catch (...) {
		// If conversion fails, assume it's a filename
		cap.open(argv[1]);
	}

	// Check that the capture source is open
	if (!cap.isOpened()) {
		std::cerr << "Error opening video from '" << argv[1] << "'" << std::endl;
		return -2;
	}

	// Read in the camera calibration
	CameraCalibration camCal;
	if (!camCal.read(argv[2])) {
		std::cerr << "Error reading camera calibration from '" << argv[2] << "'" << std::endl;
		return -3;
	}

	cap.set(cv::CAP_PROP_FRAME_WIDTH, camCal.imageSize.width);
	cap.set(cv::CAP_PROP_FRAME_HEIGHT, camCal.imageSize.height);

	// Set up the tracker
	std::unique_ptr<Tracker> tracker;
	if (!makeTracker(argv[4], tracker)) {
		std::cout << "Unrecognised tracker type '" << argv[4] << "'" << std::endl;
		return -5;
	}

	// Grab a few frames to let any auto brightness stuff kick in
	cv::Mat frame;
	for (int i = 0; i < 10; ++i) {
		cap >> frame;
	}

	// Intialise the camera pose
	CameraPose pose;
	pose = tracker->initialise(argv[3], frame, camCal);

	// Set up a renderer
	OpenCVBoxRenderer renderer(tracker->targetSize());

	Timer timer;
	float avg = 0;
	size_t frames = 0;

	// Start looping
	bool done = false;
	while (!done) {
		cap >> frame;
		if (frame.empty()) {
			// End of video file, or camera not giving frames
			done = true;
		}
		else {
			timer.reset();

			// Update the tracker
			pose = tracker->update(frame, camCal);
			double fps = 1.0 / timer.read();

			if (pose.tracked) {
				std::cout << fps << " frames/second" << std::endl;
				frames++;
				avg += fps;
				if (frames == 50) {
					std::cout << "AVERAGE FPS " << avg / 50 << std::endl;
				}
			}
			else {
				std::cout << "X " << fps << " frame/second" << std::endl;
			}

			// Render the result, and check for key presses
			switch (renderer.render(camCal, pose, frame)) {
			case Event::QUIT:
				done = true;
				break;
			case Event::NONE:
			default:
				break;
			};
		}
	}

	cv::destroyAllWindows();

	return 0;
}

