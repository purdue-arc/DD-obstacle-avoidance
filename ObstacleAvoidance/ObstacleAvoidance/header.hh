// Prevents multiple definitions
#pragma once

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

// Global Structs
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

// a_star.cpp
class AStar {
private:
	node_t** initialize_nodes();
	void free_nodes();
	float get_heuristic(int x_i, int y_i, int x_f, int y_f);
	bool outOfBounds(node_t* node);
	vector<node*> get_neighbors(int row, int col);
	void initializePriorityQueue();
	inline bool set_occupancy(node_t* node, bool occupied);
	inline bool get_occupancy(node_t* node);
	inline node_t* pop_min();
	bool compute();

public:
	AStar(bool** occ_matrix, int rows, int cols);
	AStar(bool** occ_matrix, int rows, int cols, int start_x, int start_y, int goal_x, int goal_y);
	vector<tuple<int, int>> generate_path();
	void print_node_map(bool occupancy, bool cost, bool heuristic);
};

// for syntax purposes
struct nodeComp {
	bool operator()(const node_t* n1, const node_t* n2) {
		return nodeCompare(n1, n2);
	}
};