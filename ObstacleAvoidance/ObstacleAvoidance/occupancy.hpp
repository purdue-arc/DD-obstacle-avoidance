#pragma once

#include <string>
#include <fstream>
#include <stdio.h>
#include <stdint.h>
#include <iostream>

#include "geometry.hpp"
#include "tilemaps.hpp"

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
				dif.minis[x][y] = t2.minis[x][y] & (~(t1.minis[x][y]));
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
	void PrintImage(const bimage& img) {
		for (int y = 0; y < img.width; y++) std::cout << img.line(y) << std::endl;
		std::cout << img.caption << std::endl;
	}
	template <unsigned int log2_w>
	void WriteImageTile(bimage& img, const gmtry2i::vector2i& tile_origin, const btile<log2_w>& tile) {
		for (int y = 0; y < (1 << log2_w); y++)
			for (int x = 0; x < (1 << log2_w); x++)
				if (get_bit(x, y, tile)) img(gmtry2i::vector2i(x, y) + tile_origin) = '@';
	}
	template <unsigned int log2_w>
	void WriteImageTiles(bimage& img, tmaps2::tile_stream<btile<log2_w>>* tiles) {
		const btile<log2_w>* tile;
		while (tile = tiles->next()) 
			if (gmtry2i::contains(img.bounds, tiles->last_origin())) 
				WriteImageTile(img, tiles->last_origin(), *tile);
	}
	template <unsigned int log2_w>
	void PrintItem(const tmaps2::map_item<btile<log2_w>>& item) {
		bimage img(log2_w, item.info.depth, item.info.origin);
		tmaps2::map_tstream<log2_w, btile<log2_w>> iterator(item);
		WriteImageTiles(img, &iterator);
		PrintImage(img);
	}
	template <unsigned int log2_w>
	void PrintTiles(tmaps2::tile_stream<btile<log2_w>>* stream, unsigned int depth) {
		bimage img(log2_w, depth, stream->get_bounds().min);
		WriteImageTiles(img, stream);
		PrintImage(img);
	}

	template <unsigned int log2_w, typename T>
	class mat_tile_stream : public tmaps2::tile_stream<btile<log2_w>> {
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
				tilewise_origin = tmaps2::align_down<log2_w>(mat_origin, any_tile_origin) - mat_origin;
				// = align_down<log2_w>(bounds.min, any_tile_origin - mat_origin);
				reset();
			}
			const btile<log2_w>* next() {
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
				tilewise_origin = tmaps2::align_down<log2_w>(bounds.min, tilewise_origin);
			}
			~mat_tile_stream() { }
	};

	/*
	* TODO
	*	Make branch positions and map file header have a platform-independent size
	*/
}