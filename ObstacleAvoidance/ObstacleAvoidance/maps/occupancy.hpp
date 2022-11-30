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
	void PrintTile(const btile<log2_w>& tile) {
		for (int y = (1 << log2_w) - 1; y >= 0; y--) {
			for (int x = 0; x < (1 << log2_w); x++) {
				std::cout << (get_bit(x, y, tile) ? "@" : ".") << " ";
			}
			std::cout << std::endl;
		}
	}

	class bimage {
	private:
		unsigned int width;
		unsigned int log2_pixelwidth;
		char** lines;
		char out_of_bounds_pixel;
		std::string caption;
		gmtry2i::vector2i origin;
		gmtry2i::aligned_box2i bounds;
	public:
		char& operator ()(gmtry2i::vector2i p) {
			if (gmtry2i::contains(bounds, p)) {
				gmtry2i::vector2i pixel_pos = (p - origin) >> log2_pixelwidth;
				return lines[pixel_pos.y][2 * (pixel_pos.x)];
			}
			else return out_of_bounds_pixel;
		}
		char& operator ()(unsigned int x, unsigned int y) {
			x >>= log2_pixelwidth; y >>= log2_pixelwidth;
			if (x < width && y < width) return lines[y][2 * x];
			else return out_of_bounds_pixel;
		}
		const char* line(int i) const {
			if (i < width) return lines[width - (i + 1)];
			else return &out_of_bounds_pixel;
		}
		int numLines() const {
			return width;
		}
		const char* getCaption() const {
			return caption.c_str();
		}
		gmtry2i::aligned_box2i getBounds() const {
			return bounds;
		}
		// w is tile width and tw is width of image in units of tiles
		bimage(unsigned int log2_w, unsigned int log2_wt, unsigned int log2_pw, gmtry2i::vector2i new_origin) {
			log2_pixelwidth = log2_pw;
			// twp is tile width in units of pixels
			unsigned int log2_twp = log2_w - log2_pw;

			width = 1 << (log2_wt + log2_twp);
			lines = new char* [width];
			int basex, basey, x, y;
			for (int y1 = 0; y1 < (1 << log2_wt); y1++) {
				basey = y1 << log2_twp;
				lines[basey] = new char[2 * width + 1];
				for (x = 0; x < width * 2; x++) lines[basey][x] = '-';
				lines[basey][2 * width] = '\0';
				for (int y2 = 1; y2 < (1 << log2_twp); y2++) {
					y = basey + y2;
					lines[y] = new char[2 * width + 1];
					for (int x1 = 0; x1 < (1 << log2_wt); x1++) {
						basex = x1 << log2_twp;
						lines[y][basex * 2] = '|'; lines[y][basex * 2 + 1] = ' ';
						for (int x2 = 1; x2 < (1 << log2_twp); x2++) {
							x = basex + x2;
							lines[y][x * 2] = '.'; lines[y][x * 2 + 1] = ' ';
						}
					}
					lines[y][2 * width] = '\0';
				}
			}
			origin = new_origin;
			bounds = gmtry2i::aligned_box2i(origin, 1 << (log2_w + log2_wt));
			(*this)(gmtry2i::vector2i()) = 'O';
			caption = "Image Bounds: " + gmtry2i::to_string(bounds);
		}
		~bimage() {
			for (int y = width - 1; y >= 0; y--) delete[] lines[y];
			delete[] lines;
		}
	};
	void PrintImage(const bimage& img) {
		for (int y = 0; y < img.numLines(); y++) std::cout << img.line(y) << std::endl;
		std::cout << img.getCaption() << std::endl;
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
			if (gmtry2i::contains(img.getBounds(), tiles->last_origin()))
				WriteImageTile(img, tiles->last_origin(), *tile);
	}
	void WriteImageBox(bimage& img, const gmtry2i::aligned_box2i& box, char box_name) {
		for (int x = box.min.x; x < box.max.x; x++) 
			for (int y = box.min.y; y < box.max.y; y++)
				img({ x, y }) = '@';
		if ('A' <= box_name && box_name <= 'Z') box_name = (box_name + 'a') - 'A';
		img(box.min) = box_name;
		img(box.max) = (box_name + 'A') - 'a';
	}
	template <unsigned int log2_w>
	void PrintItem(const tmaps2::map_item<btile<log2_w>>& item) {
		bimage img(log2_w, item.info.depth, 0, item.info.origin);
		tmaps2::map_tstream<log2_w, btile<log2_w>> iterator(item);
		WriteImageTiles(img, &iterator);
		PrintImage(img);
	}
	template <unsigned int log2_w>
	void PrintTiles(tmaps2::tile_stream<btile<log2_w>>* stream, unsigned int depth, unsigned int log2_pixelwidth) {
		bimage img(log2_w, depth, log2_pixelwidth, stream->get_bounds().min);
		WriteImageTiles(img, stream);
		PrintImage(img);
	}
	template <unsigned int log2_w>
	void PrintTiles(tmaps2::tile_stream<btile<log2_w>>* stream, unsigned int depth) {
		PrintTiles(stream, depth, 0);
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

	template <unsigned int log2_w, unsigned int num_layers>
	struct btile3 {
		btile<log2_w> layers[num_layers];
		btile3() = default;
		inline btile3& operator +=(const btile3<log2_w, num_layers>& tile) {
			for (int z = 0; z < num_layers; z++)
				layers[z] += tile.layers[z];
			return *this;
		}
		inline btile3& operator +=(const btile3<log2_w, num_layers>* tile) {
			for (int z = 0; z < num_layers; z++)
				layers[z] += tile->layers[z];
			return *this;
		}
		inline btile3& operator -=(const btile3<log2_w, num_layers>& tile) {
			for (int z = 0; z < num_layers; z++)
				layers[z] -= tile.layers[z];
			return *this;
		}
		inline btile3& operator -=(const btile3<log2_w, num_layers>* tile) {
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

	template <unsigned int log2_w>
	inline unsigned int compress_coords2(const gmtry2i::vector2i& p) {
		return p.x + (p.y << log2_w);
	}

	template <unsigned int log2_w>
	inline gmtry2i::vector2i decompress_coords2(unsigned int idx) {
		unsigned int mask = (1 << log2_w) - 1;
		return gmtry2i::vector2i(idx & mask, (idx >> log2_w) & mask);
	}

	template <typename tile>
	class local_occ_idcs_ostream {
	public:
		virtual void write(tile* source, unsigned int idx) = 0;
	};

	template <unsigned int log2_w, unsigned int num_layers>
	void project(const float* depths, float fov, unsigned int width, unsigned int height, 
		gmtry3::transform3 cam_pose, tmaps2::nbrng_tile<btile3<log2_w, num_layers>>* dst, gmtry3::vector3 dst_origin, 
		local_occ_idcs_ostream<tmaps2::nbrng_tile<btile3<log2_w, num_layers>>>* changes_ostream) {
		btile3<log2_w, num_layers> projected[9];
		for (int i = 0; i < 9; i++) projected[i] = btile3<log2_w, num_layers>();
		gmtry3::vector3 projected_origin = dst_origin - gmtry3::vector3(1 << log2_w, 1 << log2_w, 0);
		unsigned int proj_width = 3 * (1 << log2_w);
		unsigned int mask = (1 << log2_w) - 1;

		gmtry3::vector3 cam_space_point;
		gmtry3::vector3 projected_point;
		long px, py, pz;
		float xy_scale;
		gmtry3::transform3 to_projected = cam_pose.T() - projected_origin;
		int hwidth = width / 2;
		int hheight = height / 2;
		float img_scale = std::tan(fov) / MAX(hwidth, hheight);
		for (int x = 0; x < width; x++) for (int y = 0; y < height; y++) {
			cam_space_point.z = depths[x + y * width];
			xy_scale = img_scale * cam_space_point.z;
			cam_space_point.x = (x - hwidth) * xy_scale;
			cam_space_point.y = (hheight - y) * xy_scale; // negate if image is oriented upside down
			projected_point = to_projected * cam_space_point;
			px = projected_point.x; py = projected_point.y; pz = projected_point.z;
			if (0 <= px && px < proj_width && 0 <= py && py < proj_width && 0 <= pz && pz < num_layers)
				set_bit(px & mask, py & mask, pz, projected[(px >> log2_w) + 3 * (py >> log2_w)], true);
		}
		for (int i = 0; i < 9; i++) projected[i] -= (i != 4) ? dst->nbrs[i - (i > 4)] : dst;

		// To do: determine newly-occpupied blocks and stream them out through the changes_stream
	}

	// Don't look at this yet; I paused work on it to finish the one above
	template <unsigned int log2_w>
	void project(const float* depths, float fov,
		unsigned int width, unsigned int height,
		gmtry3::transform3 to_cam, tmaps2::map_item<btile<log2_w>> dst,
		local_occ_idcs_ostream<btile<log2_w>>* changes_ostream) { // ALSO PASS A QUEUE OR SOMETHING
		gmtry3::vector3 cam_space_point;
		gmtry2i::vector2i point2D;
		float xy_scale;
		gmtry3::transform3 to_world = to_cam.T();
		int hwidth = width / 2;
		int hheight = height / 2;
		float img_scale = std::tan(fov) / MAX(hwidth, hheight);
		for (int x = 0; x < width; x++) for (int y = 0; y < height; y++) {
			cam_space_point.z = depths[x + y * width];
			xy_scale = img_scale * cam_space_point.z;
			cam_space_point.x = (x - hwidth) * xy_scale;
			cam_space_point.y = (hheight - y) * xy_scale; // negate if image is oriented upside down
			point2D.x = to_world.R.n[0][0] * cam_space_point.x +
						to_world.R.n[1][0] * cam_space_point.y +
						to_world.R.n[2][0] * cam_space_point.z + to_world.t.x;
			point2D.y = to_world.R.n[0][1] * cam_space_point.x +
						to_world.R.n[1][1] * cam_space_point.y +
						to_world.R.n[2][1] * cam_space_point.z + to_world.t.y;
			// PUT 2D POINT IN MAP
		}
	}

	/*
	* TODO
	*	Make branch positions and map file header have a platform-independent size
	*/
}