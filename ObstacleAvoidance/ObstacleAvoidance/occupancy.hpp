#pragma once

#include <cstring>
#include <string>
//#include <fstream>
#include <stdio.h>

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
		gmtry2i::vector2i origin;
		bmap_info() {
			unsigned int log2_tile_w = log2_w;
			depth = 0;
			origin = gmtry2i::vector2i();
		}
	};

	template <unsigned int log2_w>
	struct bmap {
		bmap_info<log2_w> info;
		void* root = 0;
		~bmap() {
			delete_btile_tree<log2_w>(root, info.depth);
			root = 0;
		}
	};

	template <unsigned int log2_w>
	class bmap_istream { 
		public: 
			virtual bool get_tile(const gmtry2i::vector2i& p, btile<log2_w>* tile) = 0;
	};

	template <unsigned int log2_w>
	class bmap_ostream { 
		public: 
			virtual bool set_tile(const gmtry2i::vector2i& p, btile<log2_w> tile) = 0;
			virtual void alloc(const gmtry2i::vector2i& p) = 0;
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
				bmap_file_header() {
					info = bmap_info<log2_w>();
					root = 0;
					size = sizeof(bmap_file_header);
				}
				bmap_file_header(bmap_info<log2_w> map_info, unsigned long map_root, unsigned long file_size) {
					info = map_info;
					root = map_root;
					size = file_size;
				}
			};
			// offset: offset of an item from its parent in the file stream
			struct index_tree { 
				long pos; index_tree* branch[4];
				index_tree(long item_pos) {
					pos = item_pos;
				}
			};
			static void delete_index_tree(index_tree* tree, unsigned int depth) {
				if (depth > 0 && tree) {
					for (int i = 0; i < 4; i++)
						delete_index_tree(tree->branch[i], depth - 1);
					delete tree;
				}
			}
			// pos: position of item in file stream
			struct item_index {
				index_tree* tree;
				unsigned int depth;
				gmtry2i::vector2i origin;
				item_index(index_tree* tree_ptr, unsigned int item_depth, gmtry2i::vector2i item_origin) {
					tree = tree_ptr;
					depth = item_depth;
					origin = item_origin;
				}
			};

			bmap_file_header map_header;

			FILE* file;
			std::string file_name;
			index_tree* indices;

			void read_file(char* dst, unsigned long pos, unsigned long len) {
				fseek(file, pos, SEEK_SET);
				fread(dst, 1, len, file);
				/*
				file.seekg(pos);
				file.read(dst, len);
				*/
			}
			// cannot be used to append
			void write_file(const char* src, unsigned long pos, unsigned long len) {
				fseek(file, pos, SEEK_SET);
				fwrite(src, 1, len, file);
				/*
				file.seekp(pos);
				file.write(src, len);
				*/
			}
			void append_file(const char* src, unsigned long len) {
				fseek(file, map_header.size, SEEK_SET);
				fwrite(src, 1, len, file);
				/*
				file.seekp(map_header.size);
				file.write(src, len);
				*/
				map_header.size += len;
			}
			gmtry2i::aligned_box2i get_allocated_bounds() {
				long map_width = 1 << (map_header.info.depth + map_header.info.log2_tile_w);
				return gmtry2i::aligned_box2i(map_header.info.origin,
					map_header.info.origin + gmtry2i::vector2i(map_width, map_width));
			}
			void double_map(const gmtry2i::vector2i& direction) {
				long map_init_width = 1 << (map_header.info.depth + map_header.info.log2_tile_w);
				unsigned int old_root_index = (direction.x < 0) + 2 * (direction.y < 0);
				index_tree* new_root = new index_tree(map_header.size);
				new_root->branch[old_root_index] = indices;
				indices = new_root;

				unsigned long new_root_branches[4] = { 0 };
				new_root_branches[old_root_index] = map_header.root;
				append_file(reinterpret_cast<char*>(new_root_branches), 4 * sizeof(unsigned long));

				map_header.root = new_root->pos;
				map_header.info.depth += 1;
				map_header.info.origin -= gmtry2i::vector2i(direction.x < 0, direction.y < 0) * map_init_width;
				write_file(reinterpret_cast<char*>(&map_header), 0, sizeof(map_header));
			}
			item_index deepest_indexed_item(const gmtry2i::vector2i& tp) {
				index_tree* tree = indices;
				index_tree* next_tree;
				unsigned int tree_depth = map_header.info.depth;
				gmtry2i::vector2i tree_origin = gmtry2i::vector2i();
				gmtry2i::vector2i relative_tp;
				unsigned int hwidth = (1 << tree_depth) >> 1;
				while (tree_depth) {
					relative_tp = tp - tree_origin;
					next_tree = tree->branch[(relative_tp.x >= hwidth) + 2 * (relative_tp.y >= hwidth)];
					if (next_tree == 0) break;
					tree = next_tree;
					tree_depth -= 1;
					tree_origin += gmtry2i::vector2i(relative_tp.x >= hwidth, relative_tp.y >= hwidth) * hwidth;
					hwidth >>= 1;
				}
				return item_index(tree, tree_depth, tree_origin);
			}
			item_index seek_deepest_item(const gmtry2i::vector2i& tp) {
				item_index deepest_item = deepest_indexed_item(tp);
				unsigned int next_branch_idx;
				gmtry2i::vector2i relative_tp;
				unsigned int hwidth = (1 << deepest_item.depth) >> 1;
				unsigned long branches[4];
				while (deepest_item.depth) {
					relative_tp = tp - deepest_item.origin;
					read_file(reinterpret_cast<char*>(branches), deepest_item.tree->pos, 4 * sizeof(unsigned long));
					next_branch_idx = (relative_tp.x >= hwidth) + 2 * (relative_tp.y >= hwidth);
					if (branches[next_branch_idx] == 0) break;
					deepest_item.tree = deepest_item.tree->branch[next_branch_idx] = new index_tree(branches[next_branch_idx]);
					deepest_item.depth -= 1;
					deepest_item.origin += gmtry2i::vector2i(relative_tp.x >= hwidth, relative_tp.y >= hwidth) * hwidth;
					hwidth >>= 1;
				}
				return deepest_item;
			}
			void write_deepest_item(const gmtry2i::vector2i tp, const btile<log2_w>* tile) {
				item_index deepest_item = seek_deepest_item(tp);
				if (deepest_item.depth == 0) {
					write_file(reinterpret_cast<const char*>(tile), deepest_item.tree->pos, sizeof(btile<log2_w>));
					return;
				}
				unsigned int hwidth = (1 << deepest_item.depth) >> 1;
				gmtry2i::vector2i relative_tp = tp - deepest_item.origin;
				unsigned int next_branch_idx = (relative_tp.x >= hwidth) + 2 * (relative_tp.y >= hwidth);
				const unsigned int branches_size = 4 * sizeof(unsigned long);
				write_file(reinterpret_cast<const char*>(map_header.size),
					deepest_item.tree->pos + next_branch_idx * sizeof(unsigned long), sizeof(map_header.size));
				deepest_item.tree = deepest_item.tree->branch[next_branch_idx] = new index_tree(map_header.size);
				deepest_item.depth -= 1;
				deepest_item.origin += gmtry2i::vector2i(relative_tp.x >= hwidth, relative_tp.y >= hwidth) * hwidth;
				hwidth >>= 1;
				while (deepest_item.depth) {
					relative_tp = tp - deepest_item.origin;
					unsigned long branches[4] = { 0 };
					next_branch_idx = (relative_tp.x >= hwidth) + 2 * (relative_tp.y >= hwidth);
					branches[next_branch_idx] = map_header.size + branches_size;
					append_file(reinterpret_cast<char*>(branches), branches_size);
					deepest_item.tree = deepest_item.tree->branch[next_branch_idx] = new index_tree(branches[next_branch_idx]);
					deepest_item.depth -= 1;
					deepest_item.origin += gmtry2i::vector2i(relative_tp.x >= hwidth, relative_tp.y >= hwidth) * hwidth;
					hwidth >>= 1;
				}
				append_file(reinterpret_cast<const char*>(tile), sizeof(btile<log2_w>));
			}

		public:
			// add constructor for brand new map
			bmap_fstream(const std::string& fname) {
				file_name = fname;
				file = fopen(file_name.c_str(), "a+");
				FILE* tmp_file = fopen(file_name.c_str(), "r");
				if (tmp_file) {
					fclose(tmp_file);
				}
				else {
					map_header = bmap_file_header();
					write_file(reinterpret_cast<char*>(&map_header), 0, sizeof(map_header));
				}
				if (file == 0) throw - 1;

				read_file(reinterpret_cast<char*>(&map_header), 0, sizeof(map_header));
				if (log2_w != map_header.info.log2_tile_w) throw - 2;
				if (map_header.size < sizeof(map_header)) throw - 4;

				indices = new index_tree { map_header.root };
			}
			void alloc(gmtry2i::vector2i p) {
				gmtry2i::aligned_box2i alloc_box = get_allocated_bounds();
				while (!gmtry2i::contains(alloc_box, p)) {
					// CHECK FOR ROUNDING ERRORS
					double_map(p - gmtry2i::center(alloc_box));
					alloc_box = get_allocated_bounds();
				}
			}
			void alloc(gmtry2i::aligned_box2i box) {
				gmtry2i::aligned_box2i alloc_box = get_allocated_bounds();
				gmtry2i::vector2i box_center = gmtry2i::center(box);
				while (!gmtry2i::contains(alloc_box, box)) {
					// CHECK FOR ROUNDING ERRORS
					double_map(box_center - gmtry2i::center(alloc_box));
					alloc_box = get_allocated_bounds();
				}
			}
			bool read(const gmtry2i::vector2i& p, btile<log2_w>* tile) {
				if (!gmtry2i::contains(get_allocated_bounds(), p)) return false;
				item_index deepest_item = seek_deepest_item((p - map_header.info.origin) >> map_header.info.log2_tile_w);
				if (deepest_item.depth == 0) {
					read_file(reinterpret_cast<char*>(tile), deepest_item.tree->pos, sizeof(btile<log2_w>));
					return true;
				} else return false;
			}
			bool write(const gmtry2i::vector2i& p, const btile<log2_w>* tile) {
				alloc(p);
				write_deepest_item(p, tile);
				return true;
			}
			~bmap_fstream() {
				if (file) fclose(file);
				delete_index_tree(indices, map_header.info.depth);
			}
	};

	typedef bmap<9> occ_map512x512;
	bmap<9> abcdefghijklmnopqrstuvwxyz;
}