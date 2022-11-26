#pragma once

#include <fstream>
#include <stdio.h>
#include <iostream>

#include "geometry.hpp"

namespace tmaps2 {
	enum tile_write_mode {
		TILE_OVERWRITE_MODE = 0,
		TILE_ADD_MODE = 1
	};

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

	template <typename base_tile>
	struct nbrng_tile : base_tile {
		nbrng_tile<base_tile>* nbrs[8];
	};

	template <typename tile>
	class tile_stream {
	public:
		// Returns the stream to its initial conditions
		virtual void reset() = 0;
		// Returns the next tile in the stream, or 0 if all tiles have been retrieved
		virtual const tile* next() = 0;
		// Returns the origin of the last tile retrieved from next()
		virtual gmtry2i::vector2i last_origin() = 0;
		// Returns a bounds on all of the outgoing tiles
		virtual gmtry2i::aligned_box2i get_bounds() = 0;
		// Sets a bounds on the outgoing tiles. New bounds will automatically be tile-aligned if necessary
		virtual void set_bounds(const gmtry2i::aligned_box2i& new_bounds) = 0;
		virtual ~tile_stream() {};
	};

	template <typename tile>
	class map_istream {
	public:
		/*
		* Reads tile at given position to the destination
		* Returns false if no tile contains the position
		*/
		virtual bool read(const gmtry2i::vector2i& p, tile* dst) = 0;

		/*
		* Writes a tile stream to the destination, which can be used to stream out all tiles from a desired rectangular region of the map
		* TILE STREAM MUST BE DELETED MANUALLY
		*/
		virtual bool read(tile_stream<tile>** dst) = 0;

		/*
		* Retrieves a reference to the stream's read mode (whether to add to or overwrite the map being read to)
		*/
		virtual tile_write_mode& read_mode() = 0;

		/*
		* Returns the current bounds of the map
		*/
		virtual gmtry2i::aligned_box2i get_bounds() = 0;
	};

	template <typename tile>
	class map_ostream {
	public:
		/*
		* Writes the tile to the given position in the map
		* Expands or extends map as necessary
		*/
		virtual bool write(const gmtry2i::vector2i& p, const tile* src) = 0;

		/*
		* Writes every tile from the tile stream to the map, assuming only tile-level allignment
		* Expands or extends map as necessary
		*/
		virtual bool write(tile_stream<tile>* src) = 0;

		/*
		* Retrieves a reference to the stream's write mode (whether to add to or overwrite the map being written to)
		*/
		virtual tile_write_mode& write_mode() = 0;
	};

	template <typename tile>
	class map_iostream : map_istream<tile>, map_ostream<tile> {};

	/*
	* Holds information about a map item
	* depth: depth of the map (# of layers in the map above the tile layer)
	* origin: origin (bottom left corner position) of map
	*/
	struct map_info {
		gmtry2i::vector2i origin;
		unsigned int depth;
		map_info() {
			origin = gmtry2i::vector2i();
			depth = 0;
		}
		map_info(const gmtry2i::vector2i map_origin) {
			origin = map_origin;
			depth = 0;
		}
		map_info(const gmtry2i::vector2i map_origin, unsigned int map_depth) {
			origin = map_origin;
			depth = map_depth;
		}
	};

	struct map_tree {
		void* branch[4];
		map_tree() = default;
	};

	inline gmtry2i::vector2i get_next_branch_disp(unsigned int branch_idx, unsigned long hwidth) {
		return gmtry2i::vector2i(branch_idx & 1, branch_idx >> 1) * hwidth;
	}

	inline unsigned int get_next_branch_idx(const gmtry2i::vector2i& origin, 
											const gmtry2i::vector2i& next_origin, unsigned long hwidth) {
		return (next_origin.x - origin.x >= hwidth) + 2 * (next_origin.y - origin.y >= hwidth);
	}

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

	/*
	* Holds main copy of map data
	* All map data is properly disposed of when the map is deleted
	* info: holds the depth and origin of the root
	* root: non-null map_tree pointer
	*/
	template <typename tile>
	struct map {
		map_info info;
		map_tree* root;
		map() {
			info = map_info(gmtry2i::vector2i(), 1);
			root = new map_tree();
		}
		map(const gmtry2i::vector2i& origin) {
			info = map_info(origin, 1);
			root = new map_tree();
		}
		~map() {
			delete_map_tree<tile>(root, info.depth);
		}
	};

	/*
	* Temporarily holds information about a part of the map (either a tree or a tile)
	* Holds same data as a map, but it shouldn't be used to hold the primary pointers to things
	*		When deleted, the actual item isn't deleted
	* Ptr is always non-null, and the item it points to can have any depth
	*/
	template <typename tile>
	struct map_item {
		map_info info;
		void* ptr;
		map_item() = default;
		map_item(map<tile>* map) {
			info = map_info(map->info.origin, map->info.depth);
			ptr = map->root;
		}
		map_item(gmtry2i::vector2i item_origin, unsigned int item_depth) {
			info = map_info(item_origin, item_depth);
			ptr = 0;
		}
		map_item(void* item_ptr, gmtry2i::vector2i item_origin, unsigned int item_depth) {
			info = map_info(item_origin, item_depth);
			ptr = item_ptr;
		}
		map_item(void* item_ptr, const map_info& item_info) {
			info = item_info;
			ptr = item_ptr;
		}
	};

	template <unsigned int log2_tile_w>
	inline gmtry2i::aligned_box2i get_bounds(const map_info& info) {
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
		map_info info;
		void** items;
		gmtry2i::vector2i* origins;
		unsigned int* branch_indices;
		unsigned int current_level;
		gmtry2i::aligned_box2i bounds;
		virtual void update_next_item() {
			items[current_level + 1] = static_cast<map_tree*>(items[current_level])->branch[branch_indices[current_level]];
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
		map_tstream(const map_item<tile>& item) {
			info = item.info;
			items = new void* [info.depth + 1];
			items[0] = item.ptr;
			origins = new gmtry2i::vector2i[info.depth + 1];
			branch_indices = new unsigned int[info.depth];
			bounds = tmaps2::get_bounds<log2_tile_w>(info);
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
			unsigned int current_depth;
			while (branch_indices[0] < 4) {
				if (branch_indices[current_level] > 3) branch_indices[--current_level]++;
				else {
					current_depth = info.depth - current_level;
					origins[current_level + 1] = origins[current_level]
						+ get_next_branch_disp(branch_indices[current_level], 1 << (log2_tile_w + current_depth - 1));
					update_next_item();
					if (items[current_level + 1] && gmtry2i::intersects(bounds,
						gmtry2i::aligned_box2i(origins[current_level + 1], 1 << (current_depth + log2_tile_w - 1)))) {
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
			bounds = gmtry2i::intersection(tmaps2::get_bounds<log2_tile_w>(info), align_out<log2_tile_w>(new_bounds, info.origin));
		}
		~map_tstream() {
			for (int i = 0; i < info.depth; i++) items[i] = 0;
			delete[] items;
			delete[] origins;
			delete[] branch_indices;
		}
	};

	/*
	* Doubles the width of the map, expanding it in the given direction
	*/
	template <unsigned int log2_w, typename tile>
	void expand_map(map<tile>* map, const gmtry2i::vector2i& direction) {
		long map_init_width = 1 << (map->info.depth + log2_w);
		unsigned int old_root_index = (direction.x < 0) + 2 * (direction.y < 0);
		map_tree* new_root = new map_tree();
		new_root->branch[old_root_index] = map->root;
		map->root = new_root;
		map->info.depth += 1;
		map->info.origin -= gmtry2i::vector2i(direction.x < 0, direction.y < 0) * map_init_width;
	}

	/*
	* Fits the position into the map by expanding it if necessary
	*/
	template <unsigned int log2_w, typename tile>
	void fit_map(map<tile>* map, const gmtry2i::vector2i p) {
		gmtry2i::aligned_box2i alloc_box = get_bounds<log2_w>(map->info);
		while (!gmtry2i::contains(alloc_box, p)) {
			expand_map<log2_w, tile>(map, p - gmtry2i::center(alloc_box));
			alloc_box = get_bounds<log2_w>(map->info);
		}
	}

	/*
	* Fits the box into the map by expanding it if necessary
	*/
	template <unsigned int log2_w, typename tile>
	void fit_map(map<tile>* map, const gmtry2i::aligned_box2i box) {
		gmtry2i::aligned_box2i alloc_box = get_bounds<log2_w>(map->info);
		gmtry2i::vector2i box_center = gmtry2i::center(box);
		while (!gmtry2i::contains(alloc_box, box)) {
			expand_map<log2_w, tile>(map, box_center - gmtry2i::center(alloc_box));
			alloc_box = get_bounds<log2_w>(map->info);
		}
	}

	/*
	* Returns the info of the item from the map that most tightly fits the given bounds
	* Returns an item info of depth 1 and origin (0, 0) if the map does not contain the boundary box
	* DOES NOT AUTOMATICALLY MODIFY MAP TO FIT THE GIVEN BOUNDS
	*/
	template <unsigned int log2_w>
	map_info get_matching_item_info(const map_info& item_info, const gmtry2i::aligned_box2i& bounds) {
		map_info matching_item = map_info(gmtry2i::vector2i(), 1);
		map_info next_matching_item(item_info.origin, item_info.depth);
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

	/*
	* Retrieves the map item closest to a given point at a given depth
	* DOES NOT AUTOMATICALLY MODIFY THE MAP
	*/
	template <unsigned int log2_w, typename tile>
	map_item<tile> get_map_item(const map_item<tile>& src, const gmtry2i::vector2i& p, unsigned int depth) {
		map_item<tile> item = map_item<tile>();
		map_item<tile> next_item = src;
		unsigned int next_branch_index;
		unsigned int hwidth = (1 << (next_item.info.depth + log2_w)) >> 1;
		while (next_item.ptr && next_item.info.depth > depth) {
			item = next_item;
			next_branch_index = (p.x - item.info.origin.x >= hwidth) + 2 * (p.y - item.info.origin.y >= hwidth);
			next_item.ptr = static_cast<map_tree*>(item.ptr)->branch[next_branch_index];
			next_item.info.depth = item.info.depth - 1;
			next_item.info.origin = item.info.origin + gmtry2i::vector2i(next_branch_index & 1, next_branch_index >> 1) * hwidth;
			hwidth >>= 1;
		}
		return next_item.ptr ? next_item : item;
	}

	/*
	* Allocates an item in the map at the desired position and depth and retrieves it
	* Returns a null item (all 0s) if the map does not contain the item's boundary box
	* DOES NOT AUTOMATICALLY MODIFY MAP TO FIT THE ITEM
	*/
	template <unsigned int log2_w, typename tile>
	map_item<tile> alloc_map_item(map_item<tile> src, const gmtry2i::vector2i& p, unsigned int depth) {
		map_item<tile> item = map_item<tile>();
		map_item<tile> next_item = get_map_item<log2_w, tile>(src, p, depth);
		unsigned int next_branch_index;
		unsigned int hwidth = (1 << (next_item.info.depth + log2_w)) >> 1;
		while (next_item.info.depth > depth) {
			item = next_item;
			next_branch_index = (p.x - item.info.origin.x >= hwidth) + 2 * (p.y - item.info.origin.y >= hwidth);
			next_item.ptr = static_cast<map_tree*>(item.ptr)->branch[next_branch_index] = new map_tree();
			next_item.info.depth = item.info.depth - 1;
			next_item.info.origin = item.info.origin + gmtry2i::vector2i(next_branch_index & 1, next_branch_index >> 1) * hwidth;
			hwidth >>= 1;
		}
		if (depth == 0 && item.ptr) {
			delete static_cast<map_tree*>(next_item.ptr);
			next_item.ptr = static_cast<map_tree*>(item.ptr)->branch[next_branch_index] = new tile();
		}
		return next_item;
	}

	/*
	* Copies all tiles from an item to a map, assuming only alignment on the tile level
	* Automatically expands the map or extends its branches such that it will contain all tiles from the item
	*/
	template <unsigned int log2_w, typename tile>
	map_item<tile> set_map_tiles(map<tile>* dst, tile_stream<tile>* src, tile_write_mode mode) {
		if (gmtry2i::area(src->get_bounds()) == 0) return map_item<tile>();
		fit_map<log2_w, tile>(dst, src->get_bounds());
		map_info virtual_min_dst_item = get_matching_item_info<log2_w>(dst->info, src->get_bounds());
		map_item<tile> min_dst = alloc_map_item<log2_w, tile>(map_item<tile>(dst), virtual_min_dst_item.origin, virtual_min_dst_item.depth);
		const tile* next_tile;
		tile* map_tile;
		while (next_tile = src->next()) {
			map_tile = static_cast<tile*>(alloc_map_item<log2_w, tile>(min_dst, src->last_origin(), 0).ptr);
			if (mode) *map_tile += *next_tile;
			else *map_tile = *next_tile;
		}
		return min_dst;
	}

	template <unsigned int log2_w, typename tile>
	class map_fstream : map_iostream<tile> {
	protected:
		/*
		* root: position of root tree in file
		* size: size of file
		*/
		struct bmap_file_header {
			map_info info;
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
			index_tree* branch[4];
			index_tree(unsigned long item_pos) {
				pos = item_pos;
				branch[0] = 0;
				branch[1] = 0;
				branch[2] = 0;
				branch[3] = 0;
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
		tile_write_mode readmode;
		tile_write_mode writemode;
		const tile blank = tile();

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
			std::cout << "Branch written to position " << file.tellp() << std::endl;
			file.write(reinterpret_cast<char*>(&new_branch), sizeof(new_branch));
			std::cout << "Position of end of branch at " << file.tellp() << std::endl;


			unsigned long next_branches[4];
			read_file(next_branches, tree_pos, 4 * sizeof(unsigned long));
			std::cout << "Resultant tree at " << tree_pos << ":" << std::endl; //test
			std::cout << "{ " << next_branches[0] << ", " << next_branches[1] << ", " << next_branches[2] << ", " << next_branches[3] << " }" << std::endl; //test
		}
		inline void write_tile(const tile* src, unsigned long pos) {
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
		inline gmtry2i::aligned_box2i get_map_bounds() {
			return tmaps2::get_bounds<log2_w>(map_header.info);
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
		* Returns the existing item closest to the desired depth that contains the given point and is descended from start
		* Start must exist, and the returned item won't contain the point if start doesn't contain the point
		*/
		item_index seek_item_at_depth(const item_index& start, const gmtry2i::vector2i& p, unsigned int depth) {
			item_index item = item_index();
			item_index next_item(start.tree, start.depth, start.origin);
			unsigned int hwidth = 1 << (next_item.depth + log2_w - 1);
			unsigned int next_branch_idx;
			unsigned long next_branches[4];
			while (next_item.tree->pos && next_item.depth > depth) {
				item = next_item;
				next_branch_idx = get_next_branch_idx(item.origin, p, hwidth);
				if (item.tree->branch[next_branch_idx] == 0) {
					read_file(next_branches, item.tree->pos, 4 * sizeof(unsigned long));
					for (int i = 0; i < 4; i++) item.tree->branch[i] = new index_tree(next_branches[i]);

					std::cout << "Tree at depth " << item.depth << " indexed from " << item.tree->pos << ":" << std::endl; //test
					std::cout << "{ " << next_branches[0] << ", " << next_branches[1] << ", " << next_branches[2] << ", " << next_branches[3] << " }" << std::endl; //test
				for (int i = 0; i < 4; i++) if (next_branches[i] > 1000000)
					std::cout << "UH OH WEE WOO WEE WOO WEE WOO WEE WOO WEE WOO WEE WOO" << std::endl; //test
				}
				next_item.tree = item.tree->branch[next_branch_idx];
				next_item.depth = item.depth - 1;
				next_item.origin = item.origin + get_next_branch_disp(next_branch_idx, hwidth);
				hwidth >>= 1;
			}
			return next_item.tree->pos ? next_item : item;
		}
		/*
		* Allocates an item at the desired position and depth and returns its index
		* Returns a real item if start is real, although it won't contain the point if start doesn't contain the point
		*
		*
		* MAY INCORRECTLY ALLOCATE A TREE WHERE A TILE SHOULD BE???
		*/
		item_index alloc_item_at_depth(const item_index& start, const gmtry2i::vector2i p, unsigned int depth) {
			item_index item = seek_item_at_depth(start, p, depth);
			if (item.depth == depth) return item;
			unsigned int hwidth = 1 << (item.depth + log2_w - 1);
			unsigned int next_branch_idx = get_next_branch_idx(item.origin, p, hwidth);
			const unsigned int branches_size = 4 * sizeof(unsigned long);
			write_branch(map_header.size, next_branch_idx, item.tree->pos);
			// the only items that make it to this point have a next_item with a null pos
			item.tree->branch[next_branch_idx]->pos = map_header.size;
			item.tree = item.tree->branch[next_branch_idx];
			item.depth -= 1;
			item.origin += get_next_branch_disp(next_branch_idx, hwidth);
			hwidth >>= 1;
			while (item.depth > depth) {
				unsigned long branches[4] = { 0, 0, 0, 0 };
				next_branch_idx = get_next_branch_idx(item.origin, p, hwidth);
				branches[next_branch_idx] = map_header.size + branches_size;
				append_file(branches, branches_size);
				for (int i = 0; i < 4; i++) item.tree->branch[i] = new index_tree(branches[i]);

				std::cout << "File appended with tree at " << (branches[next_branch_idx] - branches_size) << " at depth " << item.depth << ":" << std::endl; //test
				std::cout << "{ " << branches[0] << ", " << branches[1] << ", " << branches[2] << ", " << branches[3] << " }" << std::endl; //test
				for (int i = 0; i < 4; i++) if (branches[i] > 1000000)
					std::cout << "UH OH WEE WOO WEE WOO WEE WOO WEE WOO WEE WOO WEE WOO" << std::endl; //test

				item.tree = item.tree->branch[next_branch_idx];
				item.depth -= 1;
				item.origin += get_next_branch_disp(next_branch_idx, hwidth);
				hwidth >>= 1;
			}
			append_file(&blank, depth ? branches_size : sizeof(tile));
			return item;
		}

		typedef map_tstream<log2_w, tile> maptstream;

		class map_fstream_tstream : public maptstream {
		protected:
			map_fstream* src;
			tile last_tile;
			void update_next_item() {
				index_tree* current = static_cast<index_tree*>(maptstream::items[maptstream::current_level]);
				if (current->branch[0] == 0) {
					unsigned long branches[4] = { 0, 0, 0, 0 }; //remove 0 initializations
					src->read_file(branches, current->pos, 4 * sizeof(unsigned long));
					for (int i = 0; i < 4; i++) current->branch[i] = new index_tree(branches[i]);

					std::cout << "Tree at depth " << (maptstream::info.depth - maptstream::current_level) << 
						" and position " << gmtry2i::to_string(maptstream::origins[maptstream::current_level]) <<
						" indexed from " << current->pos << ":" << std::endl; //test
					std::cout << "{ " << branches[0] << ", " << branches[1] << ", " << branches[2] << ", " << branches[3] << " }" << std::endl; //test
					for (int i = 0; i < 4; i++) if (branches[i] > 1000000)
						std::cout << "UH OH WEE WOO WEE WOO WEE WOO WEE WOO WEE WOO WEE WOO" << std::endl; //test
				}
				maptstream::items[maptstream::current_level + 1] = current->branch[maptstream::branch_indices[maptstream::current_level]];
				if (static_cast<index_tree*>(maptstream::items[maptstream::current_level + 1])->pos == 0)
					maptstream::items[maptstream::current_level + 1] = 0;
			}
			tile* get_tile(void* item) {
				src->read_file(&last_tile, static_cast<index_tree*>(item)->pos, sizeof(last_tile));
				std::cout << "Tile at position " << gmtry2i::to_string(maptstream::origins[maptstream::current_level + 1]) <<
					" built from " << static_cast<index_tree*>(item)->pos << std::endl; //test
				return &last_tile;
			}
		public:
			map_fstream_tstream(item_index item, map_fstream* source) : maptstream(map_item<tile>(item.tree, item.origin, item.depth)) {
				src = source;
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
				map_header.info = map_info(origin, 1);
				map_header.log2_tile_w = log2_w;
				map_header.root = sizeof(bmap_file_header);
				map_header.size = sizeof(bmap_file_header);
				write_file(&map_header, 0, sizeof(bmap_file_header));
				unsigned long root_branches[4] = { 0, 0, 0, 0 };
				append_file(root_branches, 4 * sizeof(unsigned long));
				std::cout << "File created!" << std::endl; //test
			}
			else {
				read_file(&map_header, 0, sizeof(bmap_file_header));
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

			readmode = TILE_OVERWRITE_MODE;
			writemode = TILE_OVERWRITE_MODE;
			indices = new index_tree(map_header.root);
		}
		bool read(const gmtry2i::vector2i& p, tile* dst) {
			if (!(file.is_open())) return false;
			if (!gmtry2i::contains(get_map_bounds(), p)) return false;
			item_index deepest_item = seek_item_at_depth(get_top_item(), p, 0);
			if (deepest_item.depth == 0) {
				if (readmode) {
					tile current_tile = tile();
					read_file(&current_tile, deepest_item.tree->pos, sizeof(tile));
					*dst += current_tile;
				}
				else read_file(dst, deepest_item.tree->pos, sizeof(tile));
				return true;
			}
			else return false;
		}
		bool read(tile_stream<tile>** dst) {
			if (!(file.is_open())) return false;
			*dst = new map_fstream_tstream(get_top_item(), this);
			return true;
		}
		bool write(const gmtry2i::vector2i& p, const tile* src) {
			if (!(file.is_open())) return false;
			fit_map(p);
			item_index dst = alloc_item_at_depth(get_top_item(), p, 0);
			write_tile(src, dst.tree->pos);
			std::cout << "File appended with new tile!" << std::endl; //test
			return true;
		}
		bool write(tile_stream<tile>* src) {
			if (!(file.is_open())) return false;
			if (gmtry2i::area(src->get_bounds()) == 0) return true;
			fit_map(src->get_bounds());
			std::cout << "Bounds of incoming tile stream: " << gmtry2i::to_string(src->get_bounds()) << std::endl; //test
			map_info virtual_min_dst = get_matching_item_info<log2_w>(map_header.info, src->get_bounds());
			std::cout << "Bounds of virtual destination for incoming tiles: " << 
				gmtry2i::to_string(tmaps2::get_bounds<log2_w>(virtual_min_dst)) << std::endl; //test
			item_index min_dst = alloc_item_at_depth(get_top_item(), virtual_min_dst.origin, virtual_min_dst.depth);
			const tile* stream_tile;
			item_index map_tile;
			while (stream_tile = src->next()) {
				map_tile = alloc_item_at_depth(min_dst, src->last_origin(), 0);
				//std::cout << "Depth of written tile (better be 0 or else ima go insaneo style): " << map_tile.depth << std::endl; //test
				std::cout << "Origin of written tile: " << gmtry2i::to_string(map_tile.origin) << std::endl;
				if (map_tile.tree == 0) { //test
					std::cout << "INVALID MAP TILE ALLOCATED" << std::endl;
					return false;
				}
				write_tile(stream_tile, map_tile.tree->pos);
			}
			return true;
		}
		gmtry2i::aligned_box2i get_bounds() {
			return get_map_bounds();
		}
		tile_write_mode& read_mode() {
			return readmode;
		}
		tile_write_mode& write_mode() {
			return writemode;
		}
		~map_fstream() {
			if (file.is_open()) {
				std::cout << "Final size to be written: " << map_header.size << std::endl; //test
				write_file(&map_header, 0, sizeof(bmap_file_header));
				read_file(&map_header, 0, sizeof(bmap_file_header)); //test
				file.close();
				std::cout << "Final written file size: " << map_header.size << std::endl; //test

			}
			delete_index_tree(indices, map_header.info.depth);
		}
	};
}