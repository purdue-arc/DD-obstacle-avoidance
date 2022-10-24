#pragma once

#include "occupancy.hpp"

namespace dstar {
	inline int ceilDiv(int a, int b) {
		return (a / b) + (a % b != 0);
	}

	inline unsigned int ceilDivByPowOf2(unsigned int a, unsigned int log2_b) {
		return (a >> log2_b) + ((a & ((1 << log2_b) - 1)) != 0);
	}

	struct queue_item {
		unsigned int k1;
		unsigned int k2;
		void* tile;
		unsigned char state_idx;
	};

	// log2_w MUST BE GREATER THAN OR EQUAL TO 4!!!
	template <unsigned int log2_w>
	struct dtile2 {
		ocpncy::btile<log2_w> occupied;
		ocpncy::btile<log2_w - 1> dangerzone; // might not need this?
		unsigned int g[1 << log2_w][1 << log2_w];
		unsigned int rhs[1 << log2_w][1 << log2_w];
		queue_item* queue_items[1 << log2_w][1 << log2_w];
		dtile2* nbrs[8];
	};


}
