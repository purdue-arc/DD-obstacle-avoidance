// Future plan: split some things into new node class for generalization

// Prevents multiple definitions
#pragma once

// Prevents warnings for depricated libraries
#define _CRT_SECURE_NO_DEPRECATE

// Standard Libraries
#include <iostream>
#include <math.h>
#include <float.h>
#include <iomanip>
#include <chrono>
#include <thread>
#include <mutex> 
#include <malloc.h>
#include <vector>
#include <tuple>
#include <limits>

// Primary Libraries
#include <librealsense2/rs.hpp>
#include <opencv2/opencv.hpp>

// Namespaces
using namespace std;
using namespace rs2;
using namespace cv;

// Constants
#define WIDTH (640)
#define HEIGHT (480)
#define ARR_SIZE (6)
#define COST (2)


// Node struct
typedef struct node {
	int x, y;
	/*
	 * Using final bit of cost to store occupancy(1 if occupied and 0
	 * otherwise). Final bit is unused becasue the blocks are stored
	 * in units of 2. Also, since units of 2, messing with final bit
	 * will still allow correct comparision. Finally, we can use "&"
	 * to get occupancy.
	 */
	unsigned int cost;
	float heuristic;
} node_t;


// Function Prototypes
// utils.cpp
inline Mat frame_to_mat(frame);
inline bool device_with_streams(vector <rs2_stream>, string&);
bool nodeCompare(const node_t* n1, const node_t* n2);
inline bool file_exists(const char* name);

// random_maze_generator.cpp
bool **create_maze_file(const char* file_name, int nrows, int ncols);
bool** create_maze(int nrows, int ncols);
 
// path_planning_testing.cpp
void test_AStar(const char* out_file_name, bool input_file_exists, const char* input_file_name, bool create_input_file, const char* new_input_file_name, int nrows, int ncols);


// For syntax purposes
struct nodeComp {
	bool operator()(const node_t* n1, const node_t* n2) {
		return nodeCompare(n1, n2);
	}
};


// a_star.cpp: Declaring the class
class AStar {
private:
	// Attributes
	int rows, cols, start_x, start_y, goal_x, goal_y;
	bool** occ_matrix;
	node_t** node_map;
	priority_queue<node_t*, vector<node_t*>, nodeComp> pq;

protected:
	// Functions
	node_t** initialize_nodes();
	void free_nodes();
	float get_heuristic(int x_i, int y_i, int x_f, int y_f);
	bool outOfBounds(node_t* node);
	vector<node*> get_neighbors(int row, int col);
	void initializePriorityQueue();
	// make the below inline for higher efficiency
	void set_occupancy(node_t* node, bool occupied);
	bool get_occupancy(node_t* node);
	int get_cost(node_t* node);
	node_t* pop_min();
	bool compute();

public:
	// Functions
	AStar(bool** occ_matrix, int rows, int cols);
	AStar(bool** occ_matrix, int rows, int cols, int start_x, int start_y, int goal_x, int goal_y);
	vector<tuple<int, int>> generate_path();
	void print_node_map(bool occupancy, bool cost, bool heuristic);
	void debugGeneratePath();
};