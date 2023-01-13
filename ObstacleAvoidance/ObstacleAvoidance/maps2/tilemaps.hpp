#pragma once

#include <iostream>

#include "geometry.hpp"

#ifdef DEBUG
#	define DEBUG_PRINT(s) std::cout << s << std::endl;
#else 
#	define DEBUG_PRINT(s)
#endif

namespace maps2 {
	enum tile_write_mode {
		TILE_OVERWRITE_MODE = 0,
		TILE_ADD_MODE = 1
	};

	inline gmtry2i::vector2i align_down(const gmtry2i::vector2i& p, const gmtry2i::vector2i any_tile_origin, unsigned int log2_w) {
		return (((p - any_tile_origin) >> log2_w) << log2_w) + any_tile_origin;
	}

	inline gmtry2i::vector2i align_up(const gmtry2i::vector2i& p, const gmtry2i::vector2i any_tile_origin, unsigned int log2_w) {
		gmtry2i::vector2i dif = p - any_tile_origin;
		gmtry2i::vector2i shifted_dif = dif >> log2_w;
		return ((shifted_dif + (dif - (shifted_dif << log2_w) > 0)) << log2_w) + any_tile_origin;
	}

	inline gmtry2i::aligned_box2i align_out(const gmtry2i::aligned_box2i& b, const gmtry2i::vector2i any_tile_origin, unsigned int log2_w) {
		return gmtry2i::aligned_box2i(align_down(b.min, any_tile_origin, log2_w), align_up(b.max, any_tile_origin, log2_w));
	}

	template <typename base_tile>
	struct nbrng_tile {
		base_tile tile;
		nbrng_tile<base_tile>* nbrs[8];
	};

	class point_ostream {
	public:
		virtual void write(const gmtry2i::vector2i& p);
	};

	class point3_ostream {
	public:
		virtual void write(const gmtry3::vector3& p);
	};

	class bounded_region {
	public:
		// Returns a tile-aligned rectangular outer bounds on all encompassed area
		virtual gmtry2i::aligned_box2i get_bounds() const = 0;
	};

	template <typename tile>
	class tile_istream : public bounded_region {
	public:
		// Resets the stream's reading position
		virtual void reset() = 0;
		// Returns the next tile in the stream, or 0 if all tiles have been retrieved
		virtual const tile* next() = 0;
		// Returns the origin of the last tile retrieved from next()
		virtual gmtry2i::vector2i last_origin() = 0;
		// Sets a bounds on the outgoing tiles. New bounds will automatically be tile-aligned if necessary
		virtual void set_bounds(const gmtry2i::aligned_box2i& new_bounds) = 0;
		virtual ~tile_istream() {};
	};

	template <typename tile>
	class tile_ostream {
	public:
		// Writes a tile at the given position to the stream
		virtual void write(const gmtry2i::vector2i& p, const tile* src) = 0;
		// Writes each tile from the tile_istream to the stream, assuming only tile-level allignment between streams
		virtual void write(tile_istream<tile>* src) {
			const tile* next_tile;
			while (next_tile = src->next())
				write(src->last_origin(), next_tile);
		}
		// Saves any unsaved changes
		virtual void flush() {}
		virtual ~tile_ostream() {};
	};

	template <unsigned int log2_w>
	class stretchable_region : bounded_region {
	public:
		/*
		* Stretches the region in the given direction
		*/
		virtual void stretch(const gmtry2i::vector2i& direction) = 0;

		/*
		* Fits the point into the region by iteratively stretching it as necessary
		*/
		void fit(const gmtry2i::vector2i& p) {
			gmtry2i::aligned_box2i alloc_box = this->get_bounds();
			while (!gmtry2i::contains(alloc_box, p)) {
				stretch(p - gmtry2i::center(alloc_box));
				alloc_box = this->get_bounds();
				DEBUG_PRINT("Expanded Map Bounds: " <<
					alloc_box.min.x << ", " << alloc_box.min.y << "; " <<
					alloc_box.max.x << ", " << alloc_box.max.y); //test
			}
		}

		/*
		* Fits the box into the region by iteratively stretching it as necessary
		*/
		void fit(const gmtry2i::aligned_box2i& box) {
			gmtry2i::aligned_box2i alloc_box = this->get_bounds();
			gmtry2i::vector2i box_center = gmtry2i::center(box);
			while (!gmtry2i::contains(alloc_box, box)) {
				stretch(box_center - gmtry2i::center(alloc_box));
				alloc_box = this->get_bounds();
				DEBUG_PRINT("Expanded Map Bounds: " <<
					alloc_box.min.x << ", " << alloc_box.min.y << "; " <<
					alloc_box.max.x << ", " << alloc_box.max.y); //test
			}
		}
	};

	template <typename tile>
	class map_istream : public bounded_region {
	public:
		/*
		* Reads tile at given position to the destination
		* Returns 0 if no tile contains the position
		*/
		virtual const tile* read(const gmtry2i::vector2i& p) = 0;

		/*
		* Writes a tile stream to the destination, which can be used to stream out all tiles from a desired rectangular region of the map
		* TILE STREAM MUST BE DELETED MANUALLY
		*/
		virtual tile_istream<tile>* read() = 0;
	};

	template <typename tile>
	class map_ostream : public tile_ostream<tile> {
	public:
		/*
		* Accessors for writing mode (determines how a written tile will be combined with an existing one)
		*/
		virtual tile_write_mode get_wmode() = 0;
		virtual void set_wmode(tile_write_mode new_write_mode) = 0;
	};

	template <typename tile>
	class map_iostream : public map_istream<tile>, public map_ostream<tile> {};

	template <typename T>
	struct spacial_tree {
		T* branch[4];
	};

	/*
	* Holds information about a spacial tree
	* depth: depth of the tree (# of layers in the tree above the tile layer)
	* origin: origin (bottom left corner position) of tree in world-space
	*/
	struct tree_info {
		gmtry2i::vector2i origin;
		unsigned int depth;
		tree_info() = default;
		tree_info(const gmtry2i::vector2i map_origin, unsigned int map_depth) {
			origin = map_origin;
			depth = map_depth;
		}
	};

	inline gmtry2i::vector2i get_next_branch_disp(unsigned int branch_idx, unsigned long hwidth) {
		return gmtry2i::vector2i(branch_idx & 1, branch_idx >> 1) * hwidth;
	}

	/*
	* Temporarily holds information about a part of the map (either a tree or a tile)
	* When deleted, the data pointed to by ptr isn't deleted
	* Ptr is always non-null, and the item it points to can have any depth
	* T must either be void or some kind of homogeneous_tree
	*/
	template <unsigned int log2_tile_w, typename tile, typename T>
	struct spacial_item {
		tree_info info;
		T* ptr;
		spacial_item() = default;
		spacial_item(T* item_ptr, gmtry2i::vector2i item_origin, unsigned int item_depth) {
			info = tree_info(item_origin, item_depth);
			ptr = item_ptr;
		}
		spacial_item(T* item_ptr, const tree_info& item_info) {
			info = item_info;
			ptr = item_ptr;
		}
		inline unsigned int get_width() const {
			return 1 << (info.depth + log2_tile_w);
		}
		inline unsigned int get_branch_width() const {
			return get_width() >> 1;
		}
		inline tree_info get_branch_info(unsigned int branch_idx, unsigned int branch_width) const {
			return tree_info(info.origin + get_next_branch_disp(branch_idx, branch_width), info.depth - 1);
		}
		inline unsigned int get_branch_idx(const gmtry2i::vector2i& branch_origin, unsigned int branch_width) const {
			return (branch_origin.x - info.origin.x >= branch_width) + 2 * (branch_origin.y - info.origin.y >= branch_width);
		}
		inline unsigned int get_branch_idx(const gmtry2i::vector2i& branch_origin) const {
			return get_branch_idx(branch_origin, get_branch_width());
		}
		inline spacial_item get_branch_item(unsigned int branch_idx, unsigned int branch_width) const {
			return spacial_item(
				static_cast<spacial_tree<T>*>(ptr)->branch[branch_idx],
				get_branch_info(branch_idx, branch_width)
			);
		}
		inline spacial_item get_branch_item(unsigned int branch_idx) const {
			return get_branch_item(branch_idx, get_branch_width());
		}
		inline spacial_item get_branch_item(const gmtry2i::vector2i& branch_origin) const {
			unsigned int branch_width = get_branch_width();
			return get_branch_item(get_branch_idx(branch_origin, branch_width), branch_width);
		}
		inline spacial_item create_branch_item(const gmtry2i::vector2i& branch_origin, T* branch_ptr) const {
			unsigned int branch_width = get_branch_width();
			unsigned int branch_idx = get_branch_idx(branch_origin, branch_width);
			return spacial_item(
				static_cast<spacial_tree<T>*>(ptr)->branch[branch_idx] = branch_ptr,
				get_branch_info(branch_idx, branch_width)
			);
		}
	};

	template <unsigned int log2_tile_w>
	inline gmtry2i::aligned_box2i get_bounds(const tree_info& info) {
		return gmtry2i::aligned_box2i(info.origin, 1 << (info.depth + log2_tile_w));
	}

	/*
	* Returns the info of the sub-item from the given item that most tightly fits the given bounds inside
	* Returns an item info of depth 1 and origin (0, 0) if the map does not contain the boundary box
	*/
	template <unsigned int log2_w>
	tree_info get_matching_item_info(const tree_info& item_info, const gmtry2i::aligned_box2i& bounds) {
		tree_info matching_item = tree_info(gmtry2i::vector2i(), 1);
		tree_info next_matching_item(item_info.origin, item_info.depth);
		unsigned int next_branch_index;
		unsigned int hwidth = 1 << (next_matching_item.depth + log2_w - 1);
		while (gmtry2i::contains(get_bounds<log2_w>(next_matching_item), bounds) && matching_item.depth) {
			matching_item = next_matching_item;
			next_branch_index = (bounds.min.x - matching_item.origin.x >= hwidth) + 2 * (bounds.min.y - matching_item.origin.y >= hwidth);
			next_matching_item.depth = matching_item.depth - 1;
			next_matching_item.origin = matching_item.origin + gmtry2i::vector2i(next_branch_index & 1, next_branch_index >> 1) * hwidth;
			hwidth >>= 1;
		}
		return matching_item;
	}
}