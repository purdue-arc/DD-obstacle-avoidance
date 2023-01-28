#pragma once

#include "geometry.hpp"

#include <string>
#include <type_traits>
#include <iostream>

namespace ascii_dsp {
	const unsigned int DEFAULT_MAX_LINE_LENGTH = 1536;

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
		void overwrite(ascii_image& img) {
			if (!gmtry2i::intersects(bounds, img.bounds)) return;
			gmtry2i::aligned_box2i intersection = gmtry2i::intersection(bounds, img.bounds);
			for (int x = intersection.min.x; x < intersection.max.x; x++)
				for (int y = intersection.min.y; y < intersection.max.y; y++) {
					char img_char = img({ x, y });
					if (img_char != '.') (*this)({ x, y }) = img_char;
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
		unsigned int get_width() {
			return width;
		}
		unsigned int get_height() {
			return height;
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
		ascii_image(const gmtry2i::aligned_box2i& viewport) : ascii_image(viewport, DEFAULT_MAX_LINE_LENGTH) {}
		// Produces ASCII image with square-tile pattern pre-printed on it
		ascii_image& operator =(const ascii_image& img) {
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
			return *this;
		}
		ascii_image(const ascii_image& img) {
			*this = img;
		}
		~ascii_image() {
			for (int y = height - 1; y >= 0; y--) delete[] lines[y];
			delete[] lines;
		}
	};

	std::ostream& operator << (std::ostream& os, const ascii_image& img) {
		for (int y = 0; y < img.get_num_lines(); y++) os << img.get_line(y) << std::endl;
		os << img.get_caption() << std::endl;
		return os;
	}

	struct decorated_point {
		gmtry2i::vector2i point;
		char decoration;
		decorated_point(const gmtry2i::vector2i& new_point, char new_decoration) {
			point = new_point;
			decoration = new_decoration;
		}
	};

	ascii_image& operator << (ascii_image& img, const decorated_point& p) {
		img(p.point) = p.decoration;
		return img;
	}

	ascii_image& operator << (ascii_image& img, const gmtry2i::vector2i& p) {
		return img << decorated_point(p, '@');
	}

	decorated_point named_point(const gmtry2i::vector2i& point, char name) {
		return decorated_point(point, name);
	}

	decorated_point gradient_point(const gmtry2i::vector2i& point, const gmtry2i::vector2i& gradient) {
		char slope;
		if (!gradient.x && !gradient.y)
			slope = '.';
		else if (2.414 * std::abs(gradient.x) < std::abs(gradient.y))
			slope = '|';
		else if (2.414 * std::abs(gradient.y) < std::abs(gradient.x))
			slope = '-';
		else slope = ((gradient.x > 0) == (gradient.y > 0)) ? '/' : '\\';
		return decorated_point(point, slope);
	}

	const char char_shade_set1[70] = ".'`^\",:;Il!i><~+_-?][}{1)(|\\/tfjrxnuvczXYUJCLQ0OZmwqpdbkhao*#MW&8%B@$";
	const char char_shade_set2[10] = ".:-=+*#%@";

	decorated_point shaded_point(const gmtry2i::vector2i& point, float normalized_shade) {
		char shade = char_shade_set1[static_cast<int>(std::min(1.0F, std::max(0.0F, normalized_shade)) * 68)];
		return decorated_point(point, shade);
	}

	decorated_point simple_shaded_point(const gmtry2i::vector2i& point, float normalized_shade) {
		char shade = ".:-=+*#%@"[static_cast<int>(std::min(1.0F, std::max(0.0F, normalized_shade)) * 9)];
		return decorated_point(point, shade);
	}

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
		gmtry2i::line_stepper2i stepper(l);
		gmtry2i::vector2i pt_buf[3] = { l.b, l.b, l.a };
		for (int t = 0; t <= stepper.length; t++) {
			pt_buf[2] = stepper.p;
			// Direction displayed is based on average of the two most recent displacements
			//		This is to make the line slopes both continuous and smooth
			img << gradient_point(pt_buf[2], pt_buf[2] - pt_buf[0]);
			pt_buf[0] = pt_buf[1];
			pt_buf[1] = pt_buf[2];
			stepper.step();
		}
		img << gradient_point(l.b, l.b - pt_buf[0]);
		return img;
	}

	template <typename T>
	using spatial_field2 = T(*)(const gmtry2i::vector2i& p);

	using vector_field2 = spatial_field2<gmtry2i::vector2i>;

	template <typename T>
	concept number_type = std::is_arithmetic<T>::value;

	template <number_type T>
	ascii_image& operator << (ascii_image& img, spatial_field2<T> field) {
		const gmtry2i::vector2i shift1(1, 1);
		gmtry2i::aligned_box2i img_bounds = img.get_bounds();
		gmtry2i::aligned_box2i field_bounds(img_bounds.min - shift1, img_bounds.max + shift1);
		gmtry2i::vector2i field_dims(field_bounds.max - field_bounds.min);
		T** field_values = new T* [field_dims.y];
		for (int y = field_bounds.min.y; y < field_bounds.max.y; y++) {
			field_values[y - field_bounds.min.y] = new T[field_dims.x];
			for (int x = field_bounds.min.x; x < field_bounds.max.x; x++) {
				field_values[y - field_bounds.min.y][x - field_bounds.min.x] = field({ x, y });
			}
		}
		for (int y = img_bounds.min.y; y < img_bounds.max.y; y++) {
			for (int x = img_bounds.min.x; x < img_bounds.max.x; x++) {
				gmtry2i::vector2i gradient = gmtry2i::vector2i();
				for (int ny = -1; ny < 2; ny++) for (int nx = -1; nx < 2; nx++)
					if (ny != 0 || nx != 0)
						gradient += gmtry2i::vector2i(nx, ny) * 
							field_values[y + ny - field_bounds.min.y][x + nx - field_bounds.min.x];
				img << gradient_point({ x, y }, gradient);
			}
		}
		for (int y = 0; y < field_dims.y; y++) {
			delete[] field_values[y];
			field_values[y] = 0;
		}
		delete[] field_values;
		return img;
	}
}