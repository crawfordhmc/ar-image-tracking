#include "CameraCalibration.h"

CameraCalibration::CameraCalibration() :
K(cv::Mat::eye(3,3,CV_64F)), d(5,0), imageSize(0,0) {

}

CameraCalibration::CameraCalibration(const CameraCalibration& camCal) :
K(camCal.K.clone()), d(camCal.d), imageSize(camCal.imageSize) {

}

CameraCalibration::~CameraCalibration() {

}

CameraCalibration& CameraCalibration::operator=(const CameraCalibration& camCal) {
	if (this != &camCal) {
		K = camCal.K.clone();
		d = camCal.d;
		imageSize = camCal.imageSize;
	}
	return *this;
}

bool CameraCalibration::read(const std::string& filename) {
	std::ifstream fin;
	fin.open(filename);
	if (!fin.is_open()) {
		return false;
	}

	fin >> imageSize.width >> imageSize.height;

	for (int row = 0; row < 3; ++row) {
		for (int col = 0; col < 3; ++col) {
			fin >> K.at<double>(row, col);
		}
	}

	size_t numCoefficients;
	fin >> numCoefficients;
	d.resize(numCoefficients);
	for (size_t i = 0; i < numCoefficients; ++i) {
		fin >> d[i];
	}
	fin.close();

	return true;
}