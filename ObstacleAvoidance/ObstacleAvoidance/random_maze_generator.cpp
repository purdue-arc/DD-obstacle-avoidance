// Includes
#include "header.hh";

// Private Define
#define MAX_MAP_BIT_VAL (2)
#define CLUSTER_SIZE (9)

// Initializes random seed
void init_random_bit() {
  srand((unsigned int) time);
}

// Gets a random bit
inline int random_bit(double density) {
  const int r = rand() / (int) (((double) RAND_MAX) * density);
  printf("%d\n", r);
  return r;
}

// Gets a random point in a r x c maze
int* random_point(int nrows, int ncols) {
	int *point = (int *) malloc(2 * sizeof(int));
	point[0] = rand() % nrows;
	point[1] = rand() % ncols;

	return point;
}

// Functions
bool **create_maze_file(const char* file_name, int nrows, int ncols, double density) {
    FILE* out_file = fopen(file_name, "w");
    if (out_file == NULL) {
        fprintf(stderr, "Problem creating/opening output file to write map into.\n");
        return NULL;
    }

    bool** maze = create_maze(nrows, ncols, density);

    // Creates a .pbm file
    fprintf(out_file, "P1\n%d %d\n", nrows, ncols);
    for (int row = 0; row < nrows; row++) {
        for (int col = 0; col < ncols; col++) {
            fprintf(out_file, "%c\n", '0' + maze[row][col]);
        }
    }

    fclose(out_file);
    out_file = NULL;

    return maze;
}


bool **create_maze(int nrows, int ncols, double density) {
    init_random_bit();

    printf("Map Shape: \n%d %d\n", nrows, ncols);

    // Creates a maze array
    bool** maze = (bool **) allocate_2d_arr(nrows, ncols, sizeof(bool));
    if (maze == NULL) {
        fprintf(stderr, "Problem allocating memory for the maze.\n");
        return NULL;
    }

    for (int row = 0; row < nrows; row++) {
        for (int col = 0; col < ncols; col++) {
            maze[row][col] = random_bit(density);
        }
    }

    return maze;
}

bool** create_clustered_maze(int nrows, int ncols) {
	init_random_bit();

	printf("Map Shape: \n%d %d\n", nrows, ncols);

	// Creates a maze array
	bool** maze = (bool**) allocate_2d_arr(nrows, ncols, sizeof(bool));
	if (maze == NULL) {
		fprintf(stderr, "Problem allocating memory for the maze.\n");
		return NULL;
	}

	int maxClusters = ceil((nrows * ncols) / CLUSTER_SIZE);
	int operations[CLUSTER_SIZE - 1][2] = { {0, -1}, {0, 1}, {1, 0}, {-1, 0}, {-1, -1}, {-1, 1}, {1, -1}, {1, 1} };

	int numClusters = 0;
	int numIters = 0;
	while (numClusters < maxClusters && numIters++ < nrows) {
		int* clusterPoint = random_point(nrows, ncols);
		int centerX = clusterPoint[0];
		int centerY = clusterPoint[1];

		if (maze[centerX][centerY]) {
			continue;
		}

		maze[centerX][centerY] = 1;
		for (int j = 0; j < CLUSTER_SIZE - 1; j++) {
			int x = operations[j][0];
			int y = operations[j][1];
			int newX = centerX + x;
			int newY = centerY + y;

			if ((newX < 0 && newX >= nrows) && (newY < 0 && newY >= ncols)) {
				continue;
			}

			maze[newX][newY] = 1;
		}

		numClusters++;
		numIters = 0;
	}

	for (int row = 0; row < nrows; row++) {
		for (int col = 0; col < ncols; col++) {
			maze[row][col] = maze[row][col] || 0;
		}
	}

	return maze;
}

// Function to read in maze
bool** read_maze_file(FILE* in_file) {
	int ret_val = fscanf(in_file, "P1\n");
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

	bool** maze = (bool **) allocate_2d_arr(nrows, ncols, sizeof(bool));
	if (maze == NULL) {
		fprintf(stderr, "Problem allocating memory for the maze.\n");
		return NULL;
	}

	for (int row = 0; row < nrows; row++) {
		for (int col = 0; col < ncols; col++) {
			ret_val = fscanf(in_file, "%d\n", &maze[row][col]);
			maze[row][col] = (maze[row][col] == 0) ? 1 : 0;
			if (ret_val != 1) {
				fprintf(stderr, "Had issues with reading file due to formatting issues.\n");
				free_2d_arr((void**)maze);
				maze = NULL;
				return NULL;
			}
		}
	}

	return maze;
}

// Writes the maze solution into a pbm file
void write_maze_sol(FILE* out_file, node_t** node_map, vector<tuple<int, int>> path, int nrows, int ncols) {
	fprintf(out_file, "P3\n%d %d\n255\n", nrows, ncols);
	int filled; bool part_of_sol;
	for (int row = 0; row < nrows; row++) {
		for (int col = 0; col < ncols; col++) {
			filled = 255 * CNode::get_occupancy(&node_map[row][col]);

			if (filled == 0) {
				part_of_sol = false;
				for (tuple<int, int> step : path) {
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