#include <iostream>
#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace rs2;
using namespace cv;

int main() {
	namedWindow("image", WINDOW_AUTOSIZE);
	cout << "Open cv is working!";
	pipeline pipe;
	cout << "rs is working!";
	return 0;
}