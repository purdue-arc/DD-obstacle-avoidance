// Prevents multiple definitions
#pragma once

// Standard Libraries
#include <iostream>
#include <math.h>
#include <float.h>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex> 
#include <malloc.h>
#include <vector>
#include <tuple>
#include <limits>

// Primary Libraries
#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>

// Internal files
// #include "compare.cpp" // DOESN'T EXIST?

// Namespaces
using namespace std;
using namespace rs2;
using namespace cv;

// Constants
#define WIDTH (640)
#define HEIGHT (480)
#define ARR_SIZE (6)
#define COST (2)

// Function Prototypes
// utils.cpp
inline Mat frame_to_mat(frame);
inline bool device_with_streams(vector <rs2_stream>, string&);