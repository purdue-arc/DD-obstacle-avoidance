#pragma once

#include <string>
//#include <fstream>
#include <stdio.h>

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
		bmap_info(const gmtry2i::vector2i map_origin) {
			unsigned int log2_tile_w = log2_w;
			depth = 0;
			origin = map_origin;
		}
		bmap_info(unsigned int map_depth, const gmtry2i::vector2i map_origin) {
			unsigned int log2_tile_w = log2_w;
			depth = map_depth;
			origin = map_origin;
		}
	};

	template <unsigned int log2_w>
	struct bmap {
		bmap_info<log2_w> info;
		void* root = 0;
		bmap(const bmap_info<log2_w> map_info, void* map_root) {
			info = map_info;
			root = map_root;
		}
		~bmap() {
			delete_btile_tree<log2_w>(root, info.depth);
			root = 0;
		}
	};

	template <unsigned int log2_w>
	struct bmap_item {
		void* ptr;
		unsigned int depth;
		gmtry2i::vector2i origin;
		bmap_item() = default;
		bmap_item(unsigned int item_depth, gmtry2i::vector2i item_origin) {
			ptr = 0;
			depth = item_depth;
			origin = item_origin;
		}
		bmap_item(void* item_ptr, unsigned int item_depth, gmtry2i::vector2i item_origin) {
			ptr = item_ptr;
			depth = item_depth;
			origin = item_origin;
		}
	};

	template <unsigned int log2_w>
	inline gmtry2i::aligned_box2i get_bmap_bounds(const bmap<log2_w>* map) {
		return gmtry2i::aligned_box2i(map->info.origin, 1 << (map->info.depth + map->info.log2_w));
	}

	template <unsigned int log2_w>
	inline gmtry2i::aligned_box2i get_bmap_item_bounds(const bmap_item<log2_w>& item) {
		return gmtry2i::aligned_box2i(item.origin, 1 << (item.depth + log2_w));
	}

	template <unsigned int log2_w>
	void expand_bmap(bmap<log2_w>* map, const gmtry2i::vector2i& direction) {
		long map_init_width = 1 << (map->info.depth + map->info.log2_tile_w);
		unsigned int old_root_index = (direction.x < 0) + 2 * (direction.y < 0);
		btile_tree* new_root = new btile_tree();
		new_root->branch[old_root_index] = map->root;
		map->root = new_root;
		map->info.depth += 1;
		map->info.origin -= gmtry2i::vector2i(direction.x < 0, direction.y < 0) * map_init_width;
	}

	template <unsigned int log2_w>
	void fit_bmap(bmap<log2_w>* map, const gmtry2i::vector2i p) {
		gmtry2i::aligned_box2i alloc_box = get_bmap_bounds(map);
		while (!gmtry2i::contains(alloc_box, p)) {
			// CHECK FOR ROUNDING ERRORS
			expand_bmap(map, p - gmtry2i::center(alloc_box));
			alloc_box = get_bmap_bounds(map);
		}
	}

	template <unsigned int log2_w>
	void fit_bmap(bmap<log2_w>* map, const gmtry2i::aligned_box2i box) {
		gmtry2i::aligned_box2i alloc_box = get_bmap_bounds(map);
		gmtry2i::vector2i box_center = gmtry2i::center(box);
		while (!gmtry2i::contains(alloc_box, box)) {
			// CHECK FOR ROUNDING ERRORS
			expand_bmap(map, box_center - gmtry2i::center(alloc_box));
			alloc_box = get_bmap_bounds(map);
		}
	}

	template <unsigned int log2_w>
	bmap_item<log2_w> get_bmap_item(const bmap<log2_w>* map, const gmtry2i::vector2i& p, unsigned int depth) {
		bmap_item<log2_w> item;
		bmap_item<log2_w> next_item(map->root, map->info.depth, map->info.origin);
		unsigned int next_branch_index;
		unsigned int hwidth = (1 << (next_item.depth + log2_w)) >> 1;
		while (next_item.ptr && next_item.depth > depth) {
			item = next_item;
			next_branch_index = (p.x - item.origin.x >= hwidth) + 2 * (p.y - item.origin.y >= hwidth);
			next_item.ptr = reinterpret_cast<btile_tree*>(item.ptr)->branch[next_branch_index];
			next_item.depth = item.depth - 1;
			next_item.origin = item.origin + gmtry2i::vector2i(next_branch_index & 1, next_branch_index >> 1) * hwidth;
			hwidth >>= 1;
		}
		return next_item.ptr ? next_item : item;
		// next_item is at desired depth if it exists
		// item is not at desired depth but it exists if next_item doesn't exist and map->root exists 
	}

	template <unsigned int log2_w>
	void set_bmap_tile(bmap<log2_w>* dst, const gmtry2i::vector2i& p, unsigned int depth, const btile<log2_w>* src) {
		fit_bmap(dst, p);
		bmap_item<log2_w> item;
		bmap_item<log2_w> next_item = get_bmap_item(dst, p, depth);
		unsigned int next_branch_index;
		unsigned int hwidth = (1 << (next_item.depth + log2_w)) >> 1;
		while (next_item.depth > depth) {
			item = next_item;
			next_branch_index = (p.x - item.origin.x >= hwidth) + 2 * (p.y - item.origin.y >= hwidth);
			next_item.ptr = reinterpret_cast<btile_tree*>(item.ptr)->branch[next_branch_index] = new btile_tree();
			next_item.depth = item.depth - 1;
			next_item.origin = item.origin + gmtry2i::vector2i(next_branch_index & 1, next_branch_index >> 1) * hwidth;
			hwidth >>= 1;
		}
		if (item.ptr) {
			delete reinterpret_cast<btile_tree*>(next_item.ptr);
			next_item.ptr = new btile<log2_w>;
		}
		*(reinterpret_cast<btile<log2_w>*>(next_item.ptr)) = *src;
	}

	template <unsigned int log2_w>
	class bmap_istream { 
		public: 
			virtual bool read(const gmtry2i::vector2i& p, btile<log2_w>* dst) = 0;
			virtual bool read(const gmtry2i::vector2i& p, unsigned int depth, bmap_item<log2_w>& dst) = 0;
			virtual gmtry2i::aligned_box2i get_bounds() = 0;
	};

	template <unsigned int log2_w>
	class bmap_ostream { 
		public: 
			virtual bool write(const gmtry2i::vector2i& p, const btile<log2_w>* src) = 0;
			virtual bool write(const gmtry2i::vector2i& p, unsigned int depth, const bmap_item<log2_w>& src) = 0;
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
			struct index_tree { 
				long pos; 
				bool fully_indexed;
				index_tree* branch[4] = { 0 };
				index_tree(long item_pos) {
					pos = item_pos;
					fully_indexed = false;
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
				item_index() = default;
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

			void read_file(void* dst, unsigned long pos, unsigned long len) {
				fseek(file, pos, SEEK_SET);
				fread(dst, 1, len, file);
				/*
				file.seekg(pos);
				file.read(dst, len);
				*/
			}
			// cannot be used to append
			void write_file(const void* src, unsigned long pos, unsigned long len) {
				fseek(file, pos, SEEK_SET);
				fwrite(src, 1, len, file);
				/*
				file.seekp(pos);
				file.write(src, len);
				*/
			}
			void append_file(const void* src, unsigned long len) {
				fseek(file, map_header.size, SEEK_SET);
				fwrite(src, 1, len, file);
				/*
				file.seekp(map_header.size);
				file.write(src, len);
				*/
				map_header.size += len;
			}
			inline gmtry2i::aligned_box2i get_map_bounds() {
				return gmtry2i::aligned_box2i(map_header.info.origin, 
					1 << (map_header.info.depth + map_header.info.log2_tile_w));
			}
			void expand_map(const gmtry2i::vector2i& direction) {
				long map_init_width = 1 << (map_header.info.depth + map_header.info.log2_tile_w);
				unsigned int old_root_index = (direction.x < 0) + 2 * (direction.y < 0);
				index_tree* new_root = new index_tree(map_header.size);
				new_root->branch[old_root_index] = indices;
				indices = new_root;

				unsigned long new_root_branches[4] = { 0 };
				new_root_branches[old_root_index] = map_header.root;
				append_file(new_root_branches, 4 * sizeof(unsigned long));

				map_header.root = new_root->pos;
				map_header.info.depth += 1;
				map_header.info.origin -= gmtry2i::vector2i(direction.x < 0, direction.y < 0) * map_init_width;
				write_file(&map_header, 0, sizeof(map_header));
			}
			void fit_map(const gmtry2i::vector2i& p) {
				gmtry2i::aligned_box2i alloc_box = get_map_bounds();
				while (!gmtry2i::contains(alloc_box, p)) {
					// CHECK FOR ROUNDING ERRORS
					expand_map(p - gmtry2i::center(alloc_box));
					alloc_box = get_map_bounds();
				}
			}
			void fit_map(const gmtry2i::aligned_box2i& box) {
				gmtry2i::aligned_box2i alloc_box = get_map_bounds();
				gmtry2i::vector2i box_center = gmtry2i::center(box);
				while (!gmtry2i::contains(alloc_box, box)) {
					// CHECK FOR ROUNDING ERRORS
					expand_map(box_center - gmtry2i::center(alloc_box));
					alloc_box = get_map_bounds();
				}
			}
			item_index indexed_item_at_depth(const gmtry2i::vector2i& p, unsigned int gdepth) {
				item_index item;
				item_index next_item(indices, map_header.info.depth, map_header.info.origin);
				unsigned int next_branch_idx;
				gmtry2i::vector2i relative_p;
				unsigned int hwidth = (1 << (item.depth + log2_w)) >> 1;
				while (next_item.tree && next_item.depth > gdepth) {
					item = next_item;
					next_branch_idx = (p.x - item.origin.x >= hwidth) + 2 * (p.y - item.origin.y >= hwidth);
					next_item.tree = item.tree->branch[next_branch_idx];
					next_item.depth = item.depth - 1;
					next_item.origin = item.origin + gmtry2i::vector2i(next_branch_idx & 1, next_branch_idx >> 1) * hwidth;
					hwidth >>= 1;
				}
				return next_item.tree ? next_item : item;
			}
			item_index seek_item_at_depth(const gmtry2i::vector2i& p, unsigned int gdepth) {
				item_index item;
				item_index next_item = indexed_item_at_depth(p, gdepth);
				unsigned int next_branch_idx;
				unsigned int hwidth = (1 << next_item.depth + log2_w) >> 1;
				unsigned long branches[4];
				while (next_item.tree->pos && next_item.depth > gdepth) {
					item = next_item;
					read_file(branches, item.tree->pos, 4 * sizeof(unsigned long));
					next_branch_idx = (p.x - item.origin.x >= hwidth) + 2 * (p.y - item.origin.y >= hwidth);
					next_item.tree = item.tree->branch[next_branch_idx] = new index_tree(branches[next_branch_idx]);
					next_item.depth = item.depth - 1;
					next_item.origin = item.origin + gmtry2i::vector2i(next_branch_idx & 1, next_branch_idx >> 1) * hwidth;
					hwidth >>= 1;
				}
				return next_item.tree->pos ? next_item : item;
			}
			void write_tile_at_bottom(const gmtry2i::vector2i p, const btile<log2_w>* tile) {
				item_index item = seek_item_at_depth(p, 0);
				if (item.depth == 0) {
					write_file(tile, item.tree->pos, sizeof(btile<log2_w>));
					return;
				}
				unsigned int hwidth = (1 << (item.depth + log2_w)) >> 1;
				unsigned int next_branch_idx = (p.x - item.origin.x >= hwidth) + 2 * (p.y - item.origin.y >= hwidth);
				const unsigned int branches_size = 4 * sizeof(unsigned long);
				write_file(&(map_header.size), item.tree->pos + next_branch_idx * sizeof(unsigned long), sizeof(map_header.size));
				item.tree = item.tree->branch[next_branch_idx] = new index_tree(map_header.size);
				item.depth -= 1;
				item.origin += gmtry2i::vector2i(next_branch_idx & 1, next_branch_idx >> 1) * hwidth;
				hwidth >>= 1;
				while (item.depth) {
					unsigned long branches[4] = { 0 };
					next_branch_idx = (p.x - item.origin.x >= hwidth) + 2 * (p.y - item.origin.y >= hwidth);
					branches[next_branch_idx] = map_header.size + branches_size;
					append_file(branches, branches_size);
					item.tree = item.tree->branch[next_branch_idx] = new index_tree(branches[next_branch_idx]);
					item.depth -= 1;
					item.origin += gmtry2i::vector2i(next_branch_idx & 1, next_branch_idx >> 1) * hwidth;
					hwidth >>= 1;
				}
				append_file(tile, sizeof(btile<log2_w>));
			}
			void* build_item(const item_index& item) {
				if (item.tree == 0) return 0;
				if (item.depth == 0) {
					btile<log2_w>* tile = new btile<log2_w>;
					read_file(tile, item.tree->pos, sizeof(btile<log2_w>));
					return tile;
				}
				else {
					if (!(item.tree->fully_indexed)) {
						unsigned long branches[4];
						read_file(branches, item.tree->pos, 4 * sizeof(unsigned long));
						for (int i = 0; i < 4; i++) if (item.tree->branch[i] == 0 && branches[i]) 
							item.tree->branch[i] = new index_tree(branches[i]);
						item.tree->fully_indexed = true;
					}
					btile_tree* item_tree = new btile_tree;
					for (int i = 0; i < 4; i++) item_tree->branch[i] = 
						build_item(item_index(item.tree->branch[0], item.depth - 1, gmtry2i::vector2i()));
					return item_tree;
				}
			}

		public:
			bmap_fstream(const std::string& fname, const gmtry2i::vector2i& origin) {
				file_name = fname;
				file = fopen(file_name.c_str(), "a+");
				FILE* tmp_file = fopen(file_name.c_str(), "r");
				if (tmp_file) {
					fclose(tmp_file);
				}
				else {
					map_header.info = bmap_info<log2_w>(origin);
					map_header.root = 0;
					map_header.size = sizeof(bmap_file_header);
					write_file(&map_header, 0, sizeof(map_header));
				}
				if (file == 0) throw - 1;

				read_file(&map_header, 0, sizeof(map_header));
				if (log2_w != map_header.info.log2_tile_w) throw - 2;
				if (map_header.size < sizeof(map_header)) throw - 4;

				if (map_header.root) indices = new index_tree{ map_header.root };
				else indices = 0;
			}
			bool read(const gmtry2i::vector2i& p, btile<log2_w>* dst) {
				if (!gmtry2i::contains(get_map_bounds(), p)) return false;
				item_index deepest_item = seek_item_at_depth(p, 0);
				if (deepest_item.tree && deepest_item.depth == 0) {
					read_file(dst, deepest_item.tree->pos, sizeof(btile<log2_w>));
					return true;
				} 
				else return false;
			}
			bool read(const gmtry2i::vector2i& p, unsigned int depth, bmap_item<log2_w>& dst) {
				if (!gmtry2i::contains(get_map_bounds(), p)) return false;
				item_index deepest_item = seek_item_at_depth(p, depth);
				if (deepest_item.tree && deepest_item.depth == depth) {
					dst = bmap_item<log2_w>(build_item(deepest_item), deepest_item.depth, deepest_item.origin);
					return true;
				}
				else return false;
			}
			gmtry2i::aligned_box2i get_bounds() { return get_map_bounds(); }
			bool write(const gmtry2i::vector2i& p, const btile<log2_w>* src) {
				if (file == 0) return false;
				fit_map(p);
				write_tile_at_bottom(p, src);
				return true;
			}
			bool write(const gmtry2i::vector2i& p, unsigned int depth, const bmap_item<log2_w>& src) {
				if (file == 0) return false;
				// FINISH WRITING THIS FUNCTION




			}
			~bmap_fstream() {
				if (file) fclose(file);
				delete_index_tree(indices, map_header.info.depth);
			}
	};

	// DON'T USE THIS FUNCTION ITS CURSED PLEASE DON'T DO IT
	// This function assumes that map and src are perfectly aligned
	// It will still work if they are not, but src will not be written to the expected place in map
	template <unsigned int log2_w>
	void add_bmap_item(bmap<log2_w>* dst, const bmap_item<log2_w>& src) {
		if (src.ptr == 0) return;
		fit_bmap(dst, get_bmap_item_bounds(src));
		bmap_item<log2_w> item;
		bmap_item<log2_w> next_item = get_bmap_item(dst, src.origin, src.depth);
		// next_item exists and might be the same type & source as src
		// depth of next_item is greater than or equal to depth of src
		unsigned int next_branch_index;
		unsigned int hwidth = (1 << (next_item.depth + log2_w)) >> 1;
		while (next_item.depth > src.depth) {
			item = next_item;
			next_branch_index = (src.origin.x - item.origin.x >= hwidth) + 2 * (src.origin.y - item.origin.y >= hwidth);
			next_item.ptr = reinterpret_cast<btile_tree*>(item.ptr)->branch[next_branch_index] = new btile_tree();
			next_item.depth = item.depth - 1;
			next_item.origin = item.origin + gmtry2i::vector2i(next_branch_index & 1, next_branch_index >> 1) * hwidth;
			hwidth >>= 1;
		}
		// next_item is same depth as src
		// if item exists then next_item is a tree type (definitely new) (might not be same type as src)
		// if item does not exist then next_item is the same type as src
		if (item.ptr) {
			delete reinterpret_cast<btile_tree*>(next_item.ptr);
			next_item.ptr = new btile<log2_w>;
		}
		// next_item is the same type as src
		if (next_item.depth == 0) {
			*reinterpret_cast<btile<log2_w>*>(next_item.ptr) = *reinterpret_cast<btile<log2_w>*>(src.ptr);
			return;
		}
		// next_item/src have depth greater than 0

		btile_tree** dst_levels = new btile_tree * [src.depth] { reinterpret_cast<btile_tree*>(next_item.ptr) };
		btile_tree** src_levels = new btile_tree * [src.depth] { reinterpret_cast<btile_tree*>(src.ptr) };
		unsigned int* branch_indices = new unsigned int[src.depth] { 0 };
		// there must be one level item per tree level (#tree levels = depth)
		unsigned int current_level = 0;
		while (branch_indices[0] < 4) {
			if (branch_indices[current_level] == 4) branch_indices[--current_level]++;
			else {
				if (src_levels[current_level]->branch[branch_indices[current_level]]) {
					if (current_level == src.depth - 1) {
						if (dst_levels[current_level]->branch[branch_indices[current_level]] == 0)
							dst_levels[current_level]->branch[branch_indices[current_level]] = new btile<log2_w>;
						*reinterpret_cast<btile<log2_w>*>(dst_levels[current_level]->branch[branch_indices[current_level]]) =
							*reinterpret_cast<btile<log2_w>*>(src_levels[current_level]->branch[branch_indices[current_level]]);
						branch_indices[current_level]++;
					}
					else {
						if (dst_levels[current_level]->branch[branch_indices[current_level]] == 0)
							dst_levels[current_level]->branch[branch_indices[current_level]] = new btile_tree();
						dst_levels[current_level] = reinterpret_cast<btile_tree*>(dst_levels[current_level]->branch[branch_indices[current_level]]);
						src_levels[current_level] = reinterpret_cast<btile_tree*>(src_levels[current_level]->branch[branch_indices[current_level]]);
						current_level++;
					}
				}
			}
		}

		delete[] dst_levels;
		delete[] branch_indices;
	}

	typedef bmap_fstream<9> occ_map512x512;
}