// Imports
#include "../header.hh";


// Tests the AStar algorithm
void test_AStar(const char *out_file_name, bool input_file_exists, const char* input_file_name, bool create_input_file, const char *new_input_file_name, int nrows, int ncols, double density) {
	// Output file
	FILE* out_file = fopen(out_file_name, "w");
	if (out_file == NULL) {
		fprintf(stderr, "There was a problem creating/writing to given output file!\n");
	}
	
	// Creating the maze
	bool** maze;
	if (input_file_exists) {
		FILE* in_file = fopen(input_file_name, "r");
		if (in_file == NULL) {
			fprintf(stderr, "Invalid input file! Please try again.\n");
		}
		maze = read_maze_file(in_file);
		fclose(in_file);
		in_file = NULL;
	}
	else if (create_input_file) {
		maze = create_maze_file(new_input_file_name, nrows, ncols, density);
	}
	else {
		maze = create_maze(nrows, ncols, density);
	}

	if (maze == NULL) {
		return;
	}
	
	// Setting up for the test
	DStar test = DStar(maze, nrows, ncols);

	// Running the test
	auto start = high_resolution_clock::now();
	vector<tuple<int, int>> path = test.generate_path();
	auto stop = high_resolution_clock::now();
	auto duration = duration_cast<microseconds>(stop - start);

	// Writing out the results
	cout << "Time taken by the program: " << duration << endl;
	if (path.size() != 0) {
		write_maze_sol(out_file, test.get_node_map(), path, nrows, ncols);
	}
	else {
		printf("No paths exist to get to the goal location.\n");
	}

	// Cleaning up resources
	fclose(out_file);
	out_file = NULL;
}
