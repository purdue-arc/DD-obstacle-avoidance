#pragma once

#include <string>
#include <fstream>
#include <stdio.h>
#include <stdint.h>
#include <iostream>

#include "geometry.hpp"

namespace ocpncy {
	/*
	* A square of bits with a width of 2 ^ n 
		(this is so the tile can be evenly cut in half all the way down to the bit level, 
		which makes certain opertions faster)
	* log2_w: base-2 logarithm of tile width; tile width = w = 2 ^ log2_w = 1 << log2_w
	* log2_w MUST BE GREATER THAN OR EQUAL TO 3!!!
	*	This is because the bit tiles are composed of 8x8 bit squares (stored as unsigned longs),
	*	so the minimum width of the tile is the width of one square, which is 8 or 2 ^ 3
	*/
	template <unsigned int log2_w>
	struct btile {
		std::uint64_t minis[1 << (log2_w - 3)][1 << (log2_w - 3)];
		btile() = default;
	};

	/*
	* Functions for getting or setting an individual bit in a tile
	* Not ideal for iterative access
	*/
	template <unsigned int log2_w>
	inline bool get_bit(int x, int y, const btile<log2_w>* ot) {
		return ((ot->minis[x >> 3][y >> 3]) >> ((x & 0b111) + ((y & 0b111) << 3))) & 0b1;
	}
	template <unsigned int log2_w>
	inline void set_bit(int x, int y, btile<log2_w>* ot, bool value) {
		ot->minis[x >> 3][y >> 3] |= value * (((std::uint64_t) 0b1) << ((x & 0b111) + ((y & 0b111) << 3)));
	}

	template <unsigned int log2_w>
	void PrintTile(const btile<log2_w>& tile) {
		for (int y = (1 << log2_w) - 1; y >= 0; y--) {
			for (int x = 0; x < (1 << log2_w); x++) {
				std::cout << (get_bit(x, y, &tile) ? "@" : ".") << " ";
			}
			std::cout << std::endl;
		}
	}

	struct btile_tree { void* branch[4]; btile_tree() = default; };

	/*
	* item_ptr: is a btile_tree* if depth > 0 or a btile* if depth == 0
	* depth: number of layers below layer of the tree parameter (depth at root of tree = #layers - 1; 
																 depth at base of tree = 0)
	*/
	template <unsigned int log2_w>
	void delete_bmap_item(void* item_ptr, unsigned int depth) {
		if (depth > 0 && item_ptr) {
			for (int i = 0; i < 4; i++)
				delete_bmap_item<log2_w>(static_cast<btile_tree*>(item_ptr)->branch[i], depth - 1);
			delete static_cast<btile_tree*>(item_ptr);
		}
		else delete static_cast<btile<log2_w>*>(item_ptr);
	}

	/*
	* Holds information about a map
	* log2_tile_w: the base-2 logarithm of the tile width (must be >= 3)
	* depth: depth of the map (# of layers in the map above the tile layer)
	* origin: origin (bottom left corner position) of map
	*/
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

	/*
	* Holds main copy of map data
	* All map data is properly disposed of when the map is deleted
	* info: holds the depth and origin of the root
	* root: root item of map (a tree if depth > 0 or tile if depth == 0)
	*/
	template <unsigned int log2_w>
	struct bmap {
		bmap_info<log2_w> info;
		void* root = 0;
		bmap() = default;
		bmap(const gmtry2i::vector2i& origin) {
			info = bmap_info<log2_w>(origin);
			root = new btile<log2_w>();
		}
		bmap(const bmap_info<log2_w> map_info, void* map_root) {
			info = map_info;
			root = map_root;
		}
		~bmap() {
			delete_bmap_item<log2_w>(root, info.depth);
			root = 0;
		}
	};

	/*
	* Temporarily holds information about a part of the map (either a tree or a tile)
	* Holds same data as a map, but it shouldn't be used to hold the primary pointers to things
	*		When deleted, the actual item isn't deleted
	*/
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
		bmap_item(void* item_ptr, gmtry2i::vector2i item_origin, unsigned int item_depth) {
			ptr = item_ptr;
			depth = item_depth;
			origin = item_origin;
		}
	};

	/*
	* Allows for easy iteration over the tiles of a map item
	* reset(): returns the iterator to its initial conditions
	* next(const bmap_item<log2_w>): returns the next tile in the item
	*		Returns 0 if all items have been iterated over
	*		Accepts items of any depth
	* last_origin(): returns the origin of the last tile retrieved from next()
	*/
	template <unsigned int log2_w>
	class bmap_item_iterator {
		private:
			bmap_item<log2_w> item;
			btile_tree** parents;
			gmtry2i::vector2i* origins;
			unsigned int* branch_indices;
			unsigned int current_level = 0;
		public:
			void reset() {
				if (item.depth) parents[0] = static_cast<btile_tree*>(item.ptr);
				origins[0] = item.origin;
				branch_indices[0] = 0;
			}
			bmap_item_iterator(const bmap_item<log2_w> new_item) {
				item = new_item;
				parents = new btile_tree * [item.depth];
				origins = new gmtry2i::vector2i[item.depth + 1];
				branch_indices = new unsigned int[item.depth];
				reset();
			}
			btile<log2_w>* next() {
				if (item.depth == 0) {
					if (branch_indices[0]) return 0;
					else {
						branch_indices[0] = 1;
						return static_cast<btile<log2_w>*>(item.ptr);
					}
				}
				unsigned int current_depth;
				while (branch_indices[0] < 4) {
					if (branch_indices[current_level] > 3) branch_indices[--current_level]++;
					else {
						if (parents[current_level]->branch[branch_indices[current_level]]) {
							current_depth = item.depth - current_level;
							origins[current_level + 1] = origins[current_level]
								+ gmtry2i::vector2i(branch_indices[current_level] & 1, branch_indices[current_level] >> 1)
								* (1 << (current_depth + log2_w - 1));
							if (current_depth == 1) {
								btile<log2_w>* tile = static_cast<btile<log2_w>*>(parents[current_level]->branch[branch_indices[current_level]]);
								branch_indices[current_level]++;
								return tile;
							}
							else {
								parents[current_level + 1] = static_cast<btile_tree*>(parents[current_level]->branch[branch_indices[current_level]]);
								branch_indices[current_level + 1] = 0;
								current_level++;
							}
						}
						else branch_indices[current_level]++;
					}
				}
				return 0;
			}
			gmtry2i::vector2i last_origin() {
				return origins[current_level + 1];
			}
			~bmap_item_iterator() {
				delete[] parents;
				delete[] origins;
				delete[] branch_indices;
			}
	};

	template <unsigned int log2_w>
	inline gmtry2i::aligned_box2i get_bmap_bounds(const bmap<log2_w>* map) {
		return gmtry2i::aligned_box2i(map->info.origin, 1 << (map->info.depth + log2_w));
	}

	template <unsigned int log2_w>
	inline gmtry2i::aligned_box2i get_bmap_item_bounds(const bmap_item<log2_w>& item) {
		return gmtry2i::aligned_box2i(item.origin, 1 << (item.depth + log2_w));
	}

	/*
	* Prints out a full map item with dividers separating the tiles
	* Prints the item bounds
	*/
	template <unsigned int log2_w>
	void PrintItem(const bmap_item<log2_w>& item) {
		unsigned int item_width = 1 << (item.depth + log2_w);
		char** img = new char*[item_width];
		int x, y;
		for (int y1 = 0; y1 < (1 << item.depth); y1++) {
			y = y1 << log2_w;
			img[y] = new char[2 * item_width + 1];
			for (x = 0; x < item_width * 2; x++) img[y][x] = '-';
			img[y][2 * item_width] = '\0';
			for (int y2 = 1; y2 < (1 << log2_w); y2++) {
				y = (y1 << log2_w) + y2;
				img[y] = new char[2 * item_width + 1];
				for (int x1 = 0; x1 < (1 << item.depth); x1++) {
					x = x1 << log2_w;
					img[y][x * 2] = '|'; img[y][x * 2 + 1] = ' ';
					for (int x2 = 1; x2 < (1 << log2_w); x2++) {
						x = (x1 << log2_w) + x2;
						img[y][x * 2] = '.'; img[y][x * 2 + 1] = ' ';
					}
				}
				img[y][2 * item_width] = '\0';
			}
		}
		gmtry2i::aligned_box2i bounds = get_bmap_item_bounds(item);
		if (gmtry2i::contains(bounds, -item.origin)) 
			img[(-item.origin.y)][2 * (-item.origin.x)] = 'O';
		
		bmap_item_iterator<log2_w> iterator(item);
		btile<log2_w>* tile;
		while (tile = iterator.next())
			for (int y = 0; y < (1 << log2_w); y++)
				for (int x = 0; x < (1 << log2_w); x++)
					if (get_bit(x, y, tile)) img[y + iterator.last_origin().y - item.origin.y]
												[2 * (x + iterator.last_origin().x - item.origin.x)] = '@';

		for (int y = item_width - 1; y >= 0; y--) {
			std::cout << img[y] << std::endl;
			delete[] img[y];
		}
		std::cout << "Item Bounds: " << bounds.min.x << ", " << bounds.min.y << "; " << 
										bounds.max.x << ", " << bounds.max.y << std::endl;
		delete[] img;
	}

	/*
	* Doubles the width of the map, expanding it in the given direction
	*/
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

	/*
	* Fits the position into the map by expanding it if necessary
	*/
	template <unsigned int log2_w>
	void fit_bmap(bmap<log2_w>* map, const gmtry2i::vector2i p) {
		gmtry2i::aligned_box2i alloc_box = get_bmap_bounds(map);
		while (!gmtry2i::contains(alloc_box, p)) {
			expand_bmap(map, p - gmtry2i::center(alloc_box));
			alloc_box = get_bmap_bounds(map);
		}
	}

	/*
	* Fits the box into the map by expanding it if necessary
	*/
	template <unsigned int log2_w>
	void fit_bmap(bmap<log2_w>* map, const gmtry2i::aligned_box2i box) {
		gmtry2i::aligned_box2i alloc_box = get_bmap_bounds(map);
		gmtry2i::vector2i box_center = gmtry2i::center(box);
		while (!gmtry2i::contains(alloc_box, box)) {
			expand_bmap(map, box_center - gmtry2i::center(alloc_box));
			alloc_box = get_bmap_bounds(map);
		}
	}

	/*
	* Returns the virtual item (has no pointer) from the map that most closely fits the given item
	* Returns a null item (all 0s) if the map does not contain the item
	* DOES NOT AUTOMATICALLY MODIFY MAP TO FIT THE ITEM
	*/
	template <unsigned int log2_w>
	bmap_item<log2_w> get_matching_virtual_bmap_item(const bmap_info<log2_w>& map_info, const bmap_item<log2_w>& item) {
		bmap_item<log2_w> virtual_item = bmap_item<log2_w>();
		bmap_item<log2_w> next_virtual_item(0, map_info.origin, map_info.depth);
		unsigned int next_branch_index;
		unsigned int hwidth = 1 << (next_virtual_item.depth + log2_w - 1);
		gmtry2i::aligned_box2i item_box = get_bmap_item_bounds(item);
		while (gmtry2i::contains(get_bmap_item_bounds(next_virtual_item), item_box)) {
			virtual_item = next_virtual_item;
			next_branch_index = (item.origin.x - virtual_item.origin.x >= hwidth) + 2 * (item.origin.y - virtual_item.origin.y >= hwidth);
			next_virtual_item.depth = virtual_item.depth - 1;
			next_virtual_item.origin = virtual_item.origin + gmtry2i::vector2i(next_branch_index & 1, next_branch_index >> 1) * hwidth;
			hwidth >>= 1;
		}
		return virtual_item;
	}

	/*
	* Retrieves the map item closest to a given point at a given depth
	* DOES NOT AUTOMATICALLY MODIFY THE MAP TO FIT THE POINT
	*/
	template <unsigned int log2_w>
	bmap_item<log2_w> get_bmap_item(const bmap<log2_w>* map, const gmtry2i::vector2i& p, unsigned int depth) {
		bmap_item<log2_w> item = bmap_item<log2_w>();
		bmap_item<log2_w> next_item(map->root, map->info.origin, map->info.depth);
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
	}

	/*
	* Allocates an item in the map at the desired position and depth
	* DOES NOT AUTOMATICALLY EXPAND MAP TO FIT THE POINT
	*/
	template <unsigned int log2_w>
	bmap_item<log2_w> alloc_bmap_item(bmap<log2_w>* dst, const gmtry2i::vector2i& p, unsigned int depth) {
		bmap_item<log2_w> item = bmap_item<log2_w>();
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
		if (depth == 0 && item.ptr) {
			delete reinterpret_cast<btile_tree*>(next_item.ptr);
			next_item.ptr = reinterpret_cast<btile_tree*>(item.ptr)->branch[next_branch_index] = new btile<log2_w>();
		}
		return next_item;
	}

	/*
	* Copies all tiles from an item to a map, assuming only alignment on the tile level
	* Automatically expands the map or extends its branches such that it will contain all tiles from the item
	*/
	template <unsigned int log2_w>
	void set_bmap_item(bmap<log2_w>* dst, const bmap_item<log2_w>& src) {
		fit_bmap(dst, get_bmap_item_bounds(src));
		bmap_item<log2_w> virtual_min_dst_item = get_matching_virtual_bmap_item(dst->info, src);
		bmap_item<log2_w> min_dst_item = alloc_bmap_item(dst, virtual_min_dst_item.origin, virtual_min_dst_item.depth);
		bmap<log2_w> min_dst(bmap_info<log2_w>(min_dst_item.depth, min_dst_item.origin), min_dst_item.ptr);
		bmap_item_iterator<log2_w> iterator(src);
		btile<log2_w>* tile;
		btile<log2_w>* map_tile;
		while (tile = iterator.next()) {
			map_tile = static_cast<btile<log2_w>*>(alloc_bmap_item(dst, iterator.last_origin(), 0).ptr);
			*map_tile = *tile;
		}
		min_dst.root = 0;
	}

	/*
	* read(const gmtry2i::vector2i&, btile<log2_w>*): 
	*		Reads tile at given position to the destination
	*		Returns false if no tile contains the position
	* read(const gmtry2i::vector2i&, unsigned int, bmap<log2_w>*):
	*		Reads the pre-existing item, which contains the given position, that is closest to the given depth
	*		Return false if no item contains the position
	* get_bounds(): 
	*		Returns the current bounds of the map
	*/
	template <unsigned int log2_w>
	class bmap_istream { 
		public: 
			virtual bool read(const gmtry2i::vector2i& p, btile<log2_w>* dst) = 0;
			virtual bool read(const gmtry2i::vector2i& p, unsigned int depth, bmap<log2_w>* dst) = 0;
			virtual gmtry2i::aligned_box2i get_bounds() = 0;
	};

	/*
	* write(const gmtry2i::vector2i&, const btile<log2_w>*):
	*		Writes the tile to the given position in the map
	*		Expands or extends map as necessary
	* write(const bmap_item<log2_w>&):
	*		Writes the full item to the map, assuming only tile-level allignment
	*		Expands or extends map as necessary
	*/
	template <unsigned int log2_w>
	class bmap_ostream { 
		public: 
			virtual bool write(const gmtry2i::vector2i& p, const btile<log2_w>* src) = 0;
			virtual bool write(const bmap_item<log2_w>& src) = 0;
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
			/*
			* Both trees and tiles are indexed as an index_tree
			*		A tile's index_tree will not have any nonzero branches
			* All subitems (even those that don't exist) of an item are indexed when any one of them are indexed, 
				saving time in future searches
			* Either none or all of an index tree's branches are indexed
			* pos: position of an item in the file
			* fully_indexed: tells whether the tree and all of its descendents are indexed
			*/
			struct index_tree { 
				unsigned long pos; 
				bool fully_indexed;
				index_tree* branch[4] = { 0 };
				index_tree(unsigned long item_pos) {
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
			/*
			* tree: stores the file position of the item, tells whether the item is fully indexed, and points to children items
			* depth: depth of the item (# of possible layers below it)
			* origin: origin (bottom left corner position) of item
			*/
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

			std::fstream file;
			std::string file_name;
			index_tree* indices;

			inline void read_file(void* dst, unsigned long pos, unsigned long len) {
				file.seekg(pos);
				file.read(static_cast<char*>(dst), len);
			}
			// cannot be used to append
			inline void write_file(const void* src, unsigned long pos, unsigned long len) {
				file.seekp(pos);
				file.write(static_cast<const char*>(src), len);
			}
			inline void append_file(const void* src, unsigned long len) {
				file.seekp(map_header.size);
				file.write(static_cast<const char*>(src), len);
				map_header.size += len;
				std::cout << "Added to file size: " << len << std::endl; //test
				std::cout << "New file size: " << map_header.size << std::endl; //test
			}
			inline void write_branch(unsigned long new_branch, unsigned int branch_index, unsigned long tree_pos) {
				file.seekp(tree_pos + sizeof(new_branch) * branch_index);
				file.write(reinterpret_cast<char*>(&new_branch), sizeof(new_branch));
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
				for (int i = 0; i < 4; i++) if (i != old_root_index) indices->branch[i] = new index_tree(0);

				unsigned long new_root_branches[4] = { 0 };
				new_root_branches[old_root_index] = map_header.root;
				append_file(new_root_branches, 4 * sizeof(unsigned long));

				map_header.root = new_root->pos;
				map_header.info.depth += 1;
				map_header.info.origin -= gmtry2i::vector2i(direction.x < 0, direction.y < 0) * map_init_width;
				std::cout << "Map Expanded!" << std::endl;
				std::cout << "New root: " << map_header.root << std::endl; //test
				std::cout << "New depth: " << map_header.info.depth << std::endl; //test
				std::cout << "New origin: " << map_header.info.origin.x << ", " << map_header.info.origin.y << std::endl; //test
			}
			void fit_map(const gmtry2i::vector2i& p) {
				gmtry2i::aligned_box2i alloc_box = get_map_bounds();
				while (!gmtry2i::contains(alloc_box, p)) {
					expand_map(p - gmtry2i::center(alloc_box));
					alloc_box = get_map_bounds();
					std::cout << "Expanded Map Bounds: " << 
								 alloc_box.min.x << ", " << alloc_box.min.y << "; " << 
								 alloc_box.max.x << ", " << alloc_box.max.y << std::endl; //test
				}
				write_file(&map_header, 0, sizeof(bmap_file_header));
			}
			void fit_map(const gmtry2i::aligned_box2i& box) {
				gmtry2i::aligned_box2i alloc_box = get_map_bounds();
				gmtry2i::vector2i box_center = gmtry2i::center(box);
				while (!gmtry2i::contains(alloc_box, box)) {
					expand_map(box_center - gmtry2i::center(alloc_box));
					alloc_box = get_map_bounds();
				}
				write_file(&map_header, 0, sizeof(bmap_file_header));
			}
			item_index get_top_item() {
				return item_index(indices, map_header.info.depth, map_header.info.origin);
			}
			/*
			* Returns the indexed item under start containing the point and closest to the desired depth
			* Returns a real item if start is real, although it won't contain the point if start doesn't contain the point
			*/
			item_index indexed_item_at_depth(const item_index& start, const gmtry2i::vector2i& p, unsigned int depth) {
				item_index item;
				item_index next_item(start.tree, start.depth, start.origin);
				unsigned int next_branch_idx;
				gmtry2i::vector2i relative_p;
				unsigned int hwidth = 1 << (next_item.depth + log2_w - 1);
				while (next_item.tree && next_item.tree->pos && next_item.depth > depth) {
					item = next_item;
					next_branch_idx = (p.x - item.origin.x >= hwidth) + 2 * (p.y - item.origin.y >= hwidth);
					next_item.tree = item.tree->branch[next_branch_idx];
					next_item.depth = item.depth - 1;
					next_item.origin = item.origin + gmtry2i::vector2i(next_branch_idx & 1, next_branch_idx >> 1) * hwidth;
					hwidth >>= 1;
				}
				return (next_item.tree && next_item.tree->pos) ? next_item : item;
			}
			/*
			* Returns the existing item under start containing the point and closest to the desired depth
			* Returns a real item if start is real, although it won't contain the point if start doesn't contain the point
			*/
			item_index seek_item_at_depth(const item_index& start, const gmtry2i::vector2i& p, unsigned int depth) {
				item_index item;
				item_index next_item = indexed_item_at_depth(start, p, depth);
				unsigned int next_branch_idx;
				unsigned int hwidth = 1 << (next_item.depth + log2_w - 1);
				unsigned long branches[4];
				while (next_item.tree->pos && next_item.depth > depth) {
					item = next_item;
					read_file(branches, item.tree->pos, 4 * sizeof(unsigned long));
					next_branch_idx = (p.x - item.origin.x >= hwidth) + 2 * (p.y - item.origin.y >= hwidth);
					for (int i = 0; i < 4; i++) item.tree->branch[i] = new index_tree(branches[i]);
					next_item.tree = item.tree->branch[next_branch_idx];
					next_item.depth = item.depth - 1;
					next_item.origin = item.origin + gmtry2i::vector2i(next_branch_idx & 1, next_branch_idx >> 1) * hwidth;
					hwidth >>= 1;
				}
				return next_item.tree->pos ? next_item : item;
			}
			/*
			* Allocates an item at the desired position and depth and returns its index
			* Returns a real item if start is real, although it won't contain the point if start doesn't contain the point
			*/
			item_index alloc_item_at_depth(const item_index& start, const gmtry2i::vector2i p, unsigned int depth) {
				item_index item = seek_item_at_depth(start, p, depth);
				if (item.depth == depth) return item;
				unsigned int hwidth = 1 << (item.depth + log2_w - 1);
				unsigned int next_branch_idx = (p.x - item.origin.x >= hwidth) + 2 * (p.y - item.origin.y >= hwidth);
				const unsigned int branches_size = 4 * sizeof(unsigned long);
				// overwrite position of next branch (was 0 before because branch didn't exist)
				write_branch(map_header.size, next_branch_idx, item.tree->pos);
				item.tree = item.tree->branch[next_branch_idx] = new index_tree(map_header.size);
				item.depth -= 1;
				item.origin += gmtry2i::vector2i(next_branch_idx & 1, next_branch_idx >> 1) * hwidth;
				hwidth >>= 1;
				while (item.depth > depth) {
					unsigned long branches[4] = { 0 };
					next_branch_idx = (p.x - item.origin.x >= hwidth) + 2 * (p.y - item.origin.y >= hwidth);
					branches[next_branch_idx] = map_header.size + branches_size;
					append_file(branches, branches_size);
					std::cout << "File appended with new tree!" << std::endl; //test
					for (int i = 0; i < 4; i++) item.tree->branch[i] = new index_tree(branches[i]);
					item.tree = item.tree->branch[next_branch_idx];
					item.depth -= 1;
					item.origin += gmtry2i::vector2i(next_branch_idx & 1, next_branch_idx >> 1) * hwidth;
					hwidth >>= 1;
				}
				if (depth) {
					unsigned long final_branches[4] = { 0 };
					append_file(final_branches, branches_size);
				} 
				else map_header.size += sizeof(btile<log2_w>);
				return item;
			}
			void* build_item(const item_index& item) {
				if (item.tree == 0 || item.tree->pos == 0) return 0;
				if (item.depth == 0) {
					std::cout << "Tile Built!" << std::endl; //test
					btile<log2_w>* tile = new btile<log2_w>;
					read_file(tile, item.tree->pos, sizeof(btile<log2_w>));
					return tile;
				}
				else {
					if (item.tree->branch[0] == 0) {
						std::cout << "Tree Being Indexed" << std::endl; //test
						unsigned long branches[4] = { 0 };
						read_file(branches, item.tree->pos, 4 * sizeof(unsigned long));
						for (int i = 0; i < 4; i++) item.tree->branch[i] = new index_tree(branches[i]);
					}
					btile_tree* item_tree = new btile_tree;
					for (int i = 0; i < 4; i++) item_tree->branch[i] = 
						build_item(item_index(item.tree->branch[i], item.depth - 1, gmtry2i::vector2i()));
					return item_tree;
				}
			}

		public:
			bmap_fstream(const std::string& fname, const gmtry2i::vector2i& origin) {
				file_name = fname;
				file.open(file_name);
				if (!(file.is_open())) {
					FILE* tmp_file = 0;
					fopen_s(&tmp_file, file_name.c_str(), "w");
					fclose(tmp_file);
					file.open(file_name);
					map_header.info = bmap_info<log2_w>(origin);
					map_header.root = sizeof(bmap_file_header);
					map_header.size = sizeof(bmap_file_header);
					write_file(&map_header, 0, sizeof(bmap_file_header));
					btile<log2_w> tile0 = btile<log2_w>();
					append_file(&tile0, sizeof(tile0));
					std::cout << "File created!" << std::endl; //test
				}
				else {
					read_file(&map_header, 0, sizeof(map_header));
					std::cout << "File already exists!" << std::endl; //test
				}
				if (!(file.is_open())) throw - 1;


				std::cout << "Recorded log2_tile_w: " << map_header.info.log2_tile_w << std::endl; //test
				std::cout << "Recorded depth: " << map_header.info.depth << std::endl; //test
				std::cout << "Recorded origin: " << map_header.info.origin.x << ", " << map_header.info.origin.y << std::endl; //test
				std::cout << "Recorded root: " << map_header.root << std::endl; //test
				std::cout << "Recorded file length: " << map_header.size << std::endl; //test


				if (log2_w != map_header.info.log2_tile_w) throw - 2;
				if (map_header.size < sizeof(bmap_file_header)) throw - 4;

				indices = new index_tree(map_header.root);
			}
			bool read(const gmtry2i::vector2i& p, btile<log2_w>* dst) {
				if (!gmtry2i::contains(get_map_bounds(), p)) return false;
				item_index deepest_item = seek_item_at_depth(get_top_item(), p, 0);
				if (deepest_item.depth == 0) {
					read_file(dst, deepest_item.tree->pos, sizeof(btile<log2_w>));
					return true;
				} 
				else return false;
			}
			bool read(const gmtry2i::vector2i& p, unsigned int depth, bmap<log2_w>* dst) {
				if (!gmtry2i::contains(get_map_bounds(), p)) return false;
				item_index deepest_item = seek_item_at_depth(get_top_item(), p, depth);
				if (deepest_item.depth == depth) {
					delete_bmap_item<log2_w>(dst->root, dst->info.depth);
					dst->info = bmap_info<log2_w>(deepest_item.depth, deepest_item.origin);
					dst->root = build_item(deepest_item);
					return true;
				}
				else return false;
			}
			gmtry2i::aligned_box2i get_bounds() { return get_map_bounds(); }
			bool write(const gmtry2i::vector2i& p, const btile<log2_w>* src) {
				if (!(file.is_open())) return false;
				fit_map(p);
				item_index tile = alloc_item_at_depth(get_top_item(), p, 0);
				write_file(src, tile.tree->pos, sizeof(btile<log2_w>));
				std::cout << "File appended with new tile!" << std::endl; //test
				return true;
			}
			bool write(const bmap_item<log2_w>& src) {
				if (!(file.is_open())) return false;
				fit_map(get_bmap_item_bounds(src));
				bmap_item<log2_w> virtual_min_dst = get_matching_virtual_bmap_item(map_header.info, src);
				item_index min_dst = alloc_item_at_depth(get_top_item(), virtual_min_dst.origin, virtual_min_dst.depth);
				bmap_item_iterator<log2_w> iterator(src);
				btile<log2_w>* tile;
				item_index map_tile;
				while (tile = iterator.next()) {
					map_tile = alloc_item_at_depth(min_dst, iterator.last_origin(), 0);
					write_file(tile, map_tile.tree->pos, sizeof(btile<log2_w>));
				}
				return true;
			}
			~bmap_fstream() {
				if (file.is_open()) {
					write_file(&map_header, 0, sizeof(bmap_file_header));
					file.close();
				}
				delete_index_tree(indices, map_header.info.depth);
			}
	};
}