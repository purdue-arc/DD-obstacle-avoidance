#pragma once


namespace dstar {
	inline int ceilDiv(int a, int b) {
		return (a / b) + (a % b != 0);
	}

	inline unsigned int ceilDivByPowOf2(unsigned int a, unsigned int log2_b) {
		return (a >> log2_b) + ((a & ((1 << log2_b) - 1)) != 0);
	}

	template <unsigned int log2_w>
	struct dtile2 {
		bool occ[1 << log2_w][1 << log2_w];
		unsigned int g[1 << log2_w][1 << log2_w];
		unsigned int rhs[1 << log2_w][1 << log2_w];
		dtile2* nbrs[8];
	};


}
