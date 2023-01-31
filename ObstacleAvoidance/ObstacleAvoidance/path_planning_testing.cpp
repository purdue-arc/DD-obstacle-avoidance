// Imports
#include "header.hh";


// Function to read in maze
bool** read_maze_file(FILE *in_file) {
	int ret_val = fscanf(in_file, "P1\n");
	printf("%d\n", ret_val);
	if (ret_val < 0) {
		fprintf(stderr, "Had issues with reading file due to formatting issues.\n");
		return NULL;
	}

	int nrows, ncols;
	ret_val = fscanf(in_file, "%d %d\n", &nrows, &ncols);
	if (ret_val != 2) {
		fprintf(stderr, "Had issues with reading file due to formatting issues.\n");
		return NULL;
	}

	bool** maze = (bool**) malloc(nrows * sizeof(bool*));
	if (maze == NULL) {
		fprintf(stderr, "Problem allocating memory for the maze.\n");
		return NULL;
	}

	for (int row = 0; row < nrows; row++) {
		maze[row] = (bool*) malloc(ncols * sizeof(bool));
		if (maze[row] == NULL) {
			fprintf(stderr, "Problem allocating memory for the maze.\n");
			for (int row2 = 0; row2 < row; row++) {
				free(maze[row2]);
				maze[row2] = NULL;
			}
			return NULL;
		}

		for (int col = 0; col < ncols; col++) {
			ret_val = fscanf(in_file, "%d\n", &maze[row][col]);
			if (ret_val != 1) {
				fprintf(stderr, "Had issues with reading file due to formatting issues.\n");
				for (int row2 = 0; row2 <= row; row++) {
					free(maze[row2]);
					maze[row2] = NULL;
				}
				return NULL;
			}
		}
	}

	return maze;
}

// Writes the maze solution into a pbm file
void write_maze_sol(FILE* out_file, bool** maze, vector<tuple<int, int>> maze_sol, int nrows, int ncols) {
	fprintf(out_file, "P3\n%d %d\n255\n", nrows, ncols);
	int filled; bool part_of_sol;
	for (int row = 0; row < nrows; row++) {
		for (int col = 0; col < ncols; col++) {
			filled = 255 * maze[row][col];

			if (filled == 0) {
				part_of_sol = false;
				for (tuple<int, int> step : maze_sol) {
					printf("element\n");
					if ((get<0>(step) == row) && (get<1>(step) == col)) {
						fprintf(out_file, "255 0 0\n");
						part_of_sol = true;
						break;
					}
				}
				if (!part_of_sol) {
					fprintf(out_file, "%d %d %d\n", filled, filled, filled);
				}
			}
			else {
				fprintf(out_file, "%d %d %d\n", filled, filled, filled);
			}
		}
	}
}

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
	time_t start, end;
	AStar test = AStar(maze, nrows, ncols);
	
	// Running the test
	time(&start);
	vector<tuple<int, int>> path = test.generate_path();
	time(&end);

	// Writing out the results
	test.print_node_map(true, false, false);
	printf("Time taken by the program: %.5d seconds\n", (double) end - start);
	if (path.size() != 0) {
		write_maze_sol(out_file, maze, path, nrows, ncols);
	}
	else {
		printf("No paths exist to get to the goal location.\n");
	}

	// Cleaning up resources
	fclose(out_file); out_file = NULL;
}
