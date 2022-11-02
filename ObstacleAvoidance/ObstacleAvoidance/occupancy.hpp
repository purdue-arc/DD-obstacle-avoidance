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
	};

	enum btile_write_mode {
		BTILE_OVERWRITE_MODE = 0,
		BTILE_ADD_MODE = 1
	};

	template <unsigned int log2_w>
	bool exists(const btile<log2_w>& t) {
		bool exists = false;
		for (int x = 0; x < 1 << (log2_w - 3); x++)
			for (int y = 0; y < 1 << (log2_w - 3); y++)
				exists |= t.minis[x][y];
		return exists;
	}

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

	// t1 is the known tile
	// t2 is purely observed
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

	template <unsigned int log2_w>
	inline gmtry2i::vector2i align_down(const gmtry2i::vector2i& p, const gmtry2i::vector2i any_tile_origin) {
		return (((p - any_tile_origin) >> log2_w) << log2_w) + any_tile_origin;
	}

	template <unsigned int log2_w>
	inline gmtry2i::vector2i align_up(const gmtry2i::vector2i& p, const gmtry2i::vector2i any_tile_origin) {
		gmtry2i::vector2i dif = p - any_tile_origin;
		gmtry2i::vector2i shifted_dif = dif >> log2_w;
		return ((shifted_dif + (dif - (shifted_dif << log2_w) > 0)) << log2_w) + any_tile_origin;
	}

	template <unsigned int log2_w>
	inline gmtry2i::aligned_box2i align_out(const gmtry2i::aligned_box2i& b, const gmtry2i::vector2i any_tile_origin) {
		return gmtry2i::aligned_box2i(align_down<log2_w>(b.min, any_tile_origin), align_up<log2_w>(b.max, any_tile_origin));
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
	* Allows for easy iteration over a source of tiles
	* Might contain nothing
	* DOES NOT AUTOMATICALLY DELETE TILE SOURCE
	*/
	template <unsigned int log2_w>
	class btile_stream {
		public:
			// Returns the stream to its initial conditions
			virtual void reset() = 0;
			// Returns the next tile in the stream, or 0 if all tiles have been retrieved
			virtual btile<log2_w>* next() = 0;
			// Returns the origin of the last tile retrieved from next()
			virtual gmtry2i::vector2i last_origin() = 0;
			// Returns a bounds on all of the outgoing tiles
			virtual gmtry2i::aligned_box2i get_bounds() = 0;
			// Sets a bounds on the outgoing tiles. New bounds will automatically be tile-aligned if necessary
			virtual void set_bounds(const gmtry2i::aligned_box2i& new_bounds) = 0;
			virtual ~btile_stream() {};
	};

	template <unsigned int log2_w>
	void WriteImage(bimage& img, btile_stream<log2_w>* tiles) {
		btile<log2_w>* tile;
		while (tile = tiles->next()) 
			if (gmtry2i::contains(img.bounds, tiles->last_origin())) 
				WriteImage(img, tiles->last_origin(), *tile);
	}

	template <unsigned int log2_w, typename T>
	class mat_tile_stream : public btile_stream<log2_w> {
		private:
			T* mat;
			unsigned int dims[2];
			gmtry2i::vector2i origin;
			btile<log2_w> last_tile;
			// all geometric objects below are positioned relative to origin
			gmtry2i::aligned_box2i mat_bounds;
			gmtry2i::aligned_box2i bounds;
			gmtry2i::vector2i tilewise_origin;
			gmtry2i::vector2i tile_origin;
			gmtry2i::vector2i last_tile_origin;
		public:
			void reset() {
				tile_origin = tilewise_origin;
			}
			mat_tile_stream(T* T_mat, unsigned int width, unsigned int height, 
							 const gmtry2i::vector2i& mat_origin, const gmtry2i::vector2i& any_tile_origin) {
				mat = T_mat;
				dims[0] = width;
				dims[1] = height;
				origin = mat_origin;
				mat_bounds = gmtry2i::aligned_box2i(gmtry2i::vector2i(), gmtry2i::vector2i(width, height));
				bounds = mat_bounds;
				tilewise_origin = align_down<log2_w>(mat_origin, any_tile_origin) - mat_origin;
				// = align_down<log2_w>(bounds.min, any_tile_origin - mat_origin);
				reset();
			}
			btile<log2_w>* next() {
				if (tile_origin.y >= bounds.max.y) return 0;
				btile<log2_w> tile = btile<log2_w>();
				gmtry2i::aligned_box2i readbox = gmtry2i::intersection(bounds, gmtry2i::aligned_box2i(tile_origin, 1 << log2_w));
				for (int x = readbox.min.x; x < readbox.max.x; x++)
					for (int y = readbox.min.y; y < readbox.max.y; y++)
						if (mat[x + y * dims[0]])
							set_bit(x - tile_origin.x, y - tile_origin.y, tile, true);
				last_tile_origin = tile_origin;
				tile_origin.x += 1 << log2_w;
				if (tile_origin.x >= bounds.max.x) {
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
				return bounds + origin;
			}
			void set_bounds(const gmtry2i::aligned_box2i& new_bounds) {
				bounds = gmtry2i::intersection(mat_bounds, new_bounds - origin);
				tilewise_origin = align_down<log2_w>(bounds.min, tilewise_origin);
			}
			~mat_tile_stream() { }
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
	* Holds information about a map item
	* depth: depth of the map (# of layers in the map above the tile layer)
	* origin: origin (bottom left corner position) of map
	*/
	struct bmap_item_info {
		gmtry2i::vector2i origin;
		unsigned int depth;
		bmap_item_info() {
			origin = gmtry2i::vector2i();
			depth = 0;
		}
		bmap_item_info(const gmtry2i::vector2i map_origin) {
			origin = map_origin;
			depth = 0;
		}
		bmap_item_info(const gmtry2i::vector2i map_origin, unsigned int map_depth) {
			origin = map_origin;
			depth = map_depth;
		}
	};

	/*
	* Holds main copy of map data
	* All map data is properly disposed of when the map is deleted
	* info: holds the depth and origin of the root
	* root: non-null btile_tree pointer
	*/
	template <unsigned int log2_w>
	struct bmap {
		bmap_item_info info;
		btile_tree* root;
		bmap() {
			info = bmap_item_info(gmtry2i::vector2i(), 1);
			root = new btile_tree();
		}
		bmap(const gmtry2i::vector2i& origin) {
			info = bmap_item_info(origin, 1);
			root = new btile_tree();
		}
		~bmap() {
			delete_bmap_item<log2_w>(root, info.depth);
		}
	};

	/*
	* Temporarily holds information about a part of the map (either a tree or a tile)
	* Holds same data as a map, but it shouldn't be used to hold the primary pointers to things
	*		When deleted, the actual item isn't deleted
	* Ptr is always non-null, and the item it points to can have any depth
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
	inline gmtry2i::aligned_box2i get_bmap_item_bounds(const bmap_item<log2_w>& item) {
		return gmtry2i::aligned_box2i(item.origin, 1 << (item.depth + log2_w));
	}

	template <unsigned int log2_w>
	inline gmtry2i::aligned_box2i get_bmap_item_bounds(const bmap_item_info& info) {
		return gmtry2i::aligned_box2i(info.origin, 1 << (info.depth + log2_w));
	}

	template <unsigned int log2_w>
	class bmap_tile_stream : public btile_stream<log2_w> {
		protected:
			bmap_item_info info;
			void** items;
			gmtry2i::vector2i* origins;
			unsigned int* branch_indices;
			unsigned int current_level;
			gmtry2i::aligned_box2i bounds;
			void update_next_item() {
				items[current_level + 1] = static_cast<btile_tree*>(items[current_level])->branch[branch_indices[current_level]];
			}
			btile<log2_w>* get_tile(void* item) {
				return static_cast<btile<log2_w>*>(item);
			}
		public:
			void reset() {
				origins[0] = info.origin;
				branch_indices[0] = 0; 
				current_level = 0;
			}
			bmap_tile_stream(const bmap_item<log2_w>& item) {
				info.origin = item.origin;
				info.depth = item.depth;
				items = new void * [item.depth + 1];
				items[0] = item.ptr;
				origins = new gmtry2i::vector2i[item.depth + 1];
				branch_indices = new unsigned int[item.depth];
				bounds = get_bmap_item_bounds(item);
				reset();
			}
			btile<log2_w>* next() {
				if (info.depth == 0) {
					if (branch_indices[0]) return 0;
					else {
						branch_indices[0]++;
						return get_tile(items[0]);
					}
				}
				unsigned int current_depth;
				while (branch_indices[0] < 4) {
					if (branch_indices[current_level] > 3) branch_indices[--current_level]++;
					else {
						current_depth = info.depth - current_level;
						origins[current_level + 1] = origins[current_level]
							+ gmtry2i::vector2i(branch_indices[current_level] & 1, branch_indices[current_level] >> 1)
							* (1 << (current_depth + log2_w - 1));
						update_next_item();
						if (items[current_level + 1] && gmtry2i::area(gmtry2i::intersection(bounds,
														gmtry2i::aligned_box2i(origins[current_level + 1], 1 << (current_depth + log2_w))))) {
							if (current_depth == 1) {
								btile<log2_w>* tile = get_tile(items[current_level + 1]);
								branch_indices[current_level]++;
								/*
								std::cout << "Tile streamed out at " << gmtry2i::to_string(origins[current_level + 1]) << std::endl; //test
								PrintTile(*tile); //test
								*/
								return tile;
							}
							else {
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
				return bounds;
			}
			void set_bounds(const gmtry2i::aligned_box2i& new_bounds) {
				bounds = gmtry2i::intersection(get_bmap_item_bounds<log2_w>(info), align_out<log2_w>(new_bounds, info.origin));
			}
			~bmap_tile_stream() {
				for (int i = 0; i < info.depth; i++) items[i] = 0;
				delete[] items;
				delete[] origins;
				delete[] branch_indices;
				std::cout << "Map tile stream deconstructed successfully!" << std::endl; //test
			}
	};

	/*
	* Prints out a full map item and a report of its bounds
	*/
	template <unsigned int log2_w>
	void PrintItem(const bmap_item<log2_w>& item) {
		bimage img(log2_w, item.depth, item.origin);
		bmap_tile_stream<log2_w> iterator(item);
		WriteImage(img, &iterator);
		PrintImage(img);
	}
	template <unsigned int log2_w>
	void PrintItem(btile_stream<log2_w>* stream, unsigned int depth) {
		bimage img(log2_w, depth, stream->get_bounds().min);
		WriteImage(img, stream);
		PrintImage(img);
	}

	/*
	* Doubles the width of the map, expanding it in the given direction
	*/
	template <unsigned int log2_w>
	void expand_bmap(bmap<log2_w>* map, const gmtry2i::vector2i& direction) {
		long map_init_width = 1 << (map->info.depth + log2_w);
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
		gmtry2i::aligned_box2i alloc_box = get_bmap_item_bounds<log2_w>(map->info);
		while (!gmtry2i::contains(alloc_box, p)) {
			expand_bmap(map, p - gmtry2i::center(alloc_box));
			alloc_box = get_bmap_item_bounds<log2_w>(map->info);
		}
	}

	/*
	* Fits the box into the map by expanding it if necessary
	*/
	template <unsigned int log2_w>
	void fit_bmap(bmap<log2_w>* map, const gmtry2i::aligned_box2i box) {
		gmtry2i::aligned_box2i alloc_box = get_bmap_item_bounds<log2_w>(map->info);
		gmtry2i::vector2i box_center = gmtry2i::center(box);
		while (!gmtry2i::contains(alloc_box, box)) {
			expand_bmap(map, box_center - gmtry2i::center(alloc_box));
			alloc_box = get_bmap_item_bounds<log2_w>(map->info);
		}
	}

	/*
	* Returns the info of the virtual item from the map that most closely fits the item with the given bounds
	* Returns a null item info (all 0s) if the map does not contain the item's boundary box
	* DOES NOT AUTOMATICALLY MODIFY MAP TO FIT THE ITEM
	*/
	template <unsigned int log2_w>
	bmap_item_info get_matching_virtual_bmap_item_info(const bmap_item_info& item_info, const gmtry2i::aligned_box2i& item_box) {
		bmap_item_info virtual_item = bmap_item_info();
		bmap_item_info next_virtual_item(item_info.origin, item_info.depth);
		unsigned int next_branch_index;
		unsigned int hwidth = 1 << (next_virtual_item.depth + log2_w - 1);
		while (gmtry2i::contains(get_bmap_item_bounds<log2_w>(next_virtual_item), item_box)) {
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
	bmap_item<log2_w> get_bmap_item(const bmap_item<log2_w>& src, const gmtry2i::vector2i& p, unsigned int depth) {
		bmap_item<log2_w> item = bmap_item<log2_w>();
		bmap_item<log2_w> next_item = src;
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
	* Returns a null item (all 0s) if the map does not contain the item's boundary box
	* DOES NOT AUTOMATICALLY MODIFY MAP TO FIT THE ITEM
	*/
	template <unsigned int log2_w>
	bmap_item<log2_w> alloc_bmap_item(bmap_item<log2_w> src, const gmtry2i::vector2i& p, unsigned int depth) {
		bmap_item<log2_w> item = bmap_item<log2_w>();
		bmap_item<log2_w> next_item = get_bmap_item(src, p, depth);
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
	void set_bmap_tiles(bmap<log2_w>* dst, btile_stream<log2_w>* src, btile_write_mode mode) {
		if (gmtry2i::area(src->get_bounds()) == 0) return;
		fit_bmap(dst, src->get_bounds());
		bmap_item_info virtual_min_dst_item = get_matching_virtual_bmap_item_info<log2_w>(dst->info, src->get_bounds());
		bmap_item<log2_w> min_dst_item = bmap_item<log2_w>(dst);
		bmap_item<log2_w> min_dst = alloc_bmap_item(min_dst_item, virtual_min_dst_item.origin, virtual_min_dst_item.depth);
		btile<log2_w>* tile;
		btile<log2_w>* map_tile;
		while (tile = src->next()) {
			map_tile = static_cast<btile<log2_w>*>(alloc_bmap_item(min_dst, src->last_origin(), 0).ptr);
			if (mode) *map_tile += *tile;
			else *map_tile = *tile;
		}
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
			* Returns false if the position lies outside the map's bounds
			*/
			virtual bool read(const gmtry2i::vector2i& p, unsigned int depth, bmap<log2_w>* dst) = 0;

			/*
			* Writes a tile stream to the destination, which can be used to stream out all tiles from a desired rectangular region of the map
			* TILE STREAM MUST BE DELETED MANUALLY
			*/
			virtual bool read(btile_stream<log2_w>** dst) = 0;

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
			virtual bool write(btile_stream<log2_w>* src) = 0;

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
				bmap_item_info info;
				unsigned int log2_tile_w;
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
			const btile<log2_w> blank = btile<log2_w>();

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
				std::cout << new_branch << " written to branch " << branch_index << " of tree at " << tree_pos << std::endl; //test
				file.seekp(tree_pos + sizeof(new_branch) * branch_index);
				file.write(reinterpret_cast<char*>(&new_branch), sizeof(new_branch));
			}
			inline void write_tile(const btile<log2_w>* src, unsigned long pos) {
				std::cout << "Tile written to file at " << pos << ":" << std::endl; //test
				if (writemode) {
					btile<log2_w> current_tile = btile<log2_w>();
					read_file(&current_tile, pos, sizeof(btile<log2_w>));
					current_tile += *src;
					write_file(&current_tile, pos, sizeof(btile<log2_w>));
					PrintTile(current_tile);
				}
				else {
					write_file(src, pos, sizeof(btile<log2_w>));
					PrintTile(*src);
				}
			}
			inline gmtry2i::aligned_box2i get_map_bounds() {
				return gmtry2i::aligned_box2i(map_header.info.origin, 
					1 << (map_header.info.depth + log2_w));
			}
			void expand_map(const gmtry2i::vector2i& direction) {
				long map_init_width = 1 << (map_header.info.depth + log2_w);
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
			* Returns the indexed item closest to the desired depth that contains the given point and is descended from start
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
			* Returns the pre-existing item closest to the desired depth that contains the given point and is descended from start
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
					unsigned long branches[4] = { 0, 0, 0, 0 };
					next_branch_idx = (p.x - item.origin.x >= hwidth) + 2 * (p.y - item.origin.y >= hwidth);
					branches[next_branch_idx] = map_header.size + branches_size;
					append_file(branches, branches_size);
					std::cout << "File appended with tree at " << (branches[next_branch_idx] - branches_size) << ":" << std::endl; //test
					std::cout << "{ " << branches[0] << ", " << branches[1] << ", " << branches[2] << ", " << branches[3] << " }" << std::endl; //test
					for (int i = 0; i < 4; i++) item.tree->branch[i] = new index_tree(branches[i]);
					item.tree = item.tree->branch[next_branch_idx];
					item.depth -= 1;
					item.origin += gmtry2i::vector2i(next_branch_idx & 1, next_branch_idx >> 1) * hwidth;
					hwidth >>= 1;
				}
				append_file(&blank, depth ? branches_size : sizeof(btile<log2_w>));
				return item;
			}
			void* build_item(const item_index& item) {
				if (item.tree == 0 || item.tree->pos == 0) return 0;
				if (item.depth == 0) {
					std::cout << "Tile built from " << item.tree->pos << ":" << std::endl; //test
					btile<log2_w>* tile = new btile<log2_w>();
					read_file(tile, item.tree->pos, sizeof(btile<log2_w>));
					PrintTile(*tile); //test
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
			class bmap_fstream_tile_stream : public btile_stream<log2_w> {
				protected:
					bmap_item_info info;
					void** items;
					gmtry2i::vector2i* origins;
					unsigned int* branch_indices;
					unsigned int current_level;
					gmtry2i::aligned_box2i bounds;

					bmap_fstream* src;
					btile<log2_w> last_tile;

					void update_next_item() {
						index_tree* current = static_cast<index_tree*>(items[current_level]);
						if (current->branch[0] == 0) {
							unsigned long branches[4] = { 0 };
							src->read_file(branches, current->pos, 4 * sizeof(unsigned long));
							for (int i = 0; i < 4; i++) current->branch[i] = new index_tree(branches[i]);
							std::cout << "Tree indexed from " << current->pos << ": " << std::endl; //test
							std::cout << "{ " << branches[0] << ", " << branches[1] << ", " << branches[2] << ", " << branches[3] << " }" << std::endl; //test
						}
						items[current_level + 1] = current->branch[branch_indices[current_level]];
						if (static_cast<index_tree*>(items[current_level + 1])->pos == 0) items[current_level + 1] = 0;
					}
					btile<log2_w>* get_tile(void* item) {
						src->read_file(&last_tile, static_cast<index_tree*>(item)->pos, sizeof(last_tile));
						std::cout << "Tile built from " << static_cast<index_tree*>(item)->pos << ":" << std::endl; //test
						PrintTile(last_tile); //test
						return &last_tile;
					}
				public:
					void reset() {
						origins[0] = info.origin;
						branch_indices[0] = 0;
						current_level = 0;
					}
					bmap_fstream_tile_stream(item_index item, bmap_fstream* source) {
						info.origin = item.origin;
						info.depth = item.depth;
						items = new void* [item.depth + 1];
						items[0] = item.tree;
						origins = new gmtry2i::vector2i[item.depth + 1];
						branch_indices = new unsigned int[item.depth];
						bounds = get_bmap_item_bounds<log2_w>(bmap_item_info(item.origin, item.depth));
						src = source;
						reset();
					}
					btile<log2_w>* next() {
						if (info.depth == 0) {
							if (branch_indices[0]) return 0;
							else {
								branch_indices[0]++;
								return get_tile(items[0]);
							}
						}
						unsigned int current_depth;
						while (branch_indices[0] < 4) {
							if (branch_indices[current_level] > 3) branch_indices[--current_level]++;
							else {
								current_depth = info.depth - current_level;
								origins[current_level + 1] = origins[current_level]
									+ gmtry2i::vector2i(branch_indices[current_level] & 1, branch_indices[current_level] >> 1)
									* (1 << (current_depth + log2_w - 1));
								update_next_item();
								if (items[current_level + 1] && gmtry2i::area(gmtry2i::intersection(bounds,
									gmtry2i::aligned_box2i(origins[current_level + 1], 1 << (current_depth + log2_w))))) {
									if (current_depth == 1) {
										btile<log2_w>* tile = get_tile(items[current_level + 1]);
										branch_indices[current_level]++;
										/*
										std::cout << "Tile streamed out at " << gmtry2i::to_string(origins[current_level + 1]) << std::endl; //test
										PrintTile(*tile); //test
										*/
										return tile;
									}
									else {
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
						return bounds;
					}
					void set_bounds(const gmtry2i::aligned_box2i& new_bounds) {
						bounds = gmtry2i::intersection(get_bmap_item_bounds<log2_w>(info), align_out<log2_w>(new_bounds, info.origin));
					}
					~bmap_fstream_tile_stream() {
						for (int i = 0; i < info.depth; i++) items[i] = 0;
						delete[] items;
						delete[] origins;
						delete[] branch_indices;
						std::cout << "Map tile stream deconstructed successfully!" << std::endl; //test
					}
			};

		public:
			bmap_fstream(const std::string& fname, const gmtry2i::vector2i& origin) {
				file_name = fname;
				file.open(file_name);
				if (!(file.is_open())) {
					FILE* tmp_file = 0;
					fopen_s(&tmp_file, file_name.c_str(), "w");
					fclose(tmp_file);
					file.open(file_name);
					map_header.info = bmap_item_info(origin, 1);
					map_header.log2_tile_w = log2_w;
					map_header.root = sizeof(bmap_file_header);
					map_header.size = sizeof(bmap_file_header);
					write_file(&map_header, 0, sizeof(bmap_file_header));
					unsigned long root_branches[4] = { 0, 0, 0, 0 };
					append_file(root_branches, 4 * sizeof(unsigned long));
					std::cout << "File created!" << std::endl; //test
				}
				else {
					read_file(&map_header, 0, sizeof(map_header));
					std::cout << "File already exists!" << std::endl; //test
				}
				if (!(file.is_open())) throw - 1;


				std::cout << "Recorded log2_tile_w: " << map_header.log2_tile_w << std::endl; //test
				std::cout << "Recorded depth: " << map_header.info.depth << std::endl; //test
				std::cout << "Recorded origin: " << map_header.info.origin.x << ", " << map_header.info.origin.y << std::endl; //test
				std::cout << "Recorded root: " << map_header.root << std::endl; //test
				std::cout << "Recorded file length: " << map_header.size << std::endl; //test


				if (log2_w != map_header.log2_tile_w) throw - 2;
				if (map_header.size < sizeof(bmap_file_header)) throw - 4;

				readmode = BTILE_OVERWRITE_MODE;
				writemode = BTILE_OVERWRITE_MODE;
				indices = new index_tree(map_header.root);
			}
			bool read(const gmtry2i::vector2i& p, btile<log2_w>* dst) {
				if (!(file.is_open())) return false;
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
			bool read(btile_stream<log2_w>** dst) {
				*dst = new bmap_fstream_tile_stream(get_top_item(), this);
				return true;
			}
			bool read(const gmtry2i::vector2i& origin, unsigned int depth, bmap<log2_w>* dst) {
				if (!(file.is_open())) return false;
				btile_stream<log2_w>* iterator;
				read(&iterator);
				iterator->set_bounds(gmtry2i::aligned_box2i(origin, 1 << (depth + log2_w)));
				set_bmap_tiles(dst, iterator, readmode);
				delete iterator;
				return true;
			}
			bool write(const gmtry2i::vector2i& p, const btile<log2_w>* src) {
				if (!(file.is_open())) return false;
				fit_map(p);
				item_index tile = alloc_item_at_depth(get_top_item(), p, 0);
				write_tile(src, tile.tree->pos);
				std::cout << "File appended with new tile!" << std::endl; //test
				return true;
			}
			bool write(btile_stream<log2_w>* src) {
				if (!(file.is_open())) return false;
				if (gmtry2i::area(src->get_bounds()) == 0) return true;
				fit_map(src->get_bounds());
				bmap_item_info virtual_min_dst = get_matching_virtual_bmap_item_info<log2_w>(map_header.info, src->get_bounds());
				item_index min_dst = alloc_item_at_depth(get_top_item(), virtual_min_dst.origin, virtual_min_dst.depth);
				btile<log2_w>* tile;
				item_index map_tile;
				while (tile = src->next()) if (exists(*tile)) {
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
	*	Deprecate the read(point, depth, map*) function
	*/
}