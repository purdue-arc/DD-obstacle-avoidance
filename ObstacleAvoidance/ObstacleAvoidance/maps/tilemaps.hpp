#pragma once

#include <fstream>
#include <stdio.h>
#include <iostream>
#include <ios>
#include <exception>
#include <stdint.h>

#include "geometry.hpp"

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

	class bounded_region {
	public:
		// Returns a tile-aligned rectangular outer bounds on all encompassed area
		virtual gmtry2i::aligned_box2i get_bounds() = 0;
	};

	template <typename tile>
	class tile_stream : public bounded_region {
	public:
		// Returns the stream to its initial conditions
		virtual void reset() = 0;
		// Returns the next tile in the stream, or 0 if all tiles have been retrieved
		virtual const tile* next() = 0;
		// Returns the origin of the last tile retrieved from next()
		virtual gmtry2i::vector2i last_origin() = 0;
		// Sets a bounds on the outgoing tiles. New bounds will automatically be tile-aligned if necessary
		virtual void set_bounds(const gmtry2i::aligned_box2i& new_bounds) = 0;
		virtual ~tile_stream() {};
	};

	template <unsigned int log2_w>
	class expandable_map : bounded_region {
	public:
		/*
		* Expands the map in the given direction
		*/
		virtual void expand(const gmtry2i::vector2i& direction) = 0;

		/*
		* Fits the position into the map by expanding it if necessary
		*/
		void fit(const gmtry2i::vector2i& p) {
			gmtry2i::aligned_box2i alloc_box = this->get_bounds();
			while (!gmtry2i::contains(alloc_box, p)) {
				expand(p - gmtry2i::center(alloc_box));
				alloc_box = this->get_bounds();
				std::cout << "Expanded Map Bounds: " <<
					alloc_box.min.x << ", " << alloc_box.min.y << "; " <<
					alloc_box.max.x << ", " << alloc_box.max.y << std::endl; //test
			}
		}

		/*
		* Fits the box into the map by expanding it if necessary
		*/
		void fit(const gmtry2i::aligned_box2i& box) {
			gmtry2i::aligned_box2i alloc_box = this->get_bounds();
			gmtry2i::vector2i box_center = gmtry2i::center(box);
			while (!gmtry2i::contains(alloc_box, box)) {
				expand(box_center - gmtry2i::center(alloc_box));
				alloc_box = this->get_bounds();
				std::cout << "Expanded Map Bounds: " <<
					alloc_box.min.x << ", " << alloc_box.min.y << "; " <<
					alloc_box.max.x << ", " << alloc_box.max.y << std::endl; //test
			}
		}
	};

	template <typename tile>
	class map_istream : public bounded_region {
	public:
		/*
		* Reads tile at given position to the destination
		* Returns false if no tile contains the position
		*/
		virtual const tile* read(const gmtry2i::vector2i& p) = 0;

		/*
		* Writes a tile stream to the destination, which can be used to stream out all tiles from a desired rectangular region of the map
		* TILE STREAM MUST BE DELETED MANUALLY
		*/
		virtual tile_stream<tile>* read() = 0;
	};

	template <unsigned int log2_w, typename tile>
	class map_ostream {
	public:
		/*
		* Writes the tile to the given position in the map
		* Expands or extends map as necessary
		*/
		virtual void write(const gmtry2i::vector2i& p, const tile* src) = 0;

		/*
		* Writes every tile from the tile stream to the map, assuming only tile-level allignment
		* Expands or extends map as necessary
		*/
		virtual void write(tile_stream<tile>* src) = 0;

		/*
		* Accessors for writing mode (affects all writing operations performed on the map file)
		*/
		virtual tile_write_mode get_wmode() = 0;
		virtual void set_wmode(tile_write_mode new_write_mode) = 0;
	};

	template <unsigned int log2_w, typename tile>
	class map_iostream : public map_istream<tile>, public map_ostream<log2_w, tile> {};

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
	* Allows for easy iteration over a source of tiles
	* Might contain nothing
	* DOES NOT AUTOMATICALLY DELETE TILE SOURCE
	*/
	template <unsigned int log2_tile_w, typename tile>
	class map_tstream : public tile_stream<tile> {
	protected:
		tree_info info;
		void** items;
		gmtry2i::vector2i* origins;
		unsigned int* branch_indices;
		unsigned int current_level;
		gmtry2i::aligned_box2i bounds;
		virtual void update_next_item() {
			items[current_level + 1] = static_cast<spacial_tree<void>*>
				(items[current_level])->branch[branch_indices[current_level]];
		}
		virtual tile* get_tile(void* item) {
			return static_cast<tile*>(item);
		}
	public:
		void reset() {
			origins[0] = info.origin;
			branch_indices[0] = 0;
			current_level = 0;
		}
		map_tstream(const spacial_item<log2_tile_w, tile, void>& item) {
			info = item.info;
			items = new void* [info.depth + 1];
			items[0] = item.ptr;
			origins = new gmtry2i::vector2i[info.depth + 1];
			branch_indices = new unsigned int[info.depth];
			bounds = maps2::get_bounds<log2_tile_w>(info);
			reset();
		}
		const tile* next() {
			if (info.depth == 0) {
				if (branch_indices[0]) return 0;
				else {
					branch_indices[0]++;
					return get_tile(items[0]);
				}
			}
			unsigned int current_depth, half_width;
			while (branch_indices[0] < 4) {
				if (branch_indices[current_level] > 3) branch_indices[--current_level]++;
				else {
					current_depth = info.depth - current_level;
					half_width = 1 << (log2_tile_w + current_depth - 1);
					origins[current_level + 1] = origins[current_level]
						+ get_next_branch_disp(branch_indices[current_level], half_width);
					update_next_item();
					if (items[current_level + 1] && gmtry2i::intersects(bounds,
						gmtry2i::aligned_box2i(origins[current_level + 1], half_width))) {
						if (current_depth == 1) {
							tile* next_tile = get_tile(items[current_level + 1]);
							branch_indices[current_level]++;
							return next_tile;
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
			bounds = gmtry2i::intersection(maps2::get_bounds<log2_tile_w>(info), align_out(new_bounds, info.origin, log2_tile_w));
			reset();
		}
		~map_tstream() {
			for (int i = 0; i < info.depth; i++) items[i] = 0;
			delete[] items;
			delete[] origins;
			delete[] branch_indices;
		}
	};

	/*
	* Returns the info of the sub-item from the given item that most tightly fits the given bounds inside
	* Returns an item info of depth 1 and origin (0, 0) if the map does not contain the boundary box
	* DOES NOT AUTOMATICALLY MODIFY MAP TO FIT THE GIVEN BOUNDS
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

	template <unsigned int log2_w, typename tile>
	class map_buffer : public map_iostream<log2_w, tile>, protected expandable_map<log2_w> {
	protected:
		typedef spacial_tree<void> map_tree;
		typedef spacial_item<log2_w, tile, void> map_item;
		/*
		* item: is a map_tree* if depth > 0 or a tile* if depth == 0
		* depth: number of layers below layer of the item parameter (depth at root of tree = #layers - 1;
																	 depth at base of tree = 0)
		*/
		template <typename tile>
		void delete_map_tree(void* item, unsigned int depth) {
			if (depth > 0 && item) {
				for (int i = 0; i < 4; i++)
					delete_map_tree<tile>(static_cast<map_tree*>(item)->branch[i], depth - 1);
				delete static_cast<map_tree*>(item);
			}
			else delete static_cast<tile*>(item);
		}

		tree_info info;
		map_tree* root;
		tile_write_mode write_mode;

		map_item get_top_item() {
			return map_item(root, info);
		}
		/*
		* Doubles the width of the map, expanding it in the given direction
		*/
		void expand(const gmtry2i::vector2i& direction) {
			long map_init_width = 1 << (info.depth + log2_w);
			unsigned int old_root_index = (direction.x < 0) + 2 * (direction.y < 0);
			map_tree* new_root = new map_tree();
			new_root->branch[old_root_index] = root;
			root = new_root;
			info.depth += 1;
			info.origin -= gmtry2i::vector2i(direction.x < 0, direction.y < 0) * map_init_width;
		}
		/*
		* Retrieves the map item closest to a given point at a given depth
		* DOES NOT AUTOMATICALLY MODIFY THE MAP
		*/
		map_item seek_item(const map_item& start, const gmtry2i::vector2i& p, unsigned int depth) {
			map_item item = map_item();
			map_item next_item = start;
			while (next_item.ptr && next_item.info.depth > depth) {
				item = next_item;
				next_item = item.get_branch_item(p);
			}
			return next_item.ptr ? next_item : item;
		}
		/*
		* Allocates an item in the map at the desired position and depth and retrieves it
		* Returns a null item (all 0s) if the map does not contain the item's boundary box
		* DOES NOT AUTOMATICALLY MODIFY MAP TO FIT THE ITEM
		*/
		map_item alloc_item(const map_item& start, const gmtry2i::vector2i& p, unsigned int depth) {
			map_item item = map_item();
			map_item next_item = seek_item(start, p, depth);
			while (next_item.info.depth > depth) {
				item = next_item;
				next_item = item.create_branch_item(p, new map_tree());
			}
			if (depth == 0 && item.ptr) {
				delete static_cast<map_tree*>(next_item.ptr);
				next_item.ptr = static_cast<map_tree*>(item.ptr)->branch[item.get_branch_idx(p)] = new tile();
			}
			return next_item;
		}

	public:
		map_buffer(gmtry2i::vector2i origin) {
			info = tree_info(origin, 1);
			root = new map_tree();
			write_mode = TILE_OVERWRITE_MODE;
		}
		const tile* read(const gmtry2i::vector2i& p) {
			if (!gmtry2i::contains(get_bounds(), p)) return 0;
			map_item deepest_item = seek_item(get_top_item(), p, 0);
			if (deepest_item.info.depth == 0) return static_cast<tile*>(deepest_item.ptr);
			else return 0;
		}
		tile_stream<tile>* read() {
			return new map_tstream<log2_w, tile>(get_top_item());
		}
		void write(const gmtry2i::vector2i& p, const tile* src) {
			this->fit(p);
			map_item dst = alloc_item(get_top_item(), p, 0);
			*static_cast<tile*>(dst.ptr) = *src;
		}
		void write(tile_stream<tile>* src) {
			if (gmtry2i::area(src->get_bounds()) == 0) return;
			this->fit(src->get_bounds());
			tree_info virtual_min_dst_item = get_matching_item_info<log2_w>(info, src->get_bounds());
			map_item min_dst = alloc_item(get_top_item(), virtual_min_dst_item.origin, virtual_min_dst_item.depth);
			const tile* next_tile;
			tile* map_tile;
			while (next_tile = src->next()) {
				map_tile = static_cast<tile*>(alloc_item(min_dst, src->last_origin(), 0).ptr);
				if (write_mode) *map_tile += *next_tile;
				else *map_tile = *next_tile;
			}
		}
		gmtry2i::aligned_box2i get_bounds() {
			return maps2::get_bounds<log2_w>(info);
		}
		tile_write_mode get_wmode() {
			return write_mode;
		}
		void set_wmode(tile_write_mode new_write_mode) {
			write_mode = new_write_mode;
		}
		~map_buffer() {
			delete_map_tree<tile>(root, info.depth);
			root = 0;
		}
	};

	template <unsigned int log2_w, typename tile>
	class map_fstream : public map_iostream<log2_w, tile>, protected expandable_map<log2_w> {
	protected:
		typedef std::uint32_t file_pos;
		const unsigned int tree_size = 4 * sizeof(file_pos);

		/*
		* root: position of root tree in file
		* size: size of file
		*/
		struct bmap_file_header {
			unsigned int log2_tile_w;
			file_pos root;
			file_pos size;
		};
		tree_info info;
		std::uint32_t file_raw_depth;
		std::uint64_t file_raw_origin[2];
		std::uint32_t file_raw_log2_tile_w;
		const unsigned int file_raw_header_size = sizeof(file_raw_depth) + 2 * sizeof(*file_raw_origin) + 
												  sizeof(file_raw_log2_tile_w) + 2 * sizeof(file_pos);
		/*
		* Both trees and tiles are indexed as an index_tree
		*		A tile's index_tree will not have any nonzero branches
		* All subitems (even those that don't exist) of an item are indexed when any one of them are indexed,
			saving time in future searches
		* Either none or all of an index tree's branches are indexed
		* pos: position of the represented item in the file
		* fully_indexed: tells whether the tree and all of its descendents are indexed
		*/
		struct index_tree : public spacial_tree<index_tree> {
			file_pos pos;
			index_tree(file_pos item_pos) : spacial_tree<index_tree>() {
				pos = item_pos;
			}
		};
		static void delete_index_tree(index_tree* tree, unsigned int depth) {
			if (depth > 0 && tree) {
				for (int i = 0; i < 4; i++)
					delete_index_tree(tree->branch[i], depth - 1);
				delete tree;
			}
		}
		typedef spacial_item<log2_w, tile, index_tree> item_index;

		bmap_file_header map_header;
		std::fstream file;
		std::string file_name;
		index_tree* indices;
		tile_write_mode writemode;
		bool header_has_unsaved_changes;
		const tile blank = tile();
		tile last_tile;

		inline void read_file(void* dst, file_pos pos, unsigned long len) {
			file.seekg(pos);
			file.read(static_cast<char*>(dst), len);
		}
		// cannot be used to append
		inline void write_file(const void* src, file_pos pos, unsigned long len) {
			file.seekp(pos);
			file.write(static_cast<const char*>(src), len);
		}
		inline void append_file(const void* src, unsigned long len) {
			file.seekp(map_header.size);
			file.write(static_cast<const char*>(src), len);
			map_header.size += len;
			header_has_unsaved_changes = true;
			std::cout << "Added to file size: " << len << std::endl; //test
			std::cout << "New file size: " << map_header.size << std::endl; //test
		}
		inline void write_branch(file_pos new_branch, unsigned int branch_index, file_pos tree_pos) {
			std::cout << new_branch << " written to branch " << branch_index << " of tree at " << tree_pos << std::endl; //test
			file.seekp(tree_pos + sizeof(new_branch) * branch_index);
			file.write(reinterpret_cast<char*>(&new_branch), sizeof(new_branch));

			file_pos next_branches[4]; //test
			read_file(next_branches, tree_pos, tree_size); //test
			std::cout << "Resultant tree at " << tree_pos << ":" << std::endl; //test
			std::cout << "{ " << next_branches[0] << ", " << next_branches[1] << ", " << 
				next_branches[2] << ", " << next_branches[3] << " }" << std::endl; //test
		}
		inline void write_tile(const tile* src, file_pos pos) {
			std::cout << "Tile written to file at " << pos << std::endl; //test
			if (writemode) {
				tile current_tile = tile();
				read_file(&current_tile, pos, sizeof(tile));
				current_tile += *src;
				write_file(&current_tile, pos, sizeof(tile));
			}
			else {
				write_file(src, pos, sizeof(tile));
			}
		}
		void read_header() {
			file_pos current_pos = 0;
			unsigned int member_size;

			read_file(&file_raw_depth, current_pos, member_size = sizeof(file_raw_depth));
			current_pos += member_size;
			info.depth = static_cast<unsigned int>(file_raw_depth);

			read_file(file_raw_origin, current_pos, member_size = 2 * sizeof(*file_raw_origin));
			current_pos += member_size;
			info.origin.x = static_cast<unsigned long>(file_raw_origin[0]);
			info.origin.y = static_cast<unsigned long>(file_raw_origin[1]);

			read_file(&file_raw_log2_tile_w, current_pos, member_size = sizeof(file_raw_log2_tile_w));
			current_pos += member_size;
			map_header.log2_tile_w = static_cast<unsigned int>(file_raw_log2_tile_w);

			read_file(&map_header.root, current_pos, 2 * sizeof(file_pos));
		}
		void write_header() {
			file_pos current_pos = 0;
			unsigned int member_size;

			file_raw_depth = static_cast<std::uint32_t>(info.depth);
			write_file(&file_raw_depth, current_pos, member_size = sizeof(file_raw_depth));
			current_pos += member_size;

			file_raw_origin[0] = static_cast<std::uint64_t>(info.origin.x);
			file_raw_origin[1] = static_cast<std::uint64_t>(info.origin.y);
			write_file(file_raw_origin, current_pos, member_size = 2 * sizeof(*file_raw_origin));
			current_pos += member_size;

			file_raw_log2_tile_w = static_cast<std::uint32_t>(map_header.log2_tile_w);
			write_file(&file_raw_log2_tile_w, current_pos, member_size = sizeof(file_raw_log2_tile_w));
			current_pos += member_size;

			write_file(&map_header.root, current_pos, 2 * sizeof(file_pos));

			header_has_unsaved_changes = false;
		}
		inline gmtry2i::aligned_box2i get_map_bounds() {
			return maps2::get_bounds<log2_w>(info);
		}
		void expand(const gmtry2i::vector2i& direction) {
			long map_init_width = 1 << (info.depth + log2_w);
			unsigned int old_root_index = (direction.x < 0) + 2 * (direction.y < 0);
			index_tree* new_root = new index_tree(map_header.size);
			new_root->branch[old_root_index] = indices;
			indices = new_root;
			for (int i = 0; i < 4; i++) if (i != old_root_index) indices->branch[i] = new index_tree(0);

			file_pos new_root_branches[4] = { 0 };
			new_root_branches[old_root_index] = map_header.root;
			append_file(new_root_branches, tree_size);

			map_header.root = new_root->pos;
			info.depth += 1;
			info.origin -= gmtry2i::vector2i(direction.x < 0, direction.y < 0) * map_init_width;
			header_has_unsaved_changes = true;
			std::cout << "Map Expanded!" << std::endl;
			std::cout << "New root: " << map_header.root << std::endl; //test
			std::cout << "New depth: " << info.depth << std::endl; //test
			std::cout << "New origin: " << info.origin.x << ", " << info.origin.y << std::endl; //test
		}
		item_index get_top_item() {
			return item_index(indices, info);
		}
		/*
		* Returns the existing item closest to the desired depth that contains the given point and is descended from start
		* Start must exist, and the returned item won't contain the point if start doesn't contain the point
		*/
		item_index seek_item(const item_index& start, const gmtry2i::vector2i& p, unsigned int depth) {
			item_index item = item_index();
			item_index next_item = start;
			unsigned int next_branch_idx;
			file_pos next_branches[4];
			while (next_item.ptr->pos && next_item.info.depth > depth) {
				item = next_item;
				next_branch_idx = item.get_branch_idx(p);
				if (item.ptr->branch[next_branch_idx] == 0) {
					read_file(next_branches, item.ptr->pos, tree_size);
					for (int i = 0; i < 4; i++) item.ptr->branch[i] = new index_tree(next_branches[i]);

					std::cout << "Tree at depth " << item.info.depth << 
						" indexed from " << item.ptr->pos << ":" << std::endl; //test
					std::cout << "{ " << next_branches[0] << ", " << next_branches[1] << ", " <<
						next_branches[2] << ", " << next_branches[3] << " }" << std::endl; //test
				}
				next_item = item.get_branch_item(next_branch_idx);
			}
			return next_item.ptr->pos ? next_item : item;
		}
		/*
		* Allocates an item at the desired position and depth and returns its index
		* Returns a real item if start is real, although it won't contain the point if start doesn't contain the point
		*/
		item_index alloc_item(const item_index& start, const gmtry2i::vector2i p, unsigned int depth) {
			item_index item = seek_item(start, p, depth);
			if (item.info.depth == depth) return item;
			unsigned int next_branch_idx = item.get_branch_idx(p);
			write_branch(map_header.size, next_branch_idx, item.ptr->pos);
			// the only items that make it to this point have a next_item with a null pos
			item.ptr->branch[next_branch_idx]->pos = map_header.size;
			item = item.get_branch_item(next_branch_idx);
			while (item.info.depth > depth) {
				file_pos branches[4] = { 0, 0, 0, 0 };
				next_branch_idx = item.get_branch_idx(p);
				branches[next_branch_idx] = map_header.size + tree_size;
				append_file(branches, tree_size);
				for (int i = 0; i < 4; i++) item.ptr->branch[i] = new index_tree(branches[i]);

				std::cout << "File appended with tree at " << (branches[next_branch_idx] - tree_size) << 
					" at depth " << item.info.depth << ":" << std::endl; //test
				std::cout << "{ " << branches[0] << ", " << branches[1] << ", " << 
					branches[2] << ", " << branches[3] << " }" << std::endl; //test

				item = item.get_branch_item(next_branch_idx);
			}
			append_file(&blank, depth ? tree_size : sizeof(tile));
			return item;
		}

		class map_fstream_tstream : public map_tstream<log2_w, tile> {
		protected:
			map_fstream* src;
			tile last_tile;
			void update_next_item() {
				index_tree* current = static_cast<index_tree*>(this->items[this->current_level]);
				if (current->branch[0] == 0) {
					file_pos branches[4] = { 0, 0, 0, 0 }; //remove 0 initializations
					src->read_file(branches, current->pos, src->tree_size);
					for (int i = 0; i < 4; i++) current->branch[i] = new index_tree(branches[i]);

					std::cout << "Tree at depth " << (this->info.depth - this->current_level) <<
						" and position " << gmtry2i::to_string(this->origins[this->current_level]) <<
						" indexed from " << current->pos << ":" << std::endl; //test
					std::cout << "{ " << branches[0] << ", " << branches[1] << ", " << 
						branches[2] << ", " << branches[3] << " }" << std::endl; //test
				}
				this->items[this->current_level + 1] = current->branch[this->branch_indices[this->current_level]];
				if (static_cast<index_tree*>(this->items[this->current_level + 1])->pos == 0)
					this->items[this->current_level + 1] = 0;
			}
			tile* get_tile(void* item) {
				src->read_file(&last_tile, static_cast<index_tree*>(item)->pos, sizeof(last_tile));
				std::cout << "Tile at position " << gmtry2i::to_string(this->origins[this->current_level + 1]) <<
					" built from " << static_cast<index_tree*>(item)->pos << std::endl; //test
				return &last_tile;
			}
		public:
			map_fstream_tstream(item_index item, map_fstream* source) : 
				map_tstream<log2_w, tile>(spacial_item<log2_w, tile, void>(item.ptr, item.info)) {
				src = source;
				last_tile = tile();
			}
		};

	public:
		map_fstream(const std::string& fname, const gmtry2i::vector2i& origin) {
			file_name = fname;
			ios::openmode filemode = ios::binary | ios::in | ios::out;
			file.open(file_name, filemode);
			if (!(file.is_open())) {
				FILE* tmp_file = 0;
				fopen_s(&tmp_file, file_name.c_str(), "w");
				fclose(tmp_file);
				file.open(file_name, filemode);
				info = tree_info(origin, 1);
				map_header.log2_tile_w = log2_w;
				map_header.root = file_raw_header_size;
				map_header.size = file_raw_header_size;
				write_header();
				file_pos root_branches[4] = { 0, 0, 0, 0 };
				append_file(root_branches, tree_size);
				std::cout << "File created!" << std::endl; //test
			}
			else {
				read_header();
				std::cout << "File already exists!" << std::endl; //test
			}
			if (!(file.is_open())) throw ios::failure("Map file cannot be opened");

			std::cout << "Recorded log2_tile_w: " << map_header.log2_tile_w << std::endl; //test
			std::cout << "Recorded depth: " << info.depth << std::endl; //test
			std::cout << "Recorded origin: " << info.origin.x << ", " << info.origin.y << std::endl; //test
			std::cout << "Recorded root: " << map_header.root << std::endl; //test
			std::cout << "Recorded file length: " << map_header.size << std::endl; //test


			if (log2_w != map_header.log2_tile_w || map_header.size < file_raw_header_size) 
				throw ios::failure("Map file formatted improperly");

			last_tile = blank;
			indices = new index_tree(map_header.root);
			writemode = TILE_OVERWRITE_MODE;
			header_has_unsaved_changes = false;
		}
		const tile* read(const gmtry2i::vector2i& p) {
			if (!(file.is_open())) throw ios::failure("Cannot read map file");
			if (!gmtry2i::contains(get_map_bounds(), p)) return 0;
			item_index deepest_item = seek_item(get_top_item(), p, 0);
			if (deepest_item.info.depth == 0) {
				read_file(&last_tile, deepest_item.ptr->pos, sizeof(tile));
				return &last_tile;
			}
			else return 0;
		}
		tile_stream<tile>* read() {
			if (!(file.is_open())) throw ios::failure("Cannot read map file");
			return new map_fstream_tstream(get_top_item(), this);
		}
		void write(const gmtry2i::vector2i& p, const tile* src) {
			if (!(file.is_open())) throw ios::failure("Cannot write to map file");
			this->fit(p);
			item_index dst = alloc_item(get_top_item(), p, 0);
			write_tile(src, dst.ptr->pos);
			std::cout << "File appended with new tile!" << std::endl; //test
		}
		void write(tile_stream<tile>* src) {
			if (!(file.is_open())) throw ios::failure("Cannot write to map file");
			if (gmtry2i::area(src->get_bounds()) == 0) return;
			this->fit(src->get_bounds());
			std::cout << "Bounds of incoming tile stream: " << gmtry2i::to_string(src->get_bounds()) << std::endl; //test
			tree_info virtual_min_dst = get_matching_item_info<log2_w>(info, src->get_bounds());
			std::cout << "Bounds of virtual destination for incoming tiles: " << 
				gmtry2i::to_string(maps2::get_bounds<log2_w>(virtual_min_dst)) << std::endl; //test
			item_index min_dst = alloc_item(get_top_item(), virtual_min_dst.origin, virtual_min_dst.depth);
			const tile* stream_tile;
			item_index map_tile;
			while (stream_tile = src->next()) {
				map_tile = alloc_item(min_dst, src->last_origin(), 0);
				//std::cout << "Depth of written tile (better be 0 or else ima go insaneo style): " << map_tile.depth << std::endl; //test
				std::cout << "Origin of written tile: " << gmtry2i::to_string(map_tile.info.origin) << std::endl;
				write_tile(stream_tile, map_tile.ptr->pos);
			}
		}
		gmtry2i::aligned_box2i get_bounds() {
			return get_map_bounds();
		}
		tile_write_mode get_wmode() {
			return writemode;
		}
		void set_wmode(tile_write_mode new_write_mode) {
			writemode = new_write_mode;
		}
		~map_fstream() {
			if (file.is_open()) {
				std::cout << "Final size of map file: " << map_header.size << std::endl; //test
				if (header_has_unsaved_changes) write_header();
				file.close();
			}
			delete_index_tree(indices, info.depth);
		}
	};
}