#pragma once

#include "tilemaps.hpp"

namespace maps2 {
	class ascii_image {
	private:
		unsigned int log2_pw;
		char** lines;
		// Chars to be returned when an attempt is made to read/write an out-of-bounds pixel
		char out_of_bounds_wpixel, const out_of_bounds_rpixel = '\0';
		std::string caption;
		// Bounds of region displayed
		gmtry2i::aligned_box2i bounds;
		// Pixel-wise dimensions
		unsigned int width, height;
		// Width of a pixel in units of characters
		const static unsigned int horizontal_multiplier = 2;
		// Finds the pixel width necessary to fit the image on screen
		static unsigned int find_log2_pixel_width(unsigned int bounds_width, unsigned int max_line_length) {
			unsigned int log2_pw = 0;
			while ((bounds_width >> log2_pw) * horizontal_multiplier > max_line_length) log2_pw++;
			return log2_pw;
		}
	public:
		// References the character being displayed for the given pixel position on the image
		char& operator ()(unsigned int x, unsigned int y) {
			// x >>= log2_pw; y >>= log2_pw;
			if (x < width && y < height) return lines[y][horizontal_multiplier * x];
			else return out_of_bounds_wpixel;
		}
		// References the character being displayed for the given point in space
		char& operator ()(const gmtry2i::vector2i& p) {
			if (gmtry2i::contains(bounds, p)) {
				gmtry2i::vector2i pixel_p = (p - bounds.min) >> log2_pw;
				return lines[pixel_p.y][horizontal_multiplier * (pixel_p.x)];
			}
			else return out_of_bounds_wpixel;
		}
		void write(const char* text, unsigned int x, unsigned int y) {
			x *= horizontal_multiplier;
			unsigned int c = 0;
			if (y < height) {
				char* line = lines[y];
				while (line[x] && text[c]) line[x++] = text[c++];
			}
		}
		void write(const char* text, const gmtry2i::vector2i& p) {
			if (gmtry2i::contains(bounds, p)) {
				gmtry2i::vector2i pixel_p = (p - bounds.min) >> log2_pw;
				write(text, pixel_p.x, pixel_p.y);
			}
		}
		const char* get_line(int i) const {
			if (i < height) return lines[height - (i + 1)];
			else return &out_of_bounds_rpixel;
		}
		int get_num_lines() const {
			return height;
		}
		void set_caption(const std::string& new_caption) {
			caption = new_caption;
		}
		const char* get_caption() const {
			return caption.c_str();
		}
		// Returns bounds of the displayed region in space; might not be tile-aligned
		gmtry2i::aligned_box2i get_bounds() const {
			return bounds;
		}
		ascii_image(const gmtry2i::aligned_box2i& viewport, unsigned int max_line_length) {
			bounds = viewport;
			log2_pw = find_log2_pixel_width(bounds.max.x - bounds.min.x, max_line_length);
			width = (bounds.max.x - bounds.min.x) >> log2_pw;
			height = (bounds.max.y - bounds.min.y) >> log2_pw;
			lines = new char* [height];
			for (unsigned int y = 0; y < height; y++) {
				lines[y] = new char[horizontal_multiplier * width + 1];
				for (unsigned int x = 0; x < width; x++)
					for (unsigned int x = 0; x < width; x++) {
						lines[y][x * horizontal_multiplier] = '.';
						for (unsigned int fillerx = 1; fillerx < horizontal_multiplier; fillerx++)
							lines[y][x * horizontal_multiplier + fillerx] = ' ';
					}
				lines[y][horizontal_multiplier * width] = '\0';
			}
			(*this)(gmtry2i::vector2i()) = 'O';
			caption = "Image Bounds: " + gmtry2i::to_string(bounds);
		}
		ascii_image(unsigned int log2_tile_width, const gmtry2i::vector2i& any_tile_origin,
			const gmtry2i::aligned_box2i& viewport, unsigned int max_line_length) {
			bounds = maps2::align_out(viewport, any_tile_origin, log2_tile_width);
			log2_pw = find_log2_pixel_width(bounds.max.x - bounds.min.x, max_line_length);
			// twp is tile width in units of pixels
			unsigned int log2_twp = log2_tile_width - log2_pw;
			unsigned int tile_pixel_width = 1 << (log2_tile_width - log2_pw);

			width = (bounds.max.x - bounds.min.x) >> log2_pw;
			height = (bounds.max.y - bounds.min.y) >> log2_pw;
			lines = new char* [height];
			unsigned int sub_tile_mask = (1 << log2_twp) - 1;
			for (unsigned int y = 0; y < height; y++) {
				lines[y] = new char[horizontal_multiplier * width + 1];
				for (unsigned int x = 0; x < width; x++) {
					lines[y][x * horizontal_multiplier] = (x & sub_tile_mask) ? '.' : '|';
					for (unsigned int fillerx = 1; fillerx < horizontal_multiplier; fillerx++)
						lines[y][x * horizontal_multiplier + fillerx] = ' ';
				}
				if ((y & sub_tile_mask) == 0)
					for (unsigned int x = 0; x < width * horizontal_multiplier; x++)
						lines[y][x] = '-';
				lines[y][horizontal_multiplier * width] = '\0';
			}
			(*this)(gmtry2i::vector2i()) = 'O';
			caption = "Image Bounds: " + gmtry2i::to_string(bounds);
		}
		ascii_image(const ascii_image& img) {
			log2_pw = img.log2_pw;
			width = img.width;
			height = img.height;
			lines = new char* [height];
			for (int y = 0; y < height; y++) {
				lines[y] = new char[horizontal_multiplier * width + 1];
				for (unsigned int x = 0; x < width * horizontal_multiplier + 1; x++)
					lines[y][x] = img.lines[y][x];
			}
			caption = img.caption;
			bounds = img.bounds;
		}
		~ascii_image() {
			for (int y = height - 1; y >= 0; y--) delete[] lines[y];
			delete[] lines;
		}
	};

	struct named_point {
		gmtry2i::vector2i point;
		char name;
		named_point(const gmtry2i::vector2i& new_point, char new_name) {
			point = new_point;
			name = new_name;
		}
	};

	struct named_rect {
		gmtry2i::aligned_box2i box;
		char name;
		char fill;
		named_rect(const gmtry2i::aligned_box2i& new_box, char new_name) {
			box = new_box;
			name = new_name;
			fill = '@';
		}
		named_rect(const gmtry2i::aligned_box2i& new_box, char new_name, char new_fill) {
			box = new_box;
			name = new_name;
			fill = new_fill;
		}
	};

	template <typename tile_type>
	struct located_tile {
		const tile_type* tile;
		gmtry2i::vector2i origin;
		located_tile(const tile_type* new_tile, const gmtry2i::vector2i& new_origin) {
			tile = new_tile;
			origin = new_origin;
		}
	};

	std::ostream& operator << (std::ostream& os, const ascii_image& img) {
		for (int y = 0; y < img.get_num_lines(); y++) os << img.get_line(y) << std::endl;
		os << img.get_caption() << std::endl;
		return os;
	}

	ascii_image& operator << (ascii_image& img, const named_point& p) {
		img(p.point) = p.name;
		return img;
	}

	ascii_image& operator << (ascii_image& img, const gmtry2i::vector2i& p) {
		return img << named_point(p, '@');
	}

	ascii_image& operator << (ascii_image& img, const named_rect& box) {
		char fill = box.fill;
		if (fill)
			for (int x = box.box.min.x; x < box.box.max.x; x++)
				for (int y = box.box.min.y; y < box.box.max.y; y++)
					img({ x, y }) = box.fill;
		char name = box.name;
		if (name) {
			if ('A' <= name && name <= 'Z') name = (name + 'a') - 'A';
			img(box.box.min) = name;
			img(box.box.max) = (name + 'A') - 'a';
		}
		return img;
	}

	ascii_image& operator << (ascii_image& img, const gmtry2i::aligned_box2i& box) {
		return img << named_rect(box, 0, '@');
	}

	ascii_image& operator << (ascii_image& img, const gmtry2i::line_segment2i& l) {
		gmtry2i::vector2i disp = l.b - l.a;
		if (disp.x && disp.y) {
			char angle = ((disp.x > 0) == (disp.y > 0)) ? '/' : '\\';
			int length = std::sqrt(gmtry2i::dot(disp, disp));
			float inv_length = 1.0F / static_cast<float>(length);
			float norm_x = disp.x * inv_length;
			float norm_y = disp.y * inv_length;
			for (int i = 0; i < length; i++) img(l.a + gmtry2i::vector2i(i * norm_x, i * norm_y)) = angle;
		}
		else if (disp.x) {
			int norm_x = (disp.x > 0) ? 1 : -1;
			int length = std::abs(disp.x);
			for (int i = 0; i < length; i++) img(l.a + gmtry2i::vector2i(i * norm_x, 0)) = '-';
		}
		else if (disp.y) {
			int norm_y = (disp.y > 0) ? 1 : -1;
			int length = std::abs(disp.y);
			for (int i = 0; i < length; i++) img(l.a + gmtry2i::vector2i(0, i * norm_y)) = '|';
		}
		return img;
	}

	template <typename T>
	concept drawable_tile = requires (const T* a, const gmtry2i::vector2i& p, maps2::ascii_image& img) {
		img = img << located_tile(a, p);
	};

	template <drawable_tile tile>
	ascii_image& operator << (ascii_image& img, tile_istream<tile>* tiles) {
		const tile* next_tile;
		while (next_tile = tiles->next())
			if (gmtry2i::contains(img.get_bounds(), tiles->last_origin()))
				img << located_tile<tile>(next_tile, tiles->last_origin());
		return img;
	}

	template <drawable_tile tile>
	ascii_image& operator << (ascii_image& img, map_istream<tile>* map) {
		tile_istream<tile>* tiles = map->read();
		img << tiles;
		delete tiles;
		return img;
	}
}