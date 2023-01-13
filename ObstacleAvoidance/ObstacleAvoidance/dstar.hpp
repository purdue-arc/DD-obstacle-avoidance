#pragma once

#include "ocpncy/occupancy.hpp"
#include "maps2/tilemaps.hpp"

namespace dstar {
	inline int ceilDiv(int a, int b) {
		return (a / b) + (a % b != 0);
	}

	inline unsigned int ceilDivByPowOf2(unsigned int a, unsigned int log2_b) {
		return (a >> log2_b) + ((a & ((1 << log2_b) - 1)) != 0);
	}

	typedef unsigned int dcosttype;

	template <unsigned int log2_w>
	struct dtile2 : public maps2::nbrng_tile<ocpncy::btile<log2_w>> {
		dcosttype g[1 << log2_w][1 << log2_w];
		dcosttype rhs[1 << log2_w][1 << log2_w];
		void* queue_items[1 << log2_w][1 << log2_w];
	};

	template <unsigned int log2_w>
	struct queue_item {
		dcosttype k1;
		dcosttype k2;
		dtile2<log2_w>* tile;
		unsigned char state_idx;
	};

	template <unsigned int log2_w>
	class priority_queue {
	public:
		virtual void* insert(const queue_item<log2_w>& item) = 0;
		virtual void remove(void* item) = 0;
		virtual void change_key(void* item) = 0;
	};


}
