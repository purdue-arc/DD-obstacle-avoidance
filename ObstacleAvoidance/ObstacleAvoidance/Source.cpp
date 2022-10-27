// header file
#include "header.hh"

// other files
#include "utils.cpp"
#include "occupancy.hpp"

int realsense_test0() {
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

int occupancy_test0() {
	try {
		ocpncy::bmap_fstream<3> mapstream("mymap", gmtry2i::vector2i(4, 5));
		ocpncy::btile<3> mytile = { { {
				(0b00000000ULL << 48) +
				(0b00100100ULL << 40) +
				(0b00100100ULL << 32) +
				(0b00000000ULL << 24) +
				(0b01000010ULL << 16) +
				(0b00111100ULL << 8) +
				(0b00000000ULL << 0)
		} } };
		ocpncy::PrintTile(mytile);
		std::cout << mapstream.write(gmtry2i::vector2i(5, 4), &mytile) << std::endl;

		ocpncy::btile<3> mytile_read;
		std::cout << mapstream.read(gmtry2i::vector2i(5, 4), &mytile_read) << std::endl;
		// This print should look the same as the last
		PrintTile(mytile_read);
		gmtry2i::aligned_box2i mapbox = mapstream.get_bounds();
		std::cout << mapbox.min.x << ", " << mapbox.min.y << "; " << mapbox.max.x << ", " << mapbox.max.y << std::endl;
	}
	catch (int i) {
		std::cout << i << std::endl;
	}

	return 0;
}

int occupancy_test1() {
	FILE* file;
	fopen_s(&file, "mymap", "r");
	if (file == 0) return -1;

	ocpncy::bmap_info<3> info;
	fseek(file, 0, SEEK_SET);
	fread(&info, 1, sizeof(info), file);
	std::cout << "Recorded log2_tile_w: " << info.log2_tile_w << std::endl;
	std::cout << "Recorded depth: " << info.depth << std::endl;
	std::cout << "Recorded origin: " << info.origin.x << ", " << info.origin.y << std::endl;

	fclose(file);
	return 0;
}

// Main
int main() {
	// return realsense_test0();
	return occupancy_test0();
}