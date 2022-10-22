#include <cstring>
#include <fstream>

#include "util_structs.hpp"
#include "geometry.hpp"

namespace ocpncy {

	/*
	* log2_w: log-base-2 of tile width; w = 2 ^ log2_w = 1 << log2_w
	* log2_w MUST BE GREATER THAN OR EQUAL TO 3!!!
	*	This is because the bit tiles are composed of 8x8 bit squares (stored as unsigned longs),
	*	so the minimum width of the tile is the width of one square, which is 8 or 2 ^ 3
	*/
	template <unsigned int log2_w>
	struct btile {
		unsigned long longs[1 << (log2_w - 3)][1 << (log2_w - 3)];
	};

	template <unsigned int log2_w>
	inline bool get_bit(int x, int y, const btile<log2_w>* ot) {
		return (ot->longs[x >> 3][y >> 3] >> ((x & 0x111) + ((y & 0x111) << 3))) & 0x1;
	}

	template <unsigned int log2_w>
	inline void set_bit(int x, int y, btile<log2_w>* ot, bool value) {
		ot->longs[x >> 3][y >> 3] |= value * 0x1 << ((x & 0x111) + ((y & 0x111) << 3));
	}

	struct btile_tree { void* branch[4]; };

	/*
	* tree: can be a btile* if it's a leaf or can be a btil_tree* if it's a branch
	* depth: number of layers below layer of the tree parameter (depth at root of tree = #layers - 1; 
																 depth at base of tree = 0)
	*/
	template <unsigned int log2_w>
	void delete_btile_tree(void* tree, unsigned int depth) {
		if (depth > 0 && tree) {
			for (int i = 0; i < 4; i++)
				delete_btile_tree<log2_w>(static_cast<btile_tree*>(tree)->branch[i], depth - 1);
			delete static_cast<btile_tree*>(tree);
		}
		else delete static_cast<btile<log2_w>*>(tree);
	}

	template <unsigned int log2_w>
	struct bmap_info {
		unsigned int log2_tile_w = log2_w;
		unsigned int depth = 0;
		long origin[2] = { 0 };
		bmap_info() {
			unsigned int log2_tile_w = log2_w;
			depth = 0;
			origin[0] = 0;
			origin[1] = 0;
		}
	};

	template <unsigned int log2_w>
	struct bmap {
		bmap_info<log2_w> info;
		btile_tree* tree = 0;
		~bmap() {
			delete_btile_tree<log2_w>(tree, info.depth);
			tree = 0;
		}
	};

	template <unsigned int log2_w>
	class bmap_istream { public: virtual btile<log2_w>* get_tile(int x, int y) = 0; };

	template <unsigned int log2_w>
	class bmap_ostream { 
		public: 
			virtual bool set_tile(int x, int y, btile<log2_w> tile) = 0;
			virtual void alloc(const gmtry2i::aligned_box2i& box) = 0;
	};

	template <unsigned int log2_w>
	class bmap_iostream : bmap_istream<log2_w>, bmap_ostream<log2_w> {};

	template <unsigned int log2_w>
	class bmap_fstream : bmap_iostream<log2_w> {
		protected:
			/*
			* root: position of root tree in file
			* size: size of file
			*/
			struct bmap_file_header {
				bmap_info<log2_w> info;
				unsigned long root;
				unsigned long size;
			};
			// offset: offset of an item from its parent in the file stream
			struct offset_tree { 
				long offset; offset_tree* branch[4];
				offset_tree(long item_offset) {
					offset = item_offset;
				}
			};
			static void delete_index_tree(offset_tree* tree, unsigned int depth) {
				if (depth > 0 && tree) {
					for (int i = 0; i < 4; i++)
						delete_index_tree(tree->branch[i], depth - 1);
					delete tree;
				}
			}
			// pos: position of item in file stream
			struct item_index {
				offset_tree* tree;
				unsigned long pos;
				unsigned int depth;
				item_index(offset_tree* tree_ptr, unsigned long tree_pos, unsigned int tree_depth) {
					tree = tree_ptr;
					pos = tree_pos;
					depth = tree_depth;
				}
			};

			bmap_file_header map_header;

			std::fstream file;
			std::string file_name;
			offset_tree* indices;

			void create_file(const std::string& fname, const bmap_info<log2_w>& info) {
				
			}
			void read_file(char* dst, unsigned long pos, unsigned long len) {
				file.seekg(pos);
				file.read(dst, len);
			}
			btile<log2_w>* read_tile(unsigned long pos) {
				btile<log2_w>* tile = new btile<log2_w>;
				read_file(reinterpret_cast<char*>(tile), pos, sizeof(btile<log2_w>));
				return tile;
			}
			// cannot be used to append
			void write_file(const char* src, unsigned long pos, unsigned long len) {
				file.seekp(pos);
				file.write(src, len);
			}
			// cannot be used to append
			void write_tile(unsigned long pos, const btile<log2_w>* tile) {
				write_file(reinterpret_cast<const char*>(tile), pos, sizeof(btile<log2_w>));
			}
			void append_file(const char* src, unsigned long len) {
				file.seekp(map_header.size);
				file.write(src, len);
				map_header.size += len;
			}
			item_index deepest_indexed_item(long tx, long ty) {
				offset_tree* tree = indices;
				offset_tree* next_tree;
				unsigned long pos = tree->offset;
				unsigned int tree_depth = map_header.info.depth;
				unsigned long hwidth = (1 << tree_depth) >> 1;
				while (tree_depth) {
					next_tree = tree->branch[(tx >= hwidth) + 2 * (ty >= hwidth)];
					if (next_tree == 0) break;
					tree = next_tree;
					pos += tree->offset;
					tx -= hwidth * (tx >= hwidth);
					ty -= hwidth * (ty >= hwidth);
					tree_depth -= 1;
					hwidth >>= 1;
				}
				return item_index(tree, pos, tree_depth);
			}
			item_index seek_deepest_item(long tx, long ty) {
				item_index deepest_item = deepest_indexed_item(tx, ty);
				long next_offset;
				unsigned long hwidth = (1 << deepest_item.depth) >> 1;
				int branch_offsets_size = 4 * sizeof(unsigned long);
				long* branch_offsets = new long[branch_offsets_size];
				while (deepest_item.depth) {
					read_file(reinterpret_cast<char*>(branch_offsets), deepest_item.pos, branch_offsets_size);
					deepest_item.tree->branch[0] = new offset_tree(branch_offsets[0]);
					deepest_item.tree->branch[1] = new offset_tree(branch_offsets[1]);
					deepest_item.tree->branch[2] = new offset_tree(branch_offsets[2]);
					deepest_item.tree->branch[3] = new offset_tree(branch_offsets[3]);
					next_offset = branch_offsets[(tx >= hwidth) + 2 * (ty >= hwidth)];
					if (next_offset == 0) break;
					deepest_item.tree = deepest_item.tree->branch[(tx >= hwidth) + 2 * (ty >= hwidth)];
					deepest_item.pos += next_offset;
					tx -= hwidth * (tx >= hwidth);
					ty -= hwidth * (ty >= hwidth);
					deepest_item.depth -= 1;
					hwidth >>= 1;
				}
				delete[] branch_offsets;
				return deepest_item;
			}

		public:
			// add constructor for brand new map
			bmap_fstream(const std::string& fname) {
				file_name = fname;
				file.open(file_name);
				if (!file.is_open()) create_file(fname, bmap_info<log2_w>());

				read_file(reinterpret_cast<char*>(&map_header), 0, sizeof(map_header));
				if (log2_w != map_header.info.log2_tile_w) throw - 1;
				if (map_header.size < 4 * sizeof(unsigned long)) throw - 2;

				indices = new offset_tree { map_header.root };
			}
			void alloc(gmtry2i::aligned_box2i box) {
				unsigned long map_width = 1 << (map_header.info.depth + map_header.info.log2_tile_w);
				while (map_header.info.origin[0] > box.min[0] || 
					map_header.info.origin[1] > box.min[1] ||
					map_header.info.origin[0] + map_width > box.max[0] || 
					map_header.info.origin[1] + map_width > box.max[1]) {



					map_width >>= 1;
				}
			}
			btile<log2_w>* get_tile(long x, long y) {
				x = (x - map_header.info.origin[0]) >> map_header.info.log2_tile_w;
				y = (y - map_header.info.origin[1]) >> map_header.info.log2_tile_w;
				int map_twidth = 1 << map_header.info.depth;
				if (x < 0 || y < 0 || x >= map_twidth || y >= map_twidth) return 0;
				item_index deepest_item = seek_deepest_item(x, y);
				if (deepest_item.depth > 0) return read_tile(deepest_item.pos);
				else return 0;
			}
			bool set_tile(long x, long y, btile<log2_w>* tile) {
				// transform coordinates
				// alloc necessary space
				// get deepest item index
				// at deepest item index, write pointer to end of file
				// append file with tree that leads to tile
			}
			~bmap_fstream() {
				if (file.is_open()) file.close();
				delete_index_tree(indices, map_header.info.depth);
			}
	};

	typedef bmap<9> occ_map512x512;
	bmap<9> abcdefghijklmnopqrstuvwxyz;
}