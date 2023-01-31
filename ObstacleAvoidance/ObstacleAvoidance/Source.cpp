// header file
//#include "header.hh"


#define OUTPUT_FILEPATH (std::string("resources/output/"))
#define RESOURCES_FILEPATH (std::string("resources/"))
// other files
#include "tests/ocpncy_tests.hpp"
#include "tests/misc_tests.hpp"
#include "tests/gmtry_tests.hpp"

// Main
int main() try {
	/*
	rs_tests::realsense_test0();

	OCCUPANCY TESTS
	oc_tests::run_all_tests();
	oc_tests::print_map_file_item<8>({ 0, 0 }, 3);
	oc_tests::occupancy_test8();
	
	GEOMETRY TESTS
	gmtry_tests::geometry_test0();
	gmtry_tests::geometry_test1();
	gmtry_tests::geometry_test2();
	gmtry_tests::geometry_test3();
	gmtry_tests::geometry_test5();
	gmtry_tests::geometry_test7();
	gmtry_tests::geometry_test8();
	gmtry_tests::projection_test0();
	gmtry_tests::projection_test1();

	A STAR TESTS (must uncomment-out 2nd line of this file)
	test_AStar("solved_maze.pbm", false, "input.pbm", true, "input.pbm", 10, 10, .9);
	*/

	gmtry_tests::projection_test2();
	return 0;
}
catch (const std::exception& e) {
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}