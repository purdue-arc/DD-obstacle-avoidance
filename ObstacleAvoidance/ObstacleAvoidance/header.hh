#pragma once

// Standard Libraries
#include <iostream>
#include <math.h>
#include <float.h>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex> 

// Primary Libraries
#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>

// Namespaces
using namespace std;
using namespace rs2;
using namespace cv;

// Constants
const int WIDTH = 640;
const int HEIGHT = 480;

// Function Prototypes
// utils.cpp
inline Mat frame_to_mat(frame);
inline bool device_with_streams(vector <rs2_stream>, string&);