// Includes
#include "header.hh"


// Sets occupancy of given node inside cost through bit manipulation
inline void CNode::set_occupancy(node_t* node, bool occupied) {
	if (occupied) {
		node->cost |= 1;
	}
	else {
		node->cost &= (~1);
	}
}

// Returns occupancy of the given node from cost var.
inline bool CNode::get_occupancy(node_t* node) {
	return (node->cost & 1);
}

/*
 * Returns actual cost value from given node through.
 * Only used for print statements
 */
inline int CNode::get_cost(node_t* node) {
	return (node->cost & (~1));
}

// Given a node, this function sets the cost.
inline void CNode::set_cost(node_t* node, int cost) {
	node->cost = (cost & (~1)) | (node->cost & 1);
}

// Given a node, this function sets both the cost and occupancy.
inline void CNode::set_cost_and_occupancy(node_t* node, int cost, bool occupied) {
	node->cost = (cost & (~1)) | ((int)occupied);
}