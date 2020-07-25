cmake_minimum_required(VERSION 2.8)
set(CMAKE_CXX_STANDARD 14)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

project( Assignment1 )

set ( OpenCV_DIR /usr/local/Cellar/opencv/4.2.0_1/) 
find_package( OpenCV REQUIRED )

add_executable( Assignment1
Assignment1Main.cpp
CameraCalibration.cpp
CameraCalibration.h
CameraPose.h
ChessboardTracker.cpp
ChessboardTracker.h
KLT_Tracker.cpp
KLT_Tracker.h
OpenCVBoxRenderer.cpp
OpenCVBoxRenderer.h
ORB_BF_Tracker.cpp
ORB_BF_Tracker.h
Renderer.h
SIFT_BF_Tracker.cpp
SIFT_BF_Tracker.h
SIFT_FLANN_Tracker.cpp
SIFT_FLANN_Tracker.h
Timer.cpp
Timer.h
Tracker.h )

target_link_libraries( Assignment1 ${OpenCV_LIBS} )
