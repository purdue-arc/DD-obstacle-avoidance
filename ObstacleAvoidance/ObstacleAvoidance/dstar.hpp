#include <cstring>
#include <fstream>

#include "util_structs.hpp"
using namespace util;

namespace dstar {
	inline int ceilDiv(int a, int b) {
		return (a / b) + (a % b != 0);
	}

	inline unsigned int ceilDivByPowOf2(unsigned int a, unsigned int log2_b) {
		return (a >> log2_b) + ((a & ((1 << log2_b) - 1)) != 0);
	}

	// log2_w >= 3
	template <unsigned int log2_w>
	struct bittile2 {
		unsigned long longs[1 << (log2_w - 3)][1 << (log2_w - 3)];
	};

	template <unsigned int log2_w>
	bool getBin(int x, int y, const bittile2<log2_w>* ot) {
		return (ot->longs[x >> 3][y >> 3] >> ((x & 0x111) + ((y & 0x111) << 3))) & 0x1;
	}

	template <unsigned int log2_w>
	void setBin(int x, int y, bittile2<log2_w>* ot, bool value) {
		ot->longs[x >> 3][y >> 3] |= value * 0x1 << ((x & 0x111) + ((y & 0x111) << 3));
	}

	struct bittile2_tree { void* sub[4]; };

	template <unsigned int log2_w>
	void delete_bittile2_tree(void* tree, unsigned int depth) {
		if (depth > 0 && tree) {
			for (int i = 0; i < 4; i++)
				delete_bittile2_tree<log2_w>(static_cast<bittile2_tree*>(tree)->sub[i], depth - 1);
			delete static_cast<bittile2_tree*>(tree);
		} 
		else delete static_cast<bittile2<log2_w>*>(tree);
	}

	template <unsigned int log2_w>
	struct occ_map2 {
		const unsigned int log2_tile_w = log2_w;
		unsigned int depth = 0;
		long origin[2]; // origin of tree (center) with respect to the coordinate system
		bittile2_tree* tree = 0;
		~occ_map2() {
			delete_bittile2_tree<log2_w>(tree, depth);
			tree = 0;
		}
	};

	template <unsigned int log2_w>
	class map_istream { public: virtual bittile2<log2_w>* getTile(int x, int y) = 0; };

	template <unsigned int log2_w>
	class map_ostream { public: virtual void setTile(int x, int y, bittile2<log2_w> tile) = 0; };

	template <unsigned int log2_w>
	class map_iostream : map_istream<log2_w>, map_ostream<log2_w> {};

	template <unsigned int log2_w>
	class map_fstream : map_iostream<log2_w> {
		protected:
			struct index_tree { unsigned long index; index_tree* sub[4]; };
			static void delete_index_tree(index_tree* tree, unsigned int depth) {
				if (depth > 0 && tree) {
					for (int i = 0; i < 4; i++)
						delete_index_tree(tree->sub[i], depth - 1);
					delete tree;
				}
			}

			const unsigned int log2_tile_w = log2_w;
			unsigned int depth = 0;
			long origin[2];

			int err;
			std::fstream file;
			index_tree* indices;

			void read_file(char* dst, unsigned long idx, unsigned long len) {
				file.seekg(idx);
				file.read(dst, len);
			}
			void write_file(const char* src, unsigned long idx, unsigned long len) {
				file.seekp(idx);
				file.write(src, len);
			}
			bittile2<log2_w>* read_tile(unsigned long idx) {
				bittile2<log2_w>* tile = new bittile2<log2_w>;
				file.seekg(idx);
				file.read(reinterpret_cast<char*>(tile), sizeof(bittile2<log2_w>));
				return tile;
			}
			unsigned long get_deepest_index(long tx, long ty, unsigned int* final_depth) {
				index_tree* tree = indices;
				index_tree* next_tree;
				unsigned int tree_depth = depth;
				unsigned long hwidth = (1 << tree_depth) >> 1;
				while (tree_depth) {
					next_tree = tree->sub[(tx >= hwidth) + 2 * (ty >= hwidth)];
					if (next_tree == 0) break;
					tree = next_tree;
					tx -= hwidth * (tx >= hwidth);
					ty -= hwidth * (ty >= hwidth);
					tree_depth--;
					hwidth >>= 1;
				}

				unsigned long index = tree->index;
				unsigned long next_index;
				unsigned long* next_sub_indices;
				while (tree_depth) {
					read_file(reinterpret_cast<char*>(next_sub_indices), index, 4 * sizeof(unsigned long));
					tree->sub[0] = new index_tree{ next_sub_indices[0] };
					tree->sub[1] = new index_tree{ next_sub_indices[1] };
					tree->sub[2] = new index_tree{ next_sub_indices[2] };
					tree->sub[3] = new index_tree{ next_sub_indices[3] };
					next_index = next_sub_indices[(tx >= hwidth) + 2 * (ty >= hwidth)];
					if (next_index == 0) break;
					tree = tree->sub[(tx >= hwidth) + 2 * (ty >= hwidth)];
					index = next_index;
					tx -= hwidth * (tx >= hwidth);
					ty -= hwidth * (ty >= hwidth);
					tree_depth--;
					hwidth >>= 1;
				}
				delete[] next_sub_indices;

				*final_depth = tree_depth;
				return index;
			}

		public:
			map_fstream(const char* fname) {
				file.open(fname);

				unsigned long header_len = sizeof(occ_map2<log2_w>) - sizeof(bittile2_tree*);
				char* occ_map_data = new char[header_len];
				read_file(occ_map_data, 0, header_len);
				char* occ_map_data_pos = occ_map_data;

				unsigned int correct_log2_w = *reinterpret_cast<unsigned int*>(occ_map_data_pos);
				occ_map_data_pos += sizeof(unsigned int);

				depth = *reinterpret_cast<unsigned int*>(occ_map_data_pos);
				occ_map_data_pos += sizeof(unsigned int);

				origin[0] = reinterpret_cast<unsigned long*>(occ_map_data_pos)[0];
				origin[1] = reinterpret_cast<unsigned long*>(occ_map_data_pos)[1];
				
				if (log2_tile_w != correct_log2_w) {
					delete[] occ_map_data;
					throw - 1;
				}

				indices = new index_tree { header_len };

				delete[] occ_map_data;
			}
			bittile2<log2_w>* get_tile(long x, long y) {
				unsigned int final_depth;
				unsigned long tile_idx = get_deepest_index(x, y, &final_depth);
				if (final_depth) return 0;
				else return read_tile(tile_idx);
			}
			void set_tile(long x, long y, bittile2<log2_w>* tile) {

			}
			~map_fstream() {
				if (file.is_open()) file.close();
				delete_index_tree(indices, depth);
			}
	};
	
	
	template <unsigned int log2_w>
	char_chain* serialize(const bittile2_tree* tree, unsigned int depth, 
						  unsigned long size_above, unsigned long* size_below) {
		const int ptrs_size = 4 * sizeof(unsigned long);
		// The my_chain's char array holds "pointers" to this tree's children
		// These "pointers" say where to find this tree's children in the file
		char_chain* my_chain = new char_chain { new unsigned char[ptrs_size] { 0 }, ptrs_size, 0 };
		my_chain->end = my_chain;
		unsigned long my_size = ptrs_size;
		unsigned long child_size;
		for (int i = 0; i < 4; i++) if (tree->sub[i]) {
			reinterpret_cast<unsigned long*>(my_chain->chars)[i] = size_above + my_size;
			if (depth > 0) append(my_chain, serialize<log2_w>(reinterpret_cast<bittile2_tree*>(tree->sub[i]),
																depth - 1, size_above + my_size, &child_size));
			else {
				child_size = sizeof(bittile2<log2_w>); // = 1 << (2 * log2_w - 3);
				append(my_chain, new char_chain { new unsigned char[child_size], child_size, 0 });
				std::memcpy(my_chain->end->chars, tree->sub[i], child_size);
			}
			my_size += child_size;
		}
		*size_below = my_size;
		return my_chain;
	}

	template <unsigned int log2_w>
	char_chain* serialize(const occ_map2<log2_w>* map, unsigned long* total_size) {
		unsigned long header_size = sizeof(occ_map2<log2_w>) - sizeof(bittile2_tree*);
		char_chain* header_chain = new char_chain{ new unsigned char[header_size], header_size, 0 };
		header_chain->end = header_chain;
		std::memcpy(header_chain->chars, &map, header_size);
		unsigned long tree_size = 0;
		if (map->tree) append(header_chain, serialize<log2_w>(map->tree, map->depth, header_size, &tree_size));
		else {
			tree_size = 4 * sizeof(unsigned long);
			char_chain* tree_chain = new char_chain{ new unsigned char[tree_size] { 0 }, tree_size, 0};
			tree_chain->end = tree_chain;
			append(header_chain, tree_chain);
		}
		*total_size = header_size + tree_size;
		return header_chain;
	}

	template <unsigned int log4_w>
	struct dtile2 {
		const static int width = 1 << log4_w;
		bool occ[width][width];
		unsigned int g[width][width];
		unsigned int rhs[width][width];
		dtile2* nbrs[8];
	};


}
