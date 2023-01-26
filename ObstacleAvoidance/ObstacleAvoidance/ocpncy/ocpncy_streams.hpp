#pragma once

#include "occupancy.hpp"
#include "maps2/maps2_streams.hpp"
#include "util/data_structs.hpp"

namespace ocpncy {
	template <unsigned int log2_w, typename T>
	class mat_tile_stream : public maps2::lim_tile_istream<otile<log2_w>> {
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
	* Does not manage the map; cannot add new tiles or load in existing ones to the map
	* NOT IMPLEMENTED YET
	*/
	template <unsigned int log2_w, unsigned int observation_radius>
	class occupancy_observer : public maps2::point2_ostream {
		static const unsigned int radius_minis = std::min((observation_radius >> LOG2_MINIW), 
		                                                  get_tile_width_minis(log2_w));
		static const unsigned int aggregator_width_minis = 1 + 2 * radius_minis;

		/*
		* For each occupied state, gradient_tile stores a certainty that is decremented when the state is expected to
		* be observed but is not, and reset to some positive value when observed. A certainty of 0 signifies that the 
		* state is known not to be occupied; the state is otherwise considered occupied.
		*/
		struct gradient_tile {
			static const unsigned char MAX_CERTAINTY = 32;

			unsigned char certainties[1 << (log2_w * 2)];
			gradient_tile() = default;
			gradient_tile(const otile<log2_w>& t) {
				for (int y = 0; y < (1 << log2_w); y++)
					for (int x = 0; x < (1 << log2_w); x++)
						if (get_bit(x, y, t)) certainties[x + (y << log2_w)] = MAX_CERTAINTY;
			}
			operator otile<log2_w>() const {
				otile<log2_w> t;
				for (int y = 0; y < (1 << log2_w); y++)
					for (int x = 0; x < (1 << log2_w); x++)
						set_bit(x, y, t, certainties[x + (y << log2_w)]);
				return t;
			}
		};

		gmtry2i::vector2i position, tile_origin;
		maps2::nbrng_tile<req_otile<log2_w>>* current_tile;
		gradient_tile* gradients[3][3];
		/*
		* When the observer needs a neighboring tile that hasn't been added to the map yet, it submits a request
		* through the tile_requestee. If the tile exists, it will be added to the map by the requestee. Otherwise,
		* and exception will be thrown.
		*/
		maps2::point2_ostream* tile_requestee;
		// For reporting changed-state occupancies
		occupancy_ostream<log2_w>* changes_listener;
		// Aggregates occupancies from one wave of observed points (aggregator is completely contained in neighborhood)
		omini aggregator[aggregator_width_minis][aggregator_width_minis] = {};
		gmtry2i::aligned_box2i aggregator_bounds;
	public:
		// Clears aggregator
		void clear() {
			for (int i = 0; i < aggregator_width_minis * aggregator_width_minis; i++)
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
				// FIX THIS!!! Observer needs a backup mechanism for when uncharted tiles are entered
				// Observer must be able to create its own tiles and delete them later
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
		* 
		* Will be removed when observer has backup mechanism to handle this case on its own
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
			maps2::tile_nbrhood<log2_w, req_otile<log2_w>> nbrhood(tile_origin, current_tile);
			// Defined relative to neighborhood origin
			gmtry2i::vector2i nbrhd_position = position - nbrhood.origin;
			// Translation from aggregator to neighborhood
			gmtry2i::vector2i gator_nbrhd_shift = nbrhood.origin - aggregator_bounds.min;
			// Defined relative to neighborhood origin
			gmtry2i::aligned_box2i nbrhd_boxes[3][3];
			for (int nbrhd_x = 0; nbrhd_x < 3; nbrhd_x++) for (int nbrhd_y = 0; nbrhd_y < 3; nbrhd_y++) {
				nbrhd_boxes[nbrhd_y][nbrhd_x] = { {nbrhd_x << log2_w, nbrhd_y << log2_w}, 1 << log2_w };
			}
			// Tracks whether each member of the neighborhood has been modified
			bool nbrs_modified[3][3] = {};
			strcts::linked_arraylist<gmtry2i::vector2i> observed_points = 
				strcts::linked_arraylist<gmtry2i::vector2i>();
			
			// Step 1: Lower certainties of missed occupancies
					/*
					* Trace the ray from each occupancy in observed_mini to the observer
					*	Optionally, skip minis that don't contain temporary occupancies
					*	Decrement certainties on all occupancies intersected along the way
					*	If a mini traced through/into had temporary occupancies, record it in some list or smth idc
					*/
			for (int ay = 0; ay < aggregator_width_minis; ay++) for (int ax = 0; ax < aggregator_width_minis; ax++) {
				omini observed_mini = aggregator[ay][ax];
				if (observed_mini) {
					// Origin of mini relative to neighborhood
					gmtry2i::vector2i mini_origin = (gmtry2i::vector2i(ax, ay) << LOG2_MINIW) + gator_nbrhd_shift;
					// Per occupancy in the observed mini
					for (int bit_idx = 0; bit_idx < MINI_AREA; bit_idx++) if ((observed_mini >> bit_idx) & 1) {
						gmtry2i::vector2i nbrhd_oc_pos(mini_origin + get_bit_offset(bit_idx));
						observed_points.add(nbrhd_oc_pos);
						gmtry2i::line_segment2i nbrhd_oc_line(nbrhd_position, nbrhd_oc_pos), tile_oc_line;
						for (int nbrhd_x = 0; nbrhd_x < 3; nbrhd_x++) for (int nbrhd_y = 0; nbrhd_y < 3; nbrhd_y++) {
							if (gmtry2i::intersects(nbrhd_oc_line, nbrhd_boxes[nbrhd_y][nbrhd_x])) {
								tile_oc_line = gmtry2i::intersection(nbrhd_oc_line, nbrhd_boxes[nbrhd_y][nbrhd_x]);
								gmtry2i::vector2i oc_disp(tile_oc_line.b - tile_oc_line.a);
								float length = std::sqrt(gmtry2i::squared(oc_disp));
								int length_int = length;
								if (length_int < length) length_int++;
								float inv_length = 1.0F / length;
								float norm_x = oc_disp.x * inv_length;
								float norm_y = oc_disp.y * inv_length;
								float x = tile_oc_line.a.x + 0.5, y = tile_oc_line.a.y + 0.5;
								gradient_tile* intersected_tile = gradients[nbrhd_y][nbrhd_x];
								for (int t = 0; t <= length_int; t++) {
									x += norm_x; y += norm_y;
									unsigned char& intrsctd_char = intersected_tile->
										certainties[static_cast<int>(x) + (static_cast<int>(y) << log2_w)];
									if (intrsctd_char) intrsctd_char--;
								}
							}
							nbrs_modified[nbrhd_y][nbrhd_x] = true;
						}
					}
				}
			}
			const int mask = get_tile_coord_mask(log2_w);

			// Step 2: Raise certainties of spotted occupancies
			int num_observed_points = observed_points.get_length();
			for (int i = 0; i < num_observed_points; i++) {
				gmtry2i::vector2i point = observed_points.next();
				gradients[point.y >> log2_w][point.x >> log2_w]->
					certainties[(point.x & mask) + ((point.y & mask) << log2_w)] = gradient_tile::MAX_CERTAINTY;
			}

			// Step 3: Compile tiles or minis from the gradient_tile certainties and compare them with the map 
			//         occupancies. Identify and report changed occupancy states, then copy them to the map.
			for (int nbrhd_x = 0; nbrhd_x < 3; nbrhd_x++) for (int nbrhd_y = 0; nbrhd_y < 3; nbrhd_y++) {
				if (nbrs_modified[nbrhd_y][nbrhd_x]) {
					otile<log2_w> compiled_tile = *(gradients[nbrhd_y][nbrhd_x]);
					req_otile<log2_w>* existing_tile = nbrhood.tiles[nbrhd_y][nbrhd_x];
					otile<log2_w> diff_tile = (compiled_tile - existing_tile->req) ^ 
					                          (existing_tile->tmp - existing_tile->req);
					gmtry2i::vector2i tile_origin(nbrhood.origin + (gmtry2i::vector2i(nbrhd_x, nbrhd_y) << log2_w));
					for (int m_idx = 0; m_idx < get_tile_area_minis(log2_w); m_idx++) if (diff_tile.minis[m_idx]) {
						omini diff_mini = diff_tile.minis[m_idx];
						gmtry2i::vector2i mini_origin(tile_origin + get_mini_offset(m_idx, log2_w));
						for (int bit_idx = 0; bit_idx < MINI_AREA; bit_idx++) {
							if ((diff_mini >> bit_idx) & 1)
								changes_listener->write(&(existing_tile->tmp),
									compress_coords2(mini_origin + get_bit_offset(bit_idx), log2_w));
						}
					}
					existing_tile->tmp = compiled_tile + existing_tile->req;
				}
			}

			

			/* Code snippet for use later
			
					gmtry2i::vector2i relative_origin(mini_origin - nbrhood.origin);
					omini existing_mini = nbrhood.tiles[relative_origin.y >> log2_w][relative_origin.x >> log2_w]->
						tmp.minis[get_mini_idx<log2_w>(relative_origin.x, relative_origin.y)];
					omini new_mini = mini_minus(observed_mini, existing_mini);
					if (new_mini) {

					}
			
			*/
		}
		~occupancy_observer() {

		}
	};
}