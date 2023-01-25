// header file
// #include "header.hh"

// other files
#include "tests/ocpncy_tests.hpp"
#include "tests/rs_tests.hpp"
#include "tests/misc_tests.hpp"
#include "tests/gmtry_tests.hpp"

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

	misc_tests::template_inheritance_test();
	misc_tests::inheritance_test2();
	misc_tests::inheritance_test3();
	gmtry_tests::geometry_test0();
	gmtry_tests::geometry_test1();
	gmtry_tests::geometry_test2();
	gmtry_tests::projection_test0();
	gmtry_tests::projection_test1();
	*/

	gmtry_tests::geometry_test4();

	return 0;
}
catch (const std::exception& e) {
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}