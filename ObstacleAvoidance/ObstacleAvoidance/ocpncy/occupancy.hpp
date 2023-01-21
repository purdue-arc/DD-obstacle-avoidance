#pragma once

#include "geometry.hpp"
#include "maps2/tilemaps.hpp"
#include "maps2/maps2_display.hpp"

#define DEFAULT_MAX_LINE_LENGTH (1536)

#include <stdint.h>
#include <iostream>

namespace ocpncy {
#define LOG2_MINIW (3)
#define MINI_COORD_MASK (0b111)

	// WxW square of occupancy states (W = width of a mini = 2 ^ LOG2_MINIW)
	typedef std::uint64_t btile_mini;

	/*
	* A square tile of bits with a width of 2 ^ n 
		(this is so the tile can be evenly cut in half all the way down to the bit level, 
		which makes certain opertions faster)
	* log2_w: base-2 logarithm of tile width; tile width = w = 2 ^ log2_w = 1 << log2_w
	* log2_w MUST BE GREATER THAN OR EQUAL TO LOG2_MINIW (= 3 at the time of writing this)!!!
	*/
	template <unsigned int log2_w>
	struct btile {
		btile_mini minis[1 << (log2_w - LOG2_MINIW)][1 << (log2_w - LOG2_MINIW)];
		btile() = default;
		inline btile& operator +=(const btile& tile) {
			for (int x = 0; x < 1 << (log2_w - LOG2_MINIW); x++)
				for (int y = 0; y < 1 << (log2_w - LOG2_MINIW); y++)
					minis[x][y] |= tile.minis[x][y];
			return *this;
		}
		inline btile& operator -=(const btile& tile) {
			for (int x = 0; x < 1 << (log2_w - LOG2_MINIW); x++)
				for (int y = 0; y < 1 << (log2_w - LOG2_MINIW); y++)
					minis[x][y] &= ~tile.minis[x][y];
			return *this;
		}
	};

	// Tests whether a tile has any occupancies
	template <unsigned int log2_w>
	bool is_occupied(const btile<log2_w>& t) {
		bool occupied = false;
		for (int x = 0; x < 1 << (log2_w - LOG2_MINIW); x++)
			for (int y = 0; y < 1 << (log2_w - LOG2_MINIW); y++)
				occupied |= t.minis[x][y];
		return occupied;
	}

	// Returned tile contains all occupancies present in t1 or t2
	template <unsigned int log2_w>
	btile<log2_w> operator +(const btile<log2_w>& t1, const btile<log2_w>& t2) {
		btile<log2_w> sum;
		for (int x = 0; x < 1 << (log2_w - LOG2_MINIW); x++)
			for (int y = 0; y < 1 << (log2_w - LOG2_MINIW); y++)
				sum.minis[x][y] = t1.minis[x][y] | t2.minis[x][y];
		return sum;
	}

	// Returned tile contains only the occupancies which are present in either t1 or t2, but not both
	template <unsigned int log2_w>
	btile<log2_w> operator ^(const btile<log2_w>& t1, const btile<log2_w>& t2) {
		btile<log2_w> dif;
		for (int x = 0; x < 1 << (log2_w - LOG2_MINIW); x++)
			for (int y = 0; y < 1 << (log2_w - LOG2_MINIW); y++)
				dif.minis[x][y] = t1.minis[x][y] ^ t2.minis[x][y];
		return dif;
	}

	// Returned tile contains only the occupancies from t1 which weren't present in t2
	template <unsigned int log2_w>
	btile<log2_w> operator -(const btile<log2_w>& t1, const btile<log2_w>& t2) {
		btile<log2_w> dif;
		for (int x = 0; x < 1 << (log2_w - LOG2_MINIW); x++)
			for (int y = 0; y < 1 << (log2_w - LOG2_MINIW); y++)
				dif.minis[x][y] = t1.minis[x][y] & ~(t2.minis[x][y]);
		return dif;
	}

	// Accessors for individual bits in a tile
	template <unsigned int log2_w>
	inline bool get_bit(int x, int y, const btile<log2_w>& ot) {
		return ((ot.minis[y >> LOG2_MINIW][x >> LOG2_MINIW]) >> 
			((x & MINI_COORD_MASK) + ((y & MINI_COORD_MASK) << LOG2_MINIW))) & 0b1;
	}
	template <unsigned int log2_w>
	inline void set_bit(int x, int y, btile<log2_w>& ot, bool value) {
		ot.minis[y >> LOG2_MINIW][x >> LOG2_MINIW] |= 
			value * (((btile_mini) 0b1) << ((x & MINI_COORD_MASK) + ((y & MINI_COORD_MASK) << LOG2_MINIW)));
	}

	// Converts between 2D tile-space coordinates and their compressed integer representation
	template <unsigned int log2_w>
	inline unsigned int compress_coords2(const gmtry2i::vector2i& p) {
		return p.x + (p.y << log2_w);
	}
	template <unsigned int log2_w>
	inline gmtry2i::vector2i decompress_coords2(unsigned int idx) {
		const unsigned int mask = (1 << log2_w) - 1;
		return gmtry2i::vector2i(idx & mask, (idx >> log2_w) & mask);
	}

	// Prints a tile
	template <unsigned int log2_w>
	std::ostream& operator << (std::ostream& os, const btile<log2_w>& tile) {
		for (int y = (1 << log2_w) - 1; y >= 0; y--) {
			for (int x = 0; x < (1 << log2_w); x++) {
				os << (get_bit(x, y, tile) ? "@" : ".") << ' ';
			}
			os << std::endl;
		}
		return os;
	}

	// Draws a tile at a position on an ASCII image
	template <unsigned int log2_w>
	maps2::ascii_image& operator << (maps2::ascii_image& img, const maps2::located_tile<btile<log2_w>>& tile) {
		for (int y = 0; y < (1 << log2_w); y++)
			for (int x = 0; x < (1 << log2_w); x++)
				if (get_bit(x, y, *tile.tile)) img(gmtry2i::vector2i(x, y) + tile.origin) = '@';
		return img;
	}

	// Makes an ASCII image fitted to the stream's output
	template <unsigned int log2_w>
	inline maps2::ascii_image make_fitted_image(maps2::tile_istream<btile<log2_w>>* stream, unsigned int max_line_length) {
		return maps2::ascii_image(log2_w, stream->get_bounds().min, stream->get_bounds(), max_line_length);
	}

	// Draws tile stream to ASCII image and then prints it
	template <unsigned int log2_w>
	std::ostream& operator << (std::ostream& os, maps2::tile_istream<btile<log2_w>>* stream) {
		maps2::ascii_image img(log2_w, stream->get_bounds().min, stream->get_bounds(), DEFAULT_MAX_LINE_LENGTH);
		return os << (img << stream);
	}
	template <unsigned int log2_w>
	std::ostream& operator << (std::ostream& os, maps2::map_istream<btile<log2_w>>* map) {
		maps2::ascii_image img(log2_w, map->get_bounds().min, map->get_bounds(), DEFAULT_MAX_LINE_LENGTH);
		return os << (img << map);
	}

	// 3D bit-tile formed by layers of 2D bit-tiles
	template <unsigned int log2_w, unsigned int num_layers>
	struct btile3 {
		btile<log2_w> layers[num_layers];
		btile3() = default;
		inline btile3& operator +=(const btile3& tile) {
			for (int z = 0; z < num_layers; z++)
				layers[z] += tile.layers[z];
			return *this;
		}
		inline btile3& operator +=(const btile3* tile) {
			for (int z = 0; z < num_layers; z++)
				layers[z] += tile->layers[z];
			return *this;
		}
		inline btile3& operator -=(const btile3& tile) {
			for (int z = 0; z < num_layers; z++)
				layers[z] -= tile.layers[z];
			return *this;
		}
		inline btile3& operator -=(const btile3* tile) {
			for (int z = 0; z < num_layers; z++)
				layers[z] -= tile->layers[z];
			return *this;
		}
		inline btile3 operator =(const btile<log2_w>& tile) {
			for (int z = 0; z < num_layers; z++)
				layers[z] = tile;
			return *this;
		}
	};

	template <unsigned int log2_w, unsigned int num_layers>
	inline bool get_bit(int x, int y, int z, const btile3<log2_w, num_layers>& ot) {
		return get_bit(x, y, ot.layers[z]);
	}

	template <unsigned int log2_w, unsigned int num_layers>
	inline void set_bit(int x, int y, int z, btile3<log2_w, num_layers>& ot, bool value) {
		set_bit(x, y, ot.layers[z], value);
	}

	template <unsigned int log2_w>
	inline unsigned int compress_coords3(const gmtry3::vector3& p) {
		return static_cast<unsigned int>(p.x) + 
			 ((static_cast<unsigned int>(p.y) +
			  (static_cast<unsigned int>(p.z) << log2_w)) << log2_w);
	}

	template <unsigned int log2_w>
	inline gmtry3::vector3 decompress_coords3(unsigned int idx) {
		unsigned int mask = (1 << log2_w) - 1;
		return gmtry3::vector3(idx & mask, (idx >> log2_w) & mask, idx >> (2 * log2_w));
	}
}