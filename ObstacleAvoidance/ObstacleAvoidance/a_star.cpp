// General include
#include "header.hh"

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


// AStar Class
class AStar {
private:
	// Attributes
	int rows, cols, start_x, start_y, goal_x, goal_y;
	node_t** node_map;
	priority_queue<node_t*, vector<node_t*>, Compare> pq;

	// allocates memory for nodes and initializes them
	node_t** initialize_nodes() {
		node_map = (node_t**)malloc(rows * sizeof(node_t*));
		for (int row = 0; row < rows; row++) {
			node_map[row] = (node_t*)malloc(cols * sizeof(node_t));
			for (int col = 0; col < cols; col++) {
				// storing x,y location
				node_map[row][col].x = row;
				node_map[row][col].y = col;

				/*
				 * Floating points support infinity and math involving infinity
				 * acccording to IEEE 754.
				 */
				node_map[row][col].cost = numeric_limits<float>::max();
				node_map[row][col].heuristic = numeric_limits<float>::max();
			}
		}
		return node_map;
	}

	// Given a node map, this function frees assosiated memory
	void free_nodes() {
		for (int row = 0; row < rows; row++) {
			free(node_map[row]);
			node_map[row] = NULL;
		}
		free(node_map);
		node_map = NULL;
	}

	// Gets heuristic from given point to end distance
	float get_heuristic(int x_i, int y_i, int x_f, int y_f) {
		return sqrt(pow((x_f - x_i), 2) + pow((y_f - y_i), 2));
	}

	// Checks whether node is out of bounds
	bool outOfBounds(node_t* node) {
		if ((node->x < 0) || (node->y < 0) ||
			(node->x >= ARR_SIZE) || (node->y >= ARR_SIZE) ||
			(get_occupancy(node) == 1)) {
			return true;
		}
		return false;
	}

	// Returns vector of neighbors for a given node
	vector<node*> get_neighbors(int row, int col) {
		vector<node*> neigbors;
		// adding all possible neighbors
		neigbors.push_back(&node_map[row][col + 1]);
		neigbors.push_back(&node_map[row + 1][col + 1]);
		neigbors.push_back(&node_map[row + 1][col]);
		neigbors.push_back(&node_map[row + 1][col - 1]);
		neigbors.push_back(&node_map[row][col - 1]);
		neigbors.push_back(&node_map[row - 1][col - 1]);
		neigbors.push_back(&node_map[row - 1][col]);
		neigbors.push_back(&node_map[row - 1][col + 1]);
		remove_if(neigbors.begin(), neigbors.end(), outOfBounds);
		return neigbors;
	}

	// Compare function determines which node is a better choice to expand upon
	bool comp(const node_t* n1, const node_t* n2) {
		if ((n1->heuristic + n1->cost) > (n2->heuristic + n2->cost)) {
			return true;
		}
		return false;
	}

	// When called, adds all elements to a priority queue
	void initializePriorityQueue() {
		// Make heap is O(n) while adding 1 @ a time is O(nlogn)
		pq.push(&node_map[0][0]);
		vector<node_t*> node_ptrs;
		for (int row = 0; row < rows; row++) {
			for (int col = 0; col < cols; col++) {
				node_ptrs.push_back(&node_map[row][col]);
			}
		}
		make_heap(node_ptrs.begin(), node_ptrs.end(), comp);
	}

	// Sets occupancy of given node inside cost through bit manipulation
	inline bool set_occupancy(node_t *node, bool occupied) {
		if (occupied) {
			node->cost |= 1;
		}
		else {
			node->cost &= (~1);
		}
	}

	// Returns occupancy of the given node from cost var through bit manipulation
	inline bool get_occupancy(node_t *node) {
		return (node->cost & 1);
	}

	/*
	 * Returns actual cost value from given node through bit manipulation.
	 * Only used for print statements
	 */
	inline bool get_cost(node_t *node) {
		return (node->cost & (~1));
	}

	// ADT method to simplify popping min element
	inline node_t *pop_min() {
		node_t* of_interest = (node_t*)pq.top();
		pq.pop();
		return of_interest;
	}

	// Performs necessary computations, stores them in node, & returns if path was found
	bool compute() {
		bool reached_dest = false;

		node_map[start_x][start_y].cost = 0.0;
		node_map[start_x][start_y].heuristic = get_heuristic(start_x, start_y, goal_x, goal_y);

		initializePriorityQueue();

		node_t* of_interest = pop_min();
		while (!pq.empty()) {
			int parent_x = of_interest->x, parent_y = of_interest->y;

			// Checking if destination reached
			if ((parent_x == goal_x) && (parent_y == goal_y)) {
				reached_dest = true;
				break;
			}

			// Expanding neighbors
			vector<node_t*> neighbors = get_neighbors(parent_x, parent_y);
			for (node_t* neighbor : neighbors) {
				neighbor->heuristic = get_heuristic(neighbor->x, neighbor->y, goal_x, goal_y);
				if ((of_interest->cost + COST) < neighbor->cost) {
					neighbor->cost = of_interest->cost + COST;
					pq.push(neighbor);
				}
			}

			of_interest = pop_min();
		}

		return reached_dest;
	}

public:
	// constructors
	AStar(bool** occ_matrix, int rows, int cols) {
		// By default, setting bottom left as start and top right corner as goal
		AStar(occ_matrix, rows, cols, 0, 0, rows - 1, cols - 1);
	}

	AStar(bool **occ_matrix, int rows, int cols, int start_x, int start_y, int goal_x, int goal_y) {
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
	vector<tuple<int, int>> generate_path() {
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

	// Prints specified matri(x)(ces) (occupancy, cost, or heuristic)
	void print_node_map(bool occupancy, bool cost, bool heuristic) {
		if (occupancy) {
			printf("Occupancy Matrix:\n");
			for (int row = 0; row < rows; row++) {
				for (int col = 0; col < cols; col++) {
					printf("%d, ", get_occupancy(&node_map[row][col]));
				}
			}
			printf("\n");
		}
		else if (cost) {
			printf("Cost Matrix:\n");
			for (int row = 0; row < rows; row++) {
				for (int col = 0; col < cols; col++) {
					printf("%d, ", get_cost(&node_map[row][col]));
				}
			}
			printf("\n");
		}
		else if (heuristic) {
			printf("Heuristic Matrix:\n");
			for (int row = 0; row < rows; row++) {
				for (int col = 0; col < cols; col++) {
					printf("%d, ", node_map[row][col].heuristic);
				}
				
			}
			printf("\n");
		}
		else {
			printf("Nothing specified to print!\n");
		}
	}
};