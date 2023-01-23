#pragma once

#include "occupancy.hpp"
#include "maps2/maps2_streams.hpp"

namespace ocpncy {
	template <unsigned int log2_w, typename T>
	class mat_tile_stream : public maps2::lim_tile_istream<otile<log2_w>> {
	private:
		T* mat;
		gmtry2i::vector2i origin;
		otile<log2_w> last_tile;
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
		const otile<log2_w>* next() {
			if (tile_origin.y >= bounds.max.y) return 0;
			otile<log2_w> tile = otile<log2_w>();
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

	// delete? THIS CLASS MIGHT BE DELETED
	template <unsigned int radius_minis>
	class occupancy_aggregator : public maps2::point2_ostream, public maps2::tile_istream<omini> {
	protected:
		omini minis[1 + 2 * radius_minis][1 + 2 * radius_minis] = {};
		gmtry2i::vector2i origin;
		// Geometric objects below are defined relative to origin
		gmtry2i::vector2i next_mini_origin, last_mini_origin;
		gmtry2i::aligned_box2i minis_bounds, read_bounds;
	public:
		void reset() {
			next_mini_origin = read_bounds.min;
		}
		occupancy_aggregator(const gmtry2i::vector2i& center, const gmtry2i::vector2i& any_mini_origin) {
			origin = maps2::align_down(center, any_mini_origin, 3) -
				(gmtry2i::vector2i(radius_minis, radius_minis) << 3);
			minis_bounds = { {}, (1 + 2 * radius_minis) << 3 };
			read_bounds = minis_bounds;
			reset();
		}
		inline void write(const gmtry2i::vector2i& p) {
			gmtry2i::vector2i local_p = p - origin;
			if (gmtry2i::contains(minis_bounds, local_p))
				minis[local_p.x >> LOG2_MINIW][local_p.y >> LOG2_MINIW] |=
				((omini)0b1) << ((local_p.x & MINI_COORD_MASK) + ((local_p.y & MINI_COORD_MASK) << LOG2_MINIW));
		}
		const omini* next() {
			while (next_mini_origin.y < read_bounds.max.y) {
				last_mini_origin = next_mini_origin;
				omini* next_mini_ptr = &(minis[last_mini_origin.y >> LOG2_MINIW][last_mini_origin.x >> LOG2_MINIW]);
				if ((next_mini_origin.x += MINI_WIDTH) > read_bounds.max.x) {
					next_mini_origin.x = read_bounds.min.x;
					next_mini_origin.y += MINI_WIDTH;
				}
				if (next_mini_ptr) return next_mini_ptr;
			}
			return 0;
		}
		gmtry2i::vector2i last_origin() {
			return last_mini_origin + origin;
		}
	};

	// 
	template <unsigned int log2_w>
	class occupancy_ostream {
	public:
		virtual void write(otile<log2_w>* tile_ptr, unsigned int occupancy_idx) = 0;
	};

	/*
	* Observes new nearby occupancies, compares them with recorded occupancies, 
	*	updates records, tracks which recorded tiles have been changed, 
	*	and feeds newly discovered occupancies to an occupancy_ostream.
	* Only cares about occupancies of observer's current tile and its neighbors.
	* Does not manage the map; cannot add new tiles or manually load in existing ones
	* NOT IMPLEMENTED YET
	*/
	template <unsigned int log2_w, unsigned int observation_radius>
	class occupancy_observer : public maps2::point2_ostream {
		static const int radius_minis = std::min((observation_radius >> LOG2_MINIW), get_tile_width_minis(log2_w));
		static const int aggregator_width = 1 + 2 * radius_minis;
		gmtry2i::vector2i position, tile_origin;
		maps2::nbrng_tile<req_otile<log2_w>>* current_tile;
		/*
		* When the observer needs a neighboring tile that hasn't been added to the map yet, it submits a request
		* through the tile_requestee. If the tile exists, it will be added to the map by the requestee. Otherwise,
		* and exception will be thrown.
		*/
		maps2::point2_ostream* tile_requestee;
		// For reporting changed-state occupancies
		occupancy_ostream<log2_w>* changes_listener;
		// Aggregates occupancies from one wave of observed points
		omini aggregator[aggregator_width][aggregator_width] = {};
		gmtry2i::aligned_box2i aggregator_bounds;
	public:
		// Clears aggregator
		void clear() {
			for (int i = 0; i < aggregator_width * aggregator_width; i++)
				(*aggregator)[i] = omini();
		}
		occupancy_observer(const gmtry2i::vector2i& init_position, maps2::nbrng_tile<req_otile<log2_w>>* init_tile,
		                   const gmtry2i::vector2i& any_tile_origin) {
			position = init_position;
			current_tile = init_tile;
			tile_origin = maps2::align_down(position, any_tile_origin, log2_w);
			tile_requestee = 0;
			changes_listener = 0;
			clear();
		}
		// Listener is alerted to changes made to occupancy states
		void set_listener(occupancy_ostream<log2_w>* new_listener) {
			changes_listener = new_listener;
		}
		// Requestee is expected to load requested tiles into the map
		void set_requestee(maps2::point2_ostream* new_requestee) {
			tile_requestee = new_requestee;
		}
		// Throws exception if path to the destination crosses tiles that don't exist
		void move(const gmtry2i::vector2i& new_position) {
			gmtry2i::vector2i new_tile_origin = maps2::align_down(new_position, tile_origin, log2_w);
			// in the loop, position is always tile-aligned
			while (tile_origin != new_tile_origin) {
				gmtry2i::vector2i disp = new_tile_origin - tile_origin;
				gmtry2i::vector2i nbr_nbrhood_coords = (disp >= 0) + (disp >= (1 << log2_w));
				gmtry2i::vector2i nbr_origin = tile_origin + ((nbr_nbrhood_coords - gmtry2i::vector2i(1, 1)) << log2_w);
				unsigned int nbr_compressed_coords = nbr_nbrhood_coords.x + 3 * nbr_nbrhood_coords.y;
				unsigned int nbr_idx = nbr_compressed_coords - (nbr_compressed_coords > 4);
				if (tile_requestee && !current_tile->nbrs[nbr_idx]) {
					tile_requestee->write(nbr_origin);
					tile_requestee->flush();
				}
				current_tile = current_tile->nbrs[nbr_idx];
				if (!current_tile) throw std::exception("Observer fell off the map!");
				tile_origin = nbr_origin;
			}
			position = new_position;
			aggregator_bounds = maps2::get_nbrhood_bounds(tile_origin, log2_w);
			clear();
		}
		/*
		* Relocates observer to new tile and position
		* Can be used to move observer to an entirely new map
		* Use this if observer falls off the map
		*/
		void relocate(const gmtry2i::vector2i& new_position, maps2::nbrng_tile<req_otile<log2_w>>* new_tile) {
			position = new_position;
			current_tile = new_tile;
		}
		void write(const gmtry2i::vector2i& p) {
			if (gmtry2i::contains(aggregator_bounds, p)) {
				gmtry2i::vector2i local_p = p - aggregator_bounds.min;
				aggregator[local_p.x >> LOG2_MINIW][local_p.y >> LOG2_MINIW] |=
					((omini)0b1) << ((local_p.x & MINI_COORD_MASK) + ((local_p.y & MINI_COORD_MASK) << LOG2_MINIW));
			}
		}
		void flush() {
			req_otile<log2_w>* nbrhood[3][3] = {
				&(current_tile->nbrs[0]->tile), &(current_tile->nbrs[1]->tile), &(current_tile->nbrs[2]->tile),
				&(current_tile->nbrs[3]->tile), &(current_tile->tile)         , &(current_tile->nbrs[4]->tile),
				&(current_tile->nbrs[5]->tile), &(current_tile->nbrs[6]->tile), &(current_tile->nbrs[7]->tile)
			};
			// do all the important stuff here
		}
	};
}