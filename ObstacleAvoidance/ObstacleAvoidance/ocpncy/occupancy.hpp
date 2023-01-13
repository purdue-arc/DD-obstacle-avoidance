#pragma once

#include "geometry.hpp"
#include "maps2/tilemaps.hpp"
#include "maps2/maps2_display.hpp"
#define DEFAULT_MAX_LINE_LENGTH (1536)

#include <stdint.h>
#include <iostream>

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
		inline btile& operator -=(const btile& tile) {
			for (int x = 0; x < 1 << (log2_w - 3); x++)
				for (int y = 0; y < 1 << (log2_w - 3); y++)
					minis[x][y] &= ~tile.minis[x][y];
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

	// t1 is purely observed
	// t2 is the known tile
	// t1 - t2 = unexpectedly observed occupancies
	template <unsigned int log2_w>
	btile<log2_w> operator -(const btile<log2_w>& t1, const btile<log2_w>& t2) {
		btile<log2_w> dif;
		for (int x = 0; x < 1 << (log2_w - 3); x++)
			for (int y = 0; y < 1 << (log2_w - 3); y++)
				dif.minis[x][y] = t1.minis[x][y] & (~(t2.minis[x][y]));
		return dif;
	}

	/*
	* Functions for getting or setting an individual bit in a tile
	*/
	template <unsigned int log2_w>
	inline bool get_bit(int x, int y, const btile<log2_w>& ot) {
		return ((ot.minis[y >> 3][x >> 3]) >> ((x & 0b111) + ((y & 0b111) << 3))) & 0b1;
	}
	template <unsigned int log2_w>
	inline void set_bit(int x, int y, btile<log2_w>& ot, bool value) {
		ot.minis[y >> 3][x >> 3] |= value * (((btile_mini) 0b1) << ((x & 0b111) + ((y & 0b111) << 3)));
	}

	template <unsigned int log2_w>
	inline unsigned int compress_coords2(const gmtry2i::vector2i& p) {
		return p.x + (p.y << log2_w);
	}

	template <unsigned int log2_w>
	inline gmtry2i::vector2i decompress_coords2(unsigned int idx) {
		unsigned int mask = (1 << log2_w) - 1;
		return gmtry2i::vector2i(idx & mask, (idx >> log2_w) & mask);
	}

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

	template <unsigned int log2_w>
	using btile_item = maps2::spacial_item<log2_w, btile<log2_w>, void>;

	template <unsigned int log2_w>
	inline maps2::ascii_image make_fitted_image(maps2::tile_istream<btile<log2_w>>* stream, unsigned int max_line_length) {
		return maps2::ascii_image(log2_w, stream->get_bounds().min, stream->get_bounds(), max_line_length);
	}

	template <unsigned int log2_w>
	maps2::ascii_image& operator << (maps2::ascii_image& img, const maps2::located_tile<btile<log2_w>>& tile) {
		for (int y = 0; y < (1 << log2_w); y++)
			for (int x = 0; x < (1 << log2_w); x++)
				if (get_bit(x, y, *tile.tile)) img(gmtry2i::vector2i(x, y) + tile.origin) = '@';
		return img;
	}
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