// header file
#include "header.hh"

// other files
#include "utils.cpp"

// Main
int main() {
	std::string serial;
	if (!device_with_streams({ RS2_STREAM_POSE }, serial))
		return EXIT_SUCCESS;

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