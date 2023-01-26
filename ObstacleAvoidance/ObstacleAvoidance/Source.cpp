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

	return 0;
}
catch (const std::exception& e) {
	std::cerr << e.what() << std::endl;
	return EXIT_FAILURE;
}