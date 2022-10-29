#pragma once

#include <string>
#include <fstream>
#include <stdio.h>
#include <stdint.h>
#include <iostream>

#include "geometry.hpp"

namespace ocpncy {

	typedef std::uint64_t btile_mini;
	/*
	* A square of bits with a width of 2 ^ n 
		(this is so the tile can be evenly cut in half all the way down to the bit level, 
		which makes certain opertions faster)
	* log2_w: base-2 logarithm of tile width; tile width = w = 2 ^ log2_w = 1 << log2_w
	* log2_w MUST BE GREATER THAN OR EQUAL TO 3!!!
	*	This is because the bit tiles are composed of 8x8 bit squares (stored as 64-bit integers),
	*	so the minimum width of the tile is the width of one square, which is 8 or 2 ^ 3
	*/
	template <unsigned int log2_w>
	struct btile {
		btile_mini minis[1 << (log2_w - 3)][1 << (log2_w - 3)];
		btile() = default;
		inline btile& operator +=(const btile& tile) {
			for (int x = 0; x < 1 << (log2_w - 3); x++)
				for (int y = 0; y < 1 << (log2_w - 3); y++)
					minis[x][y] |= tile.minis[x][y];
			return *this;

		}
		inline operator bool() const {
			bool exists = false;
			for (int x = 0; x < 1 << (log2_w - 3); x++)
				for (int y = 0; y < 1 << (log2_w - 3); y++)
					exists |= minis[x][y];
			return exists;
		}
	};

	enum btile_write_mode {
		BTILE_OVERWRITE_MODE = 0,
		BTILE_ADD_MODE = 1
	};

	template <unsigned int log2_w>
	btile<log2_w> operator +(const btile<log2_w>& t1, const btile<log2_w>& t2) {
		btile<log2_w> sum;
		for (int x = 0; x < 1 << (log2_w - 3); x++)
			for (int y = 0; y < 1 << (log2_w - 3); y++)
				sum.minis[x][y] = t1.minis[x][y] | t2.minis[x][y];
		return sum;
	}

	template <unsigned int log2_w>
	btile<log2_w> operator ^(const btile<log2_w>& t1, const btile<log2_w>& t2) {
		btile<log2_w> dif;
		for (int x = 0; x < 1 << (log2_w - 3); x++)
			for (int y = 0; y < 1 << (log2_w - 3); y++)
				dif.minis[x][y] = t1.minis[x][y] ^ t2.minis[x][y];
		return dif;
	}

	template <unsigned int log2_w>
	btile<log2_w> operator -(const btile<log2_w>& t1, const btile<log2_w>& t2) {
		btile<log2_w> dif;
		for (int x = 0; x < 1 << (log2_w - 3); x++)
			for (int y = 0; y < 1 << (log2_w - 3); y++)
				dif.minis[x][y] = t2.minis[x][y] & (~(t1.minis[x][y] & t2.minis[x][y]));
		return dif;
	}

	/*
	* Functions for getting or setting an individual bit in a tile
	*/
	template <unsigned int log2_w>
	inline bool get_bit(int x, int y, const btile<log2_w>& ot) {
		return ((ot.minis[x >> 3][y >> 3]) >> ((x & 0b111) + ((y & 0b111) << 3))) & 0b1;
	}
	template <unsigned int log2_w>
	inline void set_bit(int x, int y, btile<log2_w>& ot, bool value) {
		ot.minis[x >> 3][y >> 3] |= value * (((btile_mini) 0b1) << ((x & 0b111) + ((y & 0b111) << 3)));
	}

	template <unsigned int log2_w>
	void PrintTile(const btile<log2_w>& tile) {
		for (int y = (1 << log2_w) - 1; y >= 0; y--) {
			for (int x = 0; x < (1 << log2_w); x++) {
				std::cout << (get_bit(x, y, tile) ? "@" : ".") << " ";
			}
			std::cout << std::endl;
		}
	}

	struct bimage {
		unsigned int width;
		char** buf;
		char out_of_bounds_pixel;
		gmtry2i::vector2i origin;
		std::string caption;
		gmtry2i::aligned_box2i bounds;
		char& operator ()(gmtry2i::vector2i p) {
			if (gmtry2i::contains(bounds, p)) return buf[p.y - origin.y][2 * (p.x - origin.x)];
			else return out_of_bounds_pixel;
		}
		char& operator ()(unsigned int x, unsigned int y) {
			if (x < width && y < width) return buf[y][2 * x];
			else return out_of_bounds_pixel;
		}
		const char* line(int i) const {
			if (i < width) return buf[width - (i + 1)];
			else return &out_of_bounds_pixel;
		}
		bimage(unsigned int log2_w, unsigned int log2_tw, gmtry2i::vector2i new_origin) {
			origin = new_origin;
			width = 1 << (log2_tw + log2_w);
			buf = new char* [width];
			int x, y;
			for (int y1 = 0; y1 < (1 << log2_tw); y1++) {
				y = y1 << log2_w;
				buf[y] = new char[2 * width + 1];
				for (x = 0; x < width * 2; x++) buf[y][x] = '-';
				buf[y][2 * width] = '\0';
				for (int y2 = 1; y2 < (1 << log2_w); y2++) {
					y = (y1 << log2_w) + y2;
					buf[y] = new char[2 * width + 1];
					for (int x1 = 0; x1 < (1 << log2_tw); x1++) {
						x = x1 << log2_w;
						buf[y][x * 2] = '|'; buf[y][x * 2 + 1] = ' ';
						for (int x2 = 1; x2 < (1 << log2_w); x2++) {
							x = (x1 << log2_w) + x2;
							buf[y][x * 2] = '.'; buf[y][x * 2 + 1] = ' ';
						}
					}
					buf[y][2 * width] = '\0';
				}
			}
			bounds = gmtry2i::aligned_box2i(origin, width);
			(*this)(gmtry2i::vector2i()) = 'O';
			caption = std::string("Image Bounds: ") + std::to_string(bounds.min.x) + std::string(", ") + std::to_string(bounds.min.y) + 
								  std::string("; ") + std::to_string(bounds.max.x) + std::string(", ") + std::to_string(bounds.max.y);
		}
		~bimage() {
			for (int y = width - 1; y >= 0; y--) delete[] buf[y];
			delete[] buf;
		}
	};
	template <unsigned int log2_w>
	void WriteImage(bimage& img, const gmtry2i::vector2i& tile_origin, const btile<log2_w>& tile) {
		for (int y = 0; y < (1 << log2_w); y++)
			for (int x = 0; x < (1 << log2_w); x++)
				if (get_bit(x, y, tile)) img(gmtry2i::vector2i(x, y) + tile_origin) = '@';
	}
	void PrintImage(const bimage& img) {
		for (int y = 0; y < img.width; y++) std::cout << img.line(y) << std::endl;
		std::cout << img.caption << std::endl;
	}
	
	/*
	* Allows for easy iteration over a stream of tiles
	* reset(): returns the stream to its initial conditions
	* next(): returns the next tile in the stream
	*		Returns 0 if all tiles have been retrieved over
	* last_origin(): returns the origin of the last tile retrieved from next()
	*		Behavior undefined when next() has not been called first
	* DOES NOT AUTOMATICALLY DELETE SOURCE MATERIAL
	*/
	template <unsigned int log2_w>
	class btile_istream {
		public:
			virtual void reset() = 0;
			virtual btile<log2_w>* next() = 0;
			virtual gmtry2i::vector2i last_origin() = 0;
			virtual gmtry2i::aligned_box2i get_bounds() = 0;
	};

	template <unsigned int log2_w>
	void WriteImage(bimage& img, btile_istream<log2_w>* tiles) {
		btile<log2_w>* tile;
		while (tile = tiles->next()) 
			if (gmtry2i::contains(img.bounds, tiles->last_origin())) 
				WriteImage(img, tiles->last_origin(), *tile);
	}

	template <unsigned int log2_w, typename T>
	class occ_mat_iterator : public btile_istream<log2_w> {
		private:
			T* mat;
			unsigned int dims[2];
			gmtry2i::vector2i origin;
			btile<log2_w> last_tile;
			// all geometric objects below are positioned relative to mat_bounds
			gmtry2i::aligned_box2i mat_bounds;
			gmtry2i::vector2i tilewise_origin;
			gmtry2i::vector2i tile_origin;
			gmtry2i::vector2i last_tile_origin;
		public:
			void reset() {
				tile_origin = tilewise_origin;
			}
			occ_mat_iterator(T* T_mat, unsigned int width, unsigned int height, 
							 const gmtry2i::vector2i& mat_origin, const gmtry2i::vector2i& any_tile_origin) {
				mat = T_mat;
				dims[0] = width;
				dims[1] = height;
				origin = mat_origin;
				mat_bounds = gmtry2i::aligned_box2i(gmtry2i::vector2i(), gmtry2i::vector2i(width, height));
				tilewise_origin = ((((mat_origin - any_tile_origin) >> log2_w) << log2_w) + any_tile_origin) - mat_origin;
				reset();
			}
			btile<log2_w>* next() {
				if (tile_origin.y >= mat_bounds.max.y) return 0;
				btile<log2_w> tile = btile<log2_w>();
				gmtry2i::aligned_box2i readbox = gmtry2i::intersection(mat_bounds, gmtry2i::aligned_box2i(tile_origin, 1 << log2_w));
				for (int x = readbox.min.x; x < readbox.max.x; x++)
					for (int y = readbox.min.y; y < readbox.max.y; y++)
						if (mat[x + y * dims[0]])
							set_bit(x - tile_origin.x, y - tile_origin.y, tile, true);
				last_tile_origin = tile_origin;
				tile_origin.x += 1 << log2_w;
				if (tile_origin.x >= mat_bounds.max.x) {
					tile_origin.x = tilewise_origin.x;
					tile_origin.y += 1 << log2_w;
				}
				last_tile = tile;
				return &last_tile;
			}
			gmtry2i::vector2i last_origin() {
				return last_tile_origin + origin;
			}
			gmtry2i::aligned_box2i get_bounds() {
				return mat_bounds + origin;
			}
	};

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
		bmap() {
			info = bmap_info<log2_w>(gmtry2i::vector2i());
			root = new btile<log2_w>();
		}
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
		gmtry2i::vector2i origin;
		unsigned int depth;
		bmap_item() = default;
		bmap_item(bmap<log2_w>* map) {
			ptr = map->root;
			origin = map->info.origin;
			depth = map->info.depth;
		}
		bmap_item(gmtry2i::vector2i item_origin, unsigned int item_depth) {
			ptr = 0;
			origin = item_origin;
			depth = item_depth;
		}
		bmap_item(void* item_ptr, gmtry2i::vector2i item_origin, unsigned int item_depth) {
			ptr = item_ptr;
			origin = item_origin;
			depth = item_depth;
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

	template <unsigned int log2_w>
	class bmap_item_iterator : public btile_istream<log2_w> {
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
						branch_indices[0]++;
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
			gmtry2i::aligned_box2i get_bounds() {
				return get_bmap_item_bounds(item);
			}
			~bmap_item_iterator() {
				for (int i = 0; i < item.depth; i++) parents[i] = 0;
				delete[] parents;
				delete[] origins;
				delete[] branch_indices;
			}
	};

	/*
	* Prints out a full map item and a report of its bounds
	*/
	template <unsigned int log2_w>
	void PrintItem(const bmap_item<log2_w>& item) {
		bimage img(log2_w, item.depth, item.origin);
		bmap_item_iterator<log2_w> iterator(item);
		WriteImage(img, &iterator);
		PrintImage(img);
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
	* Returns the virtual item (has no pointer) from the map that most closely fits the item with the given bounds
	* Returns a null item (all 0s) if the map does not contain the item's boundary box
	* DOES NOT AUTOMATICALLY MODIFY MAP TO FIT THE ITEM
	*/
	template <unsigned int log2_w>
	bmap_item<log2_w> get_matching_virtual_bmap_item(const bmap_info<log2_w>& map_info, const gmtry2i::aligned_box2i& item_box) {
		bmap_item<log2_w> virtual_item = bmap_item<log2_w>();
		bmap_item<log2_w> next_virtual_item(map_info.origin, map_info.depth);
		unsigned int next_branch_index;
		unsigned int hwidth = 1 << (next_virtual_item.depth + log2_w - 1);
		while (gmtry2i::contains(get_bmap_item_bounds(next_virtual_item), item_box)) {
			virtual_item = next_virtual_item;
			next_branch_index = (item_box.min.x - virtual_item.origin.x >= hwidth) + 2 * (item_box.min.y - virtual_item.origin.y >= hwidth);
			next_virtual_item.depth = virtual_item.depth - 1;
			next_virtual_item.origin = virtual_item.origin + gmtry2i::vector2i(next_branch_index & 1, next_branch_index >> 1) * hwidth;
			hwidth >>= 1;
		}
		return virtual_item;
	}

	/*
	* Retrieves the map item closest to a given point at a given depth
	* DOES NOT AUTOMATICALLY MODIFY THE MAP
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
	* Allocates an item in the map at the desired position and depth and retrieves it
	* Extends branches as necessary
	* DOES NOT AUTOMATICALLY EXPAND MAP TO FIT THE POINT
	*/
	template <unsigned int log2_w>
	bmap_item<log2_w> alloc_bmap_item(bmap<log2_w>* map, const gmtry2i::vector2i& p, unsigned int depth) {
		bmap_item<log2_w> item = bmap_item<log2_w>();
		bmap_item<log2_w> next_item = get_bmap_item(map, p, depth);
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
	void set_bmap_item(bmap<log2_w>* dst, btile_istream<log2_w>* src, btile_write_mode mode) {
		fit_bmap(dst, src->get_bounds());
		bmap_item<log2_w> virtual_min_dst_item = get_matching_virtual_bmap_item(dst->info, src->get_bounds());
		bmap_item<log2_w> min_dst_item = alloc_bmap_item(dst, virtual_min_dst_item.origin, virtual_min_dst_item.depth);
		bmap<log2_w> min_dst(bmap_info<log2_w>(min_dst_item.depth, min_dst_item.origin), min_dst_item.ptr);
		btile<log2_w>* tile;
		btile<log2_w>* map_tile;
		while (tile = src->next()) {
			map_tile = static_cast<btile<log2_w>*>(alloc_bmap_item(dst, src->last_origin(), 0).ptr);
			if (mode) *map_tile += *tile;
			else *map_tile = *tile;
		}
		min_dst.root = 0;
	}

	template <unsigned int log2_w>
	class bmap_istream { 
		public: 
			/*
			* Reads tile at given position to the destination
			* Returns false if no tile contains the position
			*/
			virtual bool read(const gmtry2i::vector2i& p, btile<log2_w>* dst) = 0;

			/*
			* Reads the pre-existing item, which contains the given position, that is closest to the given depth
			* Return false if no item contains the position
			*/
			virtual bool read(const gmtry2i::vector2i& p, unsigned int depth, bmap<log2_w>* dst) = 0;

			/*
			* Retrieves a reference to the stream's read mode (whether to add to or overwrite the map being read to)
			*/
			virtual btile_write_mode& read_mode() = 0;

			/*
			* Returns the current bounds of the map
			*/
			virtual gmtry2i::aligned_box2i get_bounds() = 0;
	};

	template <unsigned int log2_w>
	class bmap_ostream { 
		public: 
			/*
			* Writes the tile to the given position in the map
			* Expands or extends map as necessary
			*/
			virtual bool write(const gmtry2i::vector2i& p, const btile<log2_w>* src) = 0;

			/*
			* Writes every tile from the tile stream to the map, assuming only tile-level allignment
			* Expands or extends map as necessary
			*/
			virtual bool write(btile_istream<log2_w>* src) = 0;

			/*
			* Retrieves a reference to the stream's write mode (whether to add to or overwrite the map being written to)
			*/
			virtual btile_write_mode& write_mode() = 0;
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
			* pos: position of the represented item in the file
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
			btile_write_mode readmode;
			btile_write_mode writemode;

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
			inline void write_tile(const btile<log2_w>* src, unsigned long pos) {
				if (writemode) {
					btile<log2_w> current_tile = btile<log2_w>();
					read_file(&current_tile, pos, sizeof(btile<log2_w>));
					current_tile += *src;
					write_file(&current_tile, pos, sizeof(btile<log2_w>));
				}
				else write_file(src, pos, sizeof(btile<log2_w>));
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
				else if (writemode) {
					btile<log2_w> blanktile = btile<log2_w>();
					append_file(&blanktile, sizeof(blanktile));
				}
				else map_header.size += sizeof(btile<log2_w>);
				return item;
			}
			void* build_item(const item_index& item) {
				if (item.tree == 0 || item.tree->pos == 0) return 0;
				if (item.depth == 0) {
					std::cout << "Tile Built!" << std::endl; //test
					btile<log2_w>* tile = new btile<log2_w>();
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

				readmode = BTILE_OVERWRITE_MODE;
				writemode = BTILE_OVERWRITE_MODE;
				indices = new index_tree(map_header.root);
			}
			bool read(const gmtry2i::vector2i& p, btile<log2_w>* dst) {
				if (!gmtry2i::contains(get_map_bounds(), p)) return false;
				item_index deepest_item = seek_item_at_depth(get_top_item(), p, 0);
				if (deepest_item.depth == 0) {
					if (readmode) {
						btile<log2_w> current_tile = btile<log2_w>();
						read_file(&current_tile, deepest_item.tree->pos, sizeof(btile<log2_w>));
						*dst += current_tile;
					}
					else read_file(dst, deepest_item.tree->pos, sizeof(btile<log2_w>));
					return true;
				} 
				else return false;
			}
			bool read(const gmtry2i::vector2i& p, unsigned int depth, bmap<log2_w>* dst) {
				if (!gmtry2i::contains(get_map_bounds(), p)) return false;
				item_index deepest_item = seek_item_at_depth(get_top_item(), p, depth);
				if (deepest_item.depth == depth) {
					bmap_item_iterator<log2_w> iterator(bmap_item<log2_w>(build_item(deepest_item), deepest_item.origin, deepest_item.depth));
					set_bmap_item(dst, &iterator, readmode);
					return true;
				}
				else return false;
			}
			bool write(const gmtry2i::vector2i& p, const btile<log2_w>* src) {
				if (!(file.is_open())) return false;
				fit_map(p);
				item_index tile = alloc_item_at_depth(get_top_item(), p, 0);
				write_tile(src, tile.tree->pos);
				std::cout << "File appended with new tile!" << std::endl; //test
				return true;
			}
			bool write(btile_istream<log2_w>* src) {
				if (!(file.is_open())) return false;
				fit_map(src->get_bounds());
				bmap_item<log2_w> virtual_min_dst = get_matching_virtual_bmap_item(map_header.info, src->get_bounds());
				item_index min_dst = alloc_item_at_depth(get_top_item(), virtual_min_dst.origin, virtual_min_dst.depth);
				btile<log2_w>* tile;
				item_index map_tile;
				while (tile = src->next()) if (*tile) {
					map_tile = alloc_item_at_depth(min_dst, src->last_origin(), 0);
					write_tile(tile, map_tile.tree->pos);
				}
				return true;
			}
			gmtry2i::aligned_box2i get_bounds() { 
				return get_map_bounds(); 
			}
			btile_write_mode& read_mode() {
				return readmode;
			}
			btile_write_mode& write_mode() {
				return writemode;
			}
			~bmap_fstream() {
				if (file.is_open()) {
					write_file(&map_header, 0, sizeof(bmap_file_header));
					file.close();
				}
				delete_index_tree(indices, map_header.info.depth);
			}
	};

	/*
	* TODO
	*	Make branch positions and map file header have a platform-independent size
	*/
}