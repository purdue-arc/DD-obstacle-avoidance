// Includes
#include "header.hh";

// Private Define
#define MAX_MAP_BIT_VAL (2)

// Initializes random seed
void init_random_bit() {
  srand((unsigned int) time);
}

// Gets a random bit
inline char random_bit() {
  const int r = (char) rand() % MAX_MAP_BIT_VAL;
  return r;
}

// Functions
bool **create_maze_file(const char* file_name, int nrows, int ncols) {
    FILE* out_file = fopen(file_name, "w");
    if (out_file == NULL) {
        fprintf(stderr, "Problem creating/opening output file to write map into.\n");
        return NULL;
    }

    bool** maze = create_maze(nrows, ncols);

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


bool **create_maze(int nrows, int ncols) {
    int i, j;
    init_random_bit();

    printf("Map Shape: \n%d %d\n", nrows, ncols);

    // Creates a maze array
    bool** maze = (bool **) malloc(nrows * sizeof(bool *));
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
            maze[row][col] = random_bit();
        }
    }

    return maze;
}
