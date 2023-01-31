// header file
// #include "header.hh"


#define OUTPUT_FILEPATH (std::string("resources/output/"))
#define RESOURCES_FILEPATH (std::string("resources/"))
// other files
#include "tests/ocpncy_tests.hpp"
#include "tests/rs_tests.hpp"
#include "tests/misc_tests.hpp"
#include "tests/gmtry_tests.hpp"

// Main
int main()try {
	/*
	rs_tests::realsense_test0();

	oc_tests::run_all_tests();
	oc_tests::print_map_file_item<8>({ 0, 0 }, 3);
	oc_tests::occupancy_test8();

	misc_tests::template_inheritance_test();
	misc_tests::inheritance_test2();
	misc_tests::inheritance_test3();
	gmtry_tests::geometry_test0();
	gmtry_tests::geometry_test1();
	gmtry_tests::geometry_test2();
	gmtry_tests::geometry_test3();
	gmtry_tests::geometry_test5();
	gmtry_tests::geometry_test7();
	gmtry_tests::projection_test0();
	gmtry_tests::projection_test1();
	*/

	gmtry_tests::projection_test2();
	gmtry_tests::geometry_test8();
	test_AStar("solved_maze.pbm", false, "input.pbm", true, "input.pbm", 10, 10, .9);
	test_AStar("solved_maze.pbm", false, "input.pbm", true, "input.pbm", 10, 10, .9);
	return 0;
catch (const std::exception& e) {
	std::cerr << e.what() << std::endl;
	std::cerr << "RealSense error calling " << e.get_failed_function() << "(" << e.get_failed_args() << "):\n    " << e.what() << std::endl;
	return EXIT_FAILURE;
}