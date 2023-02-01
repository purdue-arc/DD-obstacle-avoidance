#pragma once

// Includes
#include "header.hh";


// Compare function determines which node is a better choice to expand upon
bool nodeCompare(const node_t* n1, const node_t* n2) {
	if ((n1->heuristic + n1->cost) > (n2->heuristic + n2->cost)) {
		return true;
	}
	return false;
}

// Checks if file exists
inline bool file_exists(const char* name) {
	if (FILE* file = fopen(name, "r")) {
		fclose(file);
		return true;
	}
	else {
		return false;
	}
}

// Defining AStar Class
// Allocates memory for nodes and initializes them
node_t** AStar::initialize_nodes() {
	node_map = (node_t**)malloc(rows * sizeof(node_t*));
	for (int row = 0; row < rows; row++) {
		node_map[row] = (node_t*) malloc(cols * sizeof(node_t));
		for (int col = 0; col < cols; col++) {
			// storing x,y location
			node_map[row][col].x = row;
			node_map[row][col].y = col;

			/*
				* Floating points support infinity and math involving infinity
				* acccording to IEEE 754.
				*/
			node_map[row][col].cost = numeric_limits<int>::max();
			node_map[row][col].heuristic = numeric_limits<float>::max();
		}

		node_map[0][0].cost = 0;
	}
	return node_map;
}

// Given a node map, this function frees assosiated memory
void AStar::free_nodes() {
	for (int row = 0; row < rows; row++) {
		free(node_map[row]);
		node_map[row] = NULL;
	}
	free(node_map);
	node_map = NULL;
}

// Gets heuristic from given point to end distance
float AStar::get_heuristic(int x_i, int y_i, int x_f, int y_f) {
	return sqrt(pow((x_f - x_i), 2) + pow((y_f - y_i), 2));
}

// Checks whether node is out of bounds
bool AStar::outOfBounds(int row, int col) {
	if ((row < 0) || (col < 0) ||
		(row >= rows) || (col >= cols) ||
		(occ_matrix[row][col] == true)) {
		return true;
	}
	return false;
}

// Returns vector of neighbors for a given node
vector<node*> AStar::get_neighbors(int row, int col) {
	vector<node*> neighbors;

	// counter-clockwise starting at 12
	bool non_diag_valid[4] = {!outOfBounds(row-1, col),!outOfBounds(row, col - 1),!outOfBounds(row+1, col),!outOfBounds(row, col+1)};

	// adding non-diag neighbors
	if (non_diag_valid[0]) {
		neighbors.push_back(&node_map[row - 1][col]);
	}
	if (non_diag_valid[1]) {
		neighbors.push_back(&node_map[row][col - 1]);
	}
	if (non_diag_valid[2]) {
		neighbors.push_back(&node_map[row + 1][col]);
	}
	if (non_diag_valid[3]) {
		neighbors.push_back(&node_map[row][col + 1]);
	}

	// adding diag neighbors
	if (non_diag_valid[0] && non_diag_valid[1]) {
		neighbors.push_back(&node_map[row - 1][col - 1]);
	}
	if (non_diag_valid[1] && non_diag_valid[2]) {
		neighbors.push_back(&node_map[row + 1][col - 1]);
	}
	if (non_diag_valid[2] && non_diag_valid[3]) {
		neighbors.push_back(&node_map[row + 1][col + 1]);
	}
	if (non_diag_valid[3] && non_diag_valid[0]) {
		neighbors.push_back(&node_map[row - 1][col + 1]);
	}

	return neighbors;
}

// When called, adds all elements to a priority queue
void AStar::initializePriorityQueue() {
	// Make heap is O(n) while adding 1 @ a time is O(nlogn)
	pq.push(&node_map[0][0]);
	vector<node_t*> node_ptrs;
	for (int row = 0; row < rows; row++) {
		for (int col = 0; col < cols; col++) {
			node_ptrs.push_back(&node_map[row][col]);
		}
	}
	make_heap(node_ptrs.begin(), node_ptrs.end(), nodeCompare);
}

// ADT method to simplify popping min element
node_t* AStar::pop_min() {
	node_t* of_interest = (node_t*) pq.top();
	pq.pop();
	return of_interest;
}

// Performs necessary computations, stores them in node, & returns if path was found
bool AStar::compute() {
	bool reached_dest = false;

	node_map[start_x][start_y].cost = 0;
	node_map[start_x][start_y].heuristic = get_heuristic(start_x, start_y, goal_x, goal_y);

	//initializePriorityQueue();

	pq.push(&node_map[0][0]);

	node_t* of_interest = &node_map[0][0];
	while (!pq.empty()) {
		int parent_x = of_interest->x, parent_y = of_interest->y;

		// Checking if destination reached
		if ((parent_x == goal_x) && (parent_y == goal_y)) {
			reached_dest = true;
			break;
		}

		// Expanding neighbors
		vector<node_t*> neighbors = get_neighbors(parent_x, parent_y);
		for (int index = 0; index < neighbors.size(); index++) {
			neighbors[index]->heuristic = get_heuristic(neighbors[index]->x, neighbors[index]->y, goal_x, goal_y);
			if ((of_interest->cost + COST) <= neighbors[index]->cost) {
				neighbors[index]->cost = of_interest->cost + COST;
				pq.push(neighbors[index]);
			}
		}

		of_interest = pop_min();
	}

	return reached_dest;
}

// constructors
AStar::AStar(bool** occ_matrix, int rows, int cols) {
	// By default, setting bottom left as start and top right corner as goal
	this->occ_matrix = occ_matrix;
	start_x = 0;
	start_y = 0;
	goal_x = rows - 1;
	goal_y = cols - 1;
	this->rows = rows;
	this->cols = cols;
	node_map = initialize_nodes();
}

AStar::AStar(bool **occ_matrix, int rows, int cols, int start_x, int start_y, int goal_x, int goal_y) {
	this->occ_matrix = occ_matrix;
	this->start_x = start_x;
	this->start_y = start_y;
	this->goal_x = goal_x;
	this->goal_y = goal_y;
	this->rows = rows;
	this->cols = cols;
	this->node_map = initialize_nodes();
}

/*
	* Generates a path, which is returned as a list of tuples from start to finish.
	* To do this, it traverse backwards from goal by finding neighbor with lowest
	* cost and continue till cost = 0, in which case, it is the start node. Before
	* returning, list is reverse so that it goes from the start --> finish rather
	* than finish --> start. If no path exists, empty path is returned.
	*/
vector<tuple<int, int>> AStar::generate_path() {
	bool path_exists = compute();

	vector<tuple<int, int>> path;
	if (!path_exists) {
		return path;
	}

	int x = goal_x, y = goal_y;
	while (get_cost(&node_map[x][y]) != 0) {
		path.push_back(make_tuple(x, y));
		vector<node*> neighbors = get_neighbors(x, y);
		node_t* min = &node_map[x][y];
		for (int index = 0; index < neighbors.size(); index++) {
			if (get_cost(neighbors[index]) < get_cost(&node_map[x][y])) {
				min = neighbors[index];
			}
		}
		x = min->x;
		y = min->y;
	}

	reverse(path.begin(), path.end());
	return path;
}

void AStar::debugGeneratePath() {
	printf("Reminder:\n\"0\" --> unoccupied nodes\n\"1\" --> occupied nodes\n\"2\" --> generated path\n");
	vector<tuple<int, int>> path = generate_path();
	printf("Final Matrix:\n");
	for (int row = 0; row < rows; row++) {
		for (int col = 0; col < cols; col++) {
			bool inPath = false;
			for (tuple<int, int> node_in_path: path) {
				if ((get<0>(node_in_path) == row) && (get<1>(node_in_path))) {
					inPath = true;
					break;
				}
			}
			if (inPath) {
				printf("2, ");
			}
			else {
				printf("%d, ", occ_matrix[row][col]);
			}
		}
		printf("\n");
	}
}

// Prints specified matri(x)(ces) (occupancy, cost, or heuristic)
void AStar::print_node_map(bool occupancy, bool cost, bool heuristic) {
	if (occupancy) {
		printf("Occupancy Matrix:\n");
		for (int row = 0; row < rows; row++) {
			for (int col = 0; col < cols; col++) {
				printf("%d, ", occ_matrix[row][col]);
			}
			printf("\n");
		}
		printf("\n");
	}
	else if (cost) {
		printf("Cost Matrix:\n");
		for (int row = 0; row < rows; row++) {
			for (int col = 0; col < cols; col++) {
				printf("%d, ", get_cost(&node_map[row][col]));
			}
			printf("\n");
		}
		printf("\n");
	}
	else if (heuristic) {
		printf("Heuristic Matrix:\n");
		for (int row = 0; row < rows; row++) {
			for (int col = 0; col < cols; col++) {
				printf("%d, ", node_map[row][col].heuristic);
			}
			printf("\n");
		}
		printf("\n");
	}
	else {
		printf("Nothing specified to print!\n");
	}
}