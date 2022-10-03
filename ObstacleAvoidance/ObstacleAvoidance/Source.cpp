#include <iostream>
#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>

using namespace std;
using namespace rs2;
using namespace cv;

const int WIDTH = 640;
const int HEIGHT = 480;

Mat frame_to_mat(frame color_frame) {
	Mat color_image(Size(WIDTH, HEIGHT), CV_8UC3, (void*)color_frame.get_data(), Mat::AUTO_STEP);
	return color_image;
}

int main() {
	// creating and configuring a pipeline for rs2
	pipeline pipe;
	config config;
	config.enable_stream(RS2_STREAM_COLOR, WIDTH, HEIGHT, RS2_FORMAT_BGR8, 30);
	pipe.start(config);

	// setting up viewing window
	string window_name = "RealSense";
	namedWindow(window_name, WINDOW_AUTOSIZE);

	// getting realsense data and displaying it through opencv
	frameset frames;
	while (true) {
		frames = pipe.wait_for_frames();
		frame color_frame = frames.get_color_frame();
		Mat color_image = frame_to_mat(color_frame);
		imshow(window_name, color_image);
	}

	waitKey(0);

	return 0;
}