#pragma once

#include "geometry.hpp"
#include "maps2/tilemaps.hpp"
#include "maps2/maps2_display.hpp"

#define DEFAULT_MAX_LINE_LENGTH (1536)

#include <stdint.h>
#include <iostream>

namespace ocpncy {
	const unsigned int LOG2_MINIW = 3;
	const unsigned int MINI_WIDTH = 1 << LOG2_MINIW;
	const unsigned int MINI_COORD_MASK = MINI_WIDTH - 1;

	// WxW square of occupancy states (W = width of a mini = 2 ^ LOG2_MINIW)
	typedef std::uint64_t omini;

	inline omini sum(const omini& m1, const omini& m2) {
		return m1 | m2;
	}
	inline omini minus(const omini& m1, const omini& m2) {
		return m1 & ~m2;
	}
	inline omini diff(const omini& m1, const omini& m2) {
		return m1 ^ m2;
	}

	// Returns the width of a tile in units of minis
	constexpr unsigned int get_tile_width_minis(unsigned int log2_tile_w) {
		return 1 << (log2_tile_w - LOG2_MINIW);
	}
	// Returns the area of a tile in units of minis
	constexpr unsigned int get_tile_area_minis(unsigned int log2_tile_w) {
		return 1 << ((log2_tile_w - LOG2_MINIW) * 2);
	}

	/*
	* A square tile of occupancy states (represented as individual bits) with a width of 2 ^ n 
		(this is so the tile can be evenly cut in half all the way down to the bit level, 
		which makes certain opertions faster)
	* log2_w: base-2 logarithm of tile width; tile width = w = 2 ^ log2_w = 1 << log2_w
	* log2_w MUST BE GREATER THAN OR EQUAL TO LOG2_MINIW (= 3 at the time of writing this)!!!
	*/
	template <unsigned int log2_w>
	struct otile {
		omini minis[get_tile_area_minis(log2_w)];
		inline otile& operator +=(const otile& tile) {
			for (int i = 0; i < get_tile_area_minis(log2_w); i++)
				minis[i] |= tile.minis[i];
			return *this;
		}
		inline otile& operator -=(const otile& tile) {
			for (int i = 0; i < get_tile_area_minis(log2_w); i++)
				minis[i] &= ~tile.minis[i];
			return *this;
		}
	};

	// Tests whether a tile has any occupancies
	template <unsigned int log2_w>
	bool is_occupied(const otile<log2_w>& t) {
		bool occupied = false;
		for (int i = 0; i < get_tile_area_minis(log2_w); i++)
			occupied |= t.minis[i];
		return occupied;
	}

	// Returned tile contains all occupancies present in t1 or t2
	template <unsigned int log2_w>
	otile<log2_w> operator +(const otile<log2_w>& t1, const otile<log2_w>& t2) {
		otile<log2_w> sum;
		for (int i = 0; i < get_tile_area_minis(log2_w); i++)
			sum.minis[i] = t1.minis[i] | t2.minis[i];
		return sum;
	}

	// Returned tile contains only the occupancies which are present in either t1 or t2, but not both
	template <unsigned int log2_w>
	otile<log2_w> operator ^(const otile<log2_w>& t1, const otile<log2_w>& t2) {
		otile<log2_w> dif;
		for (int i = 0; i < get_tile_area_minis(log2_w); i++)
			dif.minis[i] = t1.minis[i] ^ t2.minis[i];
		return dif;
	}

	// Returned tile contains only the occupancies from t1 which weren't present in t2
	template <unsigned int log2_w>
	otile<log2_w> operator -(const otile<log2_w>& t1, const otile<log2_w>& t2) {
		otile<log2_w> dif;
		for (int i = 0; i < get_tile_area_minis(log2_w); i++)
			dif.minis[i] = t1.minis[i] & ~t2.minis[i];
		return dif;
	}

	// Accessors for individual bits in a tile
	template <unsigned int log2_w>
	inline bool get_bit(int x, int y, const otile<log2_w>& ot) {
		return ((ot.minis[(x >> LOG2_MINIW) + ((y >> LOG2_MINIW) << (log2_w - LOG2_MINIW))]) >>
			((x & MINI_COORD_MASK) + ((y & MINI_COORD_MASK) << LOG2_MINIW))) & 1;
	}
	template <unsigned int log2_w>
	inline void set_bit(int x, int y, otile<log2_w>& ot, bool value) {
		ot.minis[(x >> LOG2_MINIW) + ((y >> LOG2_MINIW) << (log2_w - LOG2_MINIW))] |=
			value * (((omini) 1) << ((x & MINI_COORD_MASK) + ((y & MINI_COORD_MASK) << LOG2_MINIW)));
	}

	// Converts between 2D tile-space coordinates and their compressed integer representation
	template <unsigned int log2_w>
	inline unsigned int compress_coords2(const gmtry2i::vector2i& p) {
		return p.x + (p.y << log2_w);
	}
	template <unsigned int log2_w>
	inline gmtry2i::vector2i decompress_coords2(unsigned int idx) {
		return gmtry2i::vector2i(idx & ((1 << log2_w) - 1), (idx >> log2_w) & ((1 << log2_w) - 1));
	}

	// Prints a tile
	template <unsigned int log2_w>
	std::ostream& operator << (std::ostream& os, const otile<log2_w>& tile) {
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
	maps2::ascii_image& operator << (maps2::ascii_image& img, const maps2::located_tile<otile<log2_w>>& tile) {
		for (int y = 0; y < (1 << log2_w); y++)
			for (int x = 0; x < (1 << log2_w); x++)
				if (get_bit(x, y, *tile.tile)) img(gmtry2i::vector2i(x, y) + tile.origin) = '@';
		return img;
	}

	// Makes an ASCII image fitted to the stream's output
	template <unsigned int log2_w>
	inline maps2::ascii_image make_fitted_image(maps2::tile_istream<otile<log2_w>>* stream, unsigned int max_line_length) {
		return maps2::ascii_image(log2_w, stream->get_bounds().min, stream->get_bounds(), max_line_length);
	}

	// Draws tile stream to ASCII image and then prints it
	template <unsigned int log2_w>
	std::ostream& operator << (std::ostream& os, maps2::tile_istream<otile<log2_w>>* stream) {
		maps2::ascii_image img(log2_w, stream->get_bounds().min, stream->get_bounds(), DEFAULT_MAX_LINE_LENGTH);
		return os << (img << stream);
	}
	template <unsigned int log2_w>
	std::ostream& operator << (std::ostream& os, maps2::map_istream<otile<log2_w>>* map) {
		maps2::ascii_image img(log2_w, map->get_bounds().min, map->get_bounds(), DEFAULT_MAX_LINE_LENGTH);
		return os << (img << map);
	}

	// Temporary/modifiable occupancy tile (tmp) that has a required minimum version (req), such that req - tmp = 0
	template <unsigned int log2_w>
	struct req_otile {
		otile<log2_w> tmp;
		otile<log2_w> req;
	};

	// 3D bit-tile formed by layers of 2D bit-tiles
	template <unsigned int log2_w, unsigned int num_layers>
	struct otile3 {
		otile<log2_w> layers[num_layers];
		otile3() = default;
		inline otile3& operator +=(const otile3& tile) {
			for (int z = 0; z < num_layers; z++)
				layers[z] += tile.layers[z];
			return *this;
		}
		inline otile3& operator +=(const otile3* tile) {
			for (int z = 0; z < num_layers; z++)
				layers[z] += tile->layers[z];
			return *this;
		}
		inline otile3& operator -=(const otile3& tile) {
			for (int z = 0; z < num_layers; z++)
				layers[z] -= tile.layers[z];
			return *this;
		}
		inline otile3& operator -=(const otile3* tile) {
			for (int z = 0; z < num_layers; z++)
				layers[z] -= tile->layers[z];
			return *this;
		}
		inline otile3 operator =(const otile<log2_w>& tile) {
			for (int z = 0; z < num_layers; z++)
				layers[z] = tile;
			return *this;
		}
	};

	template <unsigned int log2_w, unsigned int num_layers>
	inline bool get_bit(int x, int y, int z, const otile3<log2_w, num_layers>& ot) {
		return get_bit(x, y, ot.layers[z]);
	}

	template <unsigned int log2_w, unsigned int num_layers>
	inline void set_bit(int x, int y, int z, otile3<log2_w, num_layers>& ot, bool value) {
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