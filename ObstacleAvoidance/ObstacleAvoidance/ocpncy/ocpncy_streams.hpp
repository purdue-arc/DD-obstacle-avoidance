#pragma once

#include "occupancy.hpp"
#include "maps2/maps2_streams.hpp"

namespace ocpncy {
	template <unsigned int log2_w, typename T>
	class mat_tile_stream : public maps2::lim_tile_istream<btile<log2_w>> {
	private:
		T* mat;
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
			origin = mat_origin;
			mat_bounds = gmtry2i::aligned_box2i(gmtry2i::vector2i(), gmtry2i::vector2i(width, height));
			bounds = mat_bounds;
			tilewise_origin = maps2::align_down(mat_origin, any_tile_origin, log2_w) - mat_origin;
			// = align_down<log2_w>(bounds.min, any_tile_origin - mat_origin);
			reset();
		}
		const btile<log2_w>* next() {
			if (tile_origin.y >= bounds.max.y) return 0;
			btile<log2_w> tile = btile<log2_w>();
			gmtry2i::aligned_box2i readbox = gmtry2i::intersection(bounds, gmtry2i::aligned_box2i(tile_origin, 1 << log2_w));
			for (int x = readbox.min.x; x < readbox.max.x; x++)
				for (int y = readbox.min.y; y < readbox.max.y; y++)
					if (mat[x + y * mat_bounds.max.x])
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
		gmtry2i::aligned_box2i get_bounds() const {
			return maps2::align_out(bounds, tilewise_origin, log2_w) + origin;
		}
		void set_bounds(const gmtry2i::aligned_box2i& new_bounds) {
			gmtry2i::aligned_box2i local_new_bounds = new_bounds - origin;
			if (gmtry2i::intersects(mat_bounds, local_new_bounds)) 
				bounds = gmtry2i::intersection(mat_bounds, local_new_bounds);
			else bounds = { mat_bounds.min, 0 };
			tilewise_origin = maps2::align_down(bounds.min, tilewise_origin, log2_w);
			reset();
		}
		~mat_tile_stream() { }
	};

	template <typename T>
	T* halve_matrix(const T* mat, unsigned int width, unsigned int height) {
		unsigned int new_width = width >> 1;
		unsigned int new_height = height >> 1;
		T* half_mat = new T[new_width * new_height];
		for (int i = 0; i < new_width * new_height; i++) half_mat[i] = 0;
		for (unsigned int y = 0, hy = 0; y < height && hy < new_height; y++, hy = y >> 1)
			for (unsigned int x = 0, hx = 0; x < width && hx < new_width; x++, hx = x >> 1)
				half_mat[hx + new_width * hy] |= mat[x + width * y];
		return half_mat;
	}

	template <unsigned int radius_minis>
	class bmini_aggregator : public maps2::point_ostream, public maps2::tile_istream<btile_mini> {
	protected:
		btile_mini minis[1 + 2 * radius_minis][1 + 2 * radius_minis] = {};
		gmtry2i::vector2i origin;
		// Geometric objects below are defined relative to origin
		gmtry2i::vector2i next_mini_origin, last_mini_origin;
		gmtry2i::aligned_box2i minis_bounds, read_bounds;
	public:
		void reset() {
			next_mini_origin = read_bounds.min;
		}
		bmini_aggregator(const gmtry2i::vector2i& center, const gmtry2i::vector2i& any_mini_origin) {
			origin = maps2::align_down(center, any_mini_origin, 3) -
				(gmtry2i::vector2i(radius_minis, radius_minis) << 3);
			minis_bounds = { {}, (1 + 2 * radius_minis) << 3 };
			read_bounds = minis_bounds;
			reset();
		}
		inline void write(const gmtry2i::vector2i& p) {
			gmtry2i::vector2i local_p = p - origin;
			if (gmtry2i::contains(minis_bounds, local_p))
				minis[local_p.x >> 3][local_p.y >> 3] |=
				((btile_mini)0b1) << ((local_p.x & 0b111) + ((local_p.y & 0b111) << 3));
		}
		const btile_mini* next() {
			if (next_mini_origin.y > read_bounds.max.y) return 0;
			else {
				last_mini_origin = next_mini_origin;
				btile_mini* next_mini_ptr = &(minis[last_mini_origin.y >> 3][last_mini_origin.x >> 3]);
				if ((next_mini_origin.x += 8) > read_bounds.max.x) {
					next_mini_origin.x = read_bounds.min.x;
					next_mini_origin.y += 8;
				}
				return next_mini_ptr;
			}
		}
		gmtry2i::vector2i last_origin() {
			return last_mini_origin + origin;
		}
		void set_limiter(const gmtry2i::aligned_box2i& new_bounds) {
			gmtry2i::aligned_box2i local_new_bounds = new_bounds - origin;
			if (intersects(minis_bounds, local_new_bounds))
				read_bounds = gmtry2i::intersection(minis_bounds,
					maps2::align_out(local_new_bounds, minis_bounds.min, 3));
			else read_bounds = { minis_bounds.min, 0 };
		}
	};

	template <unsigned int log2_w>
	class btile_monitored_buffer : maps2::map_buffer<log2_w, maps2::nbrng_tile<btile<log2_w>>> {

	};
}