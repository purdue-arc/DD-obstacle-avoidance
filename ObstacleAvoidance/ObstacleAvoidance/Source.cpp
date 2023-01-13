// header file
// #include "header.hh"

// other files
#include "ocpncy/ocpncy_tests.hpp"

// Main
int main()try {
	// return realsense_test0();
	/*
	occupancy_test0();
	occupancy_test3();
	occupancy_test4();
	occupancy_test5();
	occupancy_test7();

	template_inheritance_test();
	geometry_test0();
	geometry_test1();

	print_map_file_tiles<8>();
	generate_map_file<8>(gmtry2i::vector2i(), gmtry2i::vector2i());
	*/

	print_map_file_item<8>({0, 0}, 3);

	return 0;
}
catch (const std::exception& e) {
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}