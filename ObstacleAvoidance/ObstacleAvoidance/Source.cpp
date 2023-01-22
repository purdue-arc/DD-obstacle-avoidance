// header file
// #include "header.hh"

// other files
#include "ocpncy/ocpncy_tests.hpp"
#include "rs_tests.hpp"

// Main
int main()try {
	/*
	rs_tests::realsense_test0();

	oc_tests::occupancy_test0();
	oc_tests::occupancy_test3();
	oc_tests::occupancy_test4();
	oc_tests::occupancy_test5();
	oc_tests::occupancy_test7();

	oc_tests::generate_map_file<8>(gmtry2i::vector2i(), gmtry2i::vector2i());
	oc_tests::print_map_file_item<8>({0, 0}, 3);
	oc_tests::print_map_file_tiles<8>();

	oc_tests::template_inheritance_test();
	oc_tests::inheritance_test2();
	oc_tests::geometry_test0();
	oc_tests::geometry_test1();
	oc_tests::geometry_test2();
	*/

	oc_tests::projection_test0();
	oc_tests::projection_test1();

	return 0;
}
catch (const std::exception& e) {
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}