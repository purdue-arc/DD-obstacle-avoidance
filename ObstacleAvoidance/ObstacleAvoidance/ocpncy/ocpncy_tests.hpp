#pragma once 

#define DEBUG
#include "occupancy.hpp"
#include "ocpncy_streams.hpp"
#include "maps2/maps2_streams.hpp"

#include "projection.hpp"

#include <iostream>

namespace oc_tests {
	ocpncy::btile<3> smileytile, frownytile;

	ocpncy::btile<4> cattile = ocpncy::btile<4>();
	gmtry2i::vector2i cattileorigin(-70, 30);
	ocpncy::btile<4> dogtile = ocpncy::btile<4>();
	gmtry2i::vector2i dogtileorigin(-25, 5);
	bool forgy[300] = { 0 };
	unsigned int forgydims[2] = { 20, 15 };
	gmtry2i::vector2i forgyorigin = gmtry2i::vector2i(-69, 9); // (5, 5)
	const float PI = 3.14159265F;

	int render_faces() {
		smileytile.minis[0][0] =
			(0b00000000ULL << 48) +
			(0b00100100ULL << 40) +
			(0b00100100ULL << 32) +
			(0b00000000ULL << 24) +
			(0b01000010ULL << 16) +
			(0b00111100ULL << 8) +
			(0b00000000ULL << 0);
		frownytile.minis[0][0] =
			(0b00000000ULL << 48) +
			(0b00100100ULL << 40) +
			(0b00100100ULL << 32) +
			(0b00000000ULL << 24) +
			(0b00111100ULL << 16) +
			(0b01000010ULL << 8) +
			(0b00000000ULL << 0);
		return 0;
	}

	int render_cat() {
		cattile = ocpncy::btile<4>();
		int circlex = 4;
		int circley = 9;
		int circler = 2;
		for (int x = 0; x < 16; x++) for (int y = 0; y < 16; y++)
			if (((x - circlex) * (x - circlex) + (y - circley) * (y - circley) - circler * circler) >> 1 == 0)
				ocpncy::set_bit(x, y, cattile, true);
		for (int x = circlex + circler; x < 15; x++) ocpncy::set_bit(x, circley, cattile, true);
		for (int leg = 0; leg < 4; leg++) {
			int x = (((10 - circlex + circler) * leg) / 4) + circlex + circler;
			for (int y = circley; y > 0; y--) ocpncy::set_bit(x, y, cattile, true);
		}
		ocpncy::set_bit(3, 12, cattile, true);
		ocpncy::set_bit(5, 12, cattile, true);
		//PrintTile(cattile);
		return 0;
	}

	int render_dog() {
		dogtile = ocpncy::btile<4>();
		int circlex = 4;
		int circley = 9;
		int circler = 2;
		for (int x = 0; x < 16; x++) for (int y = 0; y < 16; y++)
			if (((x - circlex) * (x - circlex) + (y - circley) * (y - circley) - circler * circler) >> 1 == 0)
				ocpncy::set_bit(x, y, dogtile, true);
		for (int x = circlex + circler; x < 15; x++) ocpncy::set_bit(x, circley, dogtile, true);
		for (int leg = 0; leg < 4; leg++) {
			int x = (((10 - circlex + circler) * leg) / 4) + circlex + circler;
			for (int y = circley; y > 0; y--) ocpncy::set_bit(x, y, dogtile, true);
		}
		ocpncy::set_bit(3, 12, dogtile, true); ocpncy::set_bit(3, 13, dogtile, true);
		ocpncy::set_bit(5, 12, dogtile, true); ocpncy::set_bit(5, 13, dogtile, true);
		for (int x = circlex - circler; x >= 0; x--) {
			ocpncy::set_bit(x, circley, dogtile, true);
			ocpncy::set_bit(x, circley - 1, dogtile, true);
		}
		//PrintTile(dogtile);
		return 0;
	}

	int render_forgy() {
		int ovalx = forgydims[0] / 2 - 1;
		int ovaly = 1 + forgydims[1] / 2;
		int ovalrx = 5;
		int ovalry = 3;
		for (int x = 0; x < forgydims[0]; x++) for (int y = 0; y < forgydims[1]; y++)
			if (((x - ovalx) * (x - ovalx) * ovalry * ovalry + (y - ovaly) * (y - ovaly) * ovalrx * ovalrx
				- ovalrx * ovalrx * ovalry * ovalry) >> 7 == 0)
				forgy[x + y * forgydims[0]] = true;
		for (int leg = 0; leg < 4; leg++) {
			int x = ovalx - ovalrx / 2 + (leg * ovalrx) / 2;
			for (int y = ovaly - ovalry; y > ovaly - ovalry * 2; y--) forgy[x + y * forgydims[0]] = true;
		}
		forgy[7 + (ovaly + 1) * forgydims[0]] = true;
		forgy[6 + (ovaly - 1) * forgydims[0]] = true;
		forgy[7 + (ovaly - 1) * forgydims[0]] = true;
		forgy[8 + (ovaly - 1) * forgydims[0]] = true;

		ocpncy::mat_tile_stream<4, bool> iterator(forgy, forgydims[0], forgydims[1], forgyorigin, gmtry2i::vector2i(42, 35));
		maps2::ascii_image img(4, gmtry2i::vector2i(42, 35), gmtry2i::aligned_box2i(forgyorigin, 1 << (4 + 2)), -1);
		std::cout << (img << maps2::named_rect(iterator.get_bounds(), 'x', 0) << &iterator);

		return 0;
	}

	// writes the smileytile to the mymap file at (14, 14), reads it back, and prints it
	// writes the frowneytile to the mymap file at (5, 4), reads it back, and prints it
	int occupancy_test0() { // PASSED!
		std::cout << "OCCUPANCY TEST 0" << std::endl;
		render_faces();
		maps2::map_fstream<3, ocpncy::btile<3>> mapstream("mymap", gmtry2i::vector2i(4, 5));
		const ocpncy::btile<3>* mytile = 0;

		// Running the program with enable_write = true allows it to write the smiley and frowny tiles
		// You can run it with enable_write = false and get the same final result if it the tile has already been written
		bool enable_write = true;
		if (enable_write) {
			std::cout << "Tile to Write: " << std::endl << smileytile;
			mapstream.write(gmtry2i::vector2i(14, 14), &smileytile);

			mytile = mapstream.read(gmtry2i::vector2i(14, 14));
			std::cout << "Reading Success: " << static_cast<bool>(mytile) << std::endl;
			if (mytile) std::cout << "Tile Retrieved from File: " << std::endl << *mytile;

			std::cout << "Tile to Write: " << std::endl << frownytile;
			mapstream.write(gmtry2i::vector2i(5, 4), &frownytile);
		}

		mytile = mapstream.read(gmtry2i::vector2i(5, 4));
		std::cout << "Reading Success: " << static_cast<bool>(mytile) << std::endl;
		if (mytile) std::cout << "Tile Retrieved from File: " << std::endl << *mytile;

		std::cout << "Final Map Bounds: " << gmtry2i::to_string(mapstream.get_bounds()) << std::endl;
		std::cout << &mapstream;

		return 0;
	}

	typedef maps2::map_fstream<4, ocpncy::btile<4>> bmap_fstream4;
	typedef maps2::map_buffer<4, ocpncy::btile<4>> bmap_buffer4;
	typedef maps2::spacial_item<4, void> bmap_item4;
	typedef maps2::lim_tile_istream<ocpncy::btile<4>> btile_lim_stream4;
	typedef maps2::tile_istream<ocpncy::btile<4>> btile_stream4;

	// Writes cattile to the mymap4 file at (-70, 30), reads the tile back, and prints it
	int occupancy_test3() { // PASSED!
		std::cout << "OCCUPANCY TEST 3" << std::endl;
		render_cat();

		bmap_fstream4 mapstream("mymap4", gmtry2i::vector2i(42, 35));
		const ocpncy::btile<4>* mytile = 0;

		std::cout << "Tile to Write: " << std::endl << cattile;
		mapstream.write(gmtry2i::vector2i(-70, 30), &cattile);

		mytile = mapstream.read(gmtry2i::vector2i(-70, 30));
		std::cout << "Reading Success: " << static_cast<bool>(mytile) << std::endl;
		if (mytile) std::cout << "Tile Retrieved from File: " << std::endl << *mytile;

		std::cout << "Final Map Bounds: " << gmtry2i::to_string(mapstream.get_bounds()) << std::endl;
		std::cout << &mapstream;

		return 0;
	}

	// RUN AFTER TEST 3
	// Reads the mymap4 file and prints part of its contents
	int occupancy_test4() { // PASSED!
		std::cout << "OCCUPANCY TEST 4" << std::endl;
		bmap_fstream4 mapstream("mymap4", gmtry2i::vector2i());
		bmap_buffer4 map(mapstream.get_bounds().min);

		btile_stream4* file_tile_stream = mapstream.read();
		//file_tile_stream->set_bounds(gmtry2i::aligned_box2i(gmtry2i::vector2i(-20, 0), 1 << (4 + 2)));
		map.set_wmode(maps2::TILE_ADD_MODE);
		map.write(file_tile_stream);
		std::cout << &map;
		delete file_tile_stream;
		return 0;
	}

	// RUN AFTER TEST 3
	// Writes dogtile to a map at (-25, 5), writes the map to the mymap4 file, reads the file, and prints part of its contents
	int occupancy_test5() { // PASSED!
		std::cout << "OCCUPANCY TEST 5" << std::endl;
		render_dog();

		bmap_fstream4 mapstream("mymap4", gmtry2i::vector2i());

		bmap_buffer4 map(mapstream.get_bounds().min);
		map.write(dogtileorigin, &dogtile);
		dogtileorigin = maps2::align_down(dogtileorigin, map.get_bounds().min, 4);
		std::cout << "New Dogtile Origin: " << gmtry2i::to_string(dogtileorigin) << std::endl;
		btile_lim_stream4* iterator = map.read(gmtry2i::aligned_box2i(dogtileorigin + gmtry2i::vector2i(15, 15),
			dogtileorigin + gmtry2i::vector2i(1 << 4, 1 << 4)));
		std::cout << "New Map Iterator Bounds: " << gmtry2i::to_string(iterator->get_bounds()) << std::endl;

		std::cout << "Tile to Write: " << std::endl << dogtile;
		mapstream.write(iterator);
		std::cout << "Dogtile written to file successfully!" << std::endl;
		delete iterator;

		bmap_buffer4 map2(mapstream.get_bounds().min);
		btile_lim_stream4* file_tile_stream = mapstream.read(gmtry2i::aligned_box2i(dogtileorigin, 1 << (4 + 2)));
		map2.set_wmode(maps2::TILE_ADD_MODE);
		map2.write(file_tile_stream);
		std::cout << "Dogtile written to file successfully!" << std::endl;
		std::cout << &map2;
		delete file_tile_stream;

		std::cout << &mapstream;

		return 0;
	}

	// RUN AFTER TEST 3 OR 3 AND 5
	// Uses a tile stream to read tiles out from the boolean occupancy array (a matrix stored in row-major order)
	// Writes the tiles to the file as they are read out from the array
	// Uses a stream to read back all of the file's tiles straight to the image before it's printed
	int occupancy_test7() {
		std::cout << "OCCUPANCY TEST 7" << std::endl;

		bmap_fstream4 mapstream("mymap4", gmtry2i::vector2i());
		mapstream.set_wmode(maps2::TILE_ADD_MODE);

		std::cout << "Tiles to Write: " << std::endl;
		render_forgy();

		ocpncy::mat_tile_stream<4, bool> iterator(forgy, forgydims[0], forgydims[1], forgyorigin, mapstream.get_bounds().min);
		mapstream.write(&iterator);

		btile_lim_stream4* file_tile_stream = mapstream.read(gmtry2i::aligned_box2i(forgyorigin, 1 << (6)));
		std::cout << file_tile_stream;
		delete file_tile_stream;

		return 0;
	}

	template <int i>
	class A {
	protected:
		const char* hidden_message = "Class a's hidden message";
	public:
		A() = default;
		virtual const char* message() { return "Class a's message"; }
		void print_message() { std::cout << message() << std::endl; }
	};

	template <int i>
	class B : public A<i> {
	public:
		B() = default;
		virtual const char* message() { return A<i>::hidden_message; }
	};

	int template_inheritance_test() {
		A<4> a = A<4>();
		a.print_message();
		B<4> b = B<4>();
		b.print_message();

		A<4>* undercoverB = new B<4>();
		undercoverB->print_message();
		delete undercoverB;

		return 0;
	}

	class C {
	public:
		virtual void print_message() = 0;
	};

	class D : public C {
	protected:
		const char* get_message() {
			return "hello";
		}
	public:
		void print_message() {
			std::cout << get_message() << std::endl;
		}
	};

	class E : public D {
	protected:
		const char* get_message() {
			return "goodbye";
		}
	};

	int inheritance_test2()  {
		D d = D();
		E e = E();

		C* c_ptr = &d;
		c_ptr->print_message();
		c_ptr = &e;
		c_ptr->print_message();

		e.print_message();

		return 0;
	}

	int geometry_test0() {
		float theta = 3.1415 / 6;
		gmtry3::rotor3 r = gmtry3::unit_make_rotor(gmtry3::vector3(0, 1, 0), theta) *
			gmtry3::unit_make_rotor(gmtry3::vector3(0, 0, 1), 2 * theta);
		gmtry3::matrix3 M = gmtry3::make_rotation(1, theta) * gmtry3::make_rotation(2, 2 * theta);
		std::cout << gmtry3::to_string(r * gmtry3::vector3(1.5, 2, 1)) << std::endl;
		std::cout << gmtry3::to_string(M * gmtry3::vector3(1.5, 2, 1)) << std::endl;

		return 0;
	}

	int geometry_test1() {
		maps2::ascii_image img(gmtry2i::aligned_box2i(-gmtry2i::vector2i(1, 1) << 5, 1 << 6), -1);
		gmtry2i::aligned_box2i box1({ 5, 10 }, 5);
		gmtry2i::aligned_box2i box2({ 15, 0 }, 5);
		img << maps2::named_rect(box1, 'a')
			<< maps2::named_rect(box2, 'b')
			<< maps2::named_rect(box1 - box2, 'c');
		std::cout << img;
		return 0;
	}

	int geometry_test2() {
		maps2::ascii_image img(gmtry2i::aligned_box2i(gmtry2i::vector2i(), 100), DEFAULT_MAX_LINE_LENGTH);
		img.write("hello", { 99, 50 });
		gmtry2i::vector2i p1(20, 15), p2(105, 40), q1(76, -50), q2(5, 90);
		gmtry2i::line_segment2i l1(p1, p2), l2(q1, q2);
		img << l1 << l2;
		img << p1 << p2 << q1 << q2;
		img(gmtry2i::intersection(l1, l2)) = 'I';
		std::cout << img;
		return 0;
	}

	class image_projector2 : public maps2::point_ostream {
		maps2::ascii_image& img;
		gmtry2i::vector2i cam_origin;
	public:
		image_projector2(maps2::ascii_image& new_img, const gmtry2i::vector2i& new_cam_origin) : img(new_img) {
			cam_origin = new_cam_origin;
		}
		void write(const gmtry2i::vector2i& p) {
			//std::cout << "point detected" << std::endl;
			//std::cout << gmtry2i::to_string(p) << std::endl;
			img << gmtry2i::line_segment2i(cam_origin, p);
			img(p) = '@';
		}
	};

	int projection_test0() { // PASSED
		prjctn::cam_info config(PI / 2, 4, 1, {});
		float* depths = new float[config.width * config.height] {10, 11, 12, 13};
		gmtry3::vector3 cam_origin(40, 6, 0);
		gmtry2i::vector2i cam_origin2(cam_origin.x, cam_origin.y);
		gmtry3::matrix3 cam_orientation = gmtry3::make_rotation(2, PI / 4);		// Yaw left 45deg
		//								* gmtry3::make_rotation(0, -PI / 3);	// Pitch down 60deg
		maps2::ascii_image img(gmtry2i::aligned_box2i(gmtry2i::vector2i(), 64), DEFAULT_MAX_LINE_LENGTH);
		image_projector2 projector(img, cam_origin2);
		// Draw camera's local axes
		//img << gmtry2i::line_segment2i(cam_origin2, cam_origin2 + 
		//	   gmtry2i::vector2i(cam_orientation(0).x * 8, cam_orientation(0).y * 8))
		//	<< gmtry2i::line_segment2i(cam_origin2, cam_origin2 + 
		//	   gmtry2i::vector2i(cam_orientation(1).x * 8, cam_orientation(1).y * 8));
		config.pose = gmtry3::transform3(cam_orientation, cam_origin);
		prjctn::deproject(depths, config, {}, &projector);
		img(cam_origin2) = 'C';
		std::cout << img;

		delete[] depths;
		return 0;
	}

	int projection_test1() { // PASSED
		prjctn::cam_info config(PI / 3, 3, 2, {});
		float* depths = new float[config.width * config.height] {6, 7, 8, 9, 10, 11};
		gmtry3::vector3 cam_origin(40, 6, 0);
		gmtry2i::vector2i cam_origin2(cam_origin.x, cam_origin.y);
		gmtry3::matrix3 cam_orientation = gmtry3::make_rotation(2, PI / 4)	// Yaw left
		//								* gmtry3::make_rotation(0, PI / 2)	// Pitch up
										* gmtry3::make_rotation(1, -PI / 6);// Roll down to the left
		maps2::ascii_image img(gmtry2i::aligned_box2i(gmtry2i::vector2i(), 64), DEFAULT_MAX_LINE_LENGTH);
		image_projector2 projector(img, cam_origin2);
		config.pose = gmtry3::transform3(cam_orientation, cam_origin);
		prjctn::deproject(depths, config, {}, &projector);
		img(cam_origin2) = 'C';
		std::cout << img;

		delete[] depths;
		return 0;
	}

	unsigned char* get_map_image(std::uint32_t* width, std::uint32_t* height) {
		std::fstream file;
		file.open("map.b");
		if (!file.is_open()) return 0;
		file.read(reinterpret_cast<char*>(width), 4);
		file.read(reinterpret_cast<char*>(height), 4);
		unsigned int area = (*width) * (*height);
		unsigned char* pixels = new unsigned char[area];
		file.read(reinterpret_cast<char*>(pixels), area);
		file.close();
		return pixels;
	}

	template <unsigned int log2_w>
	void generate_map_file(const gmtry2i::vector2i& map_origin, const gmtry2i::vector2i& any_tile_origin) {
		std::uint32_t og_width, og_height;
		unsigned char* og_pixels = get_map_image(&og_width, &og_height);
		if (og_pixels == 0) return;
		unsigned int width = og_width >> 1;
		unsigned int height = og_height >> 1;
		unsigned char* pixels = ocpncy::halve_matrix<unsigned char>(og_pixels, og_width, og_height);
		delete[] og_pixels;

		ocpncy::mat_tile_stream<log2_w, unsigned char> tiles_in(pixels, width, height, map_origin, any_tile_origin);
		std::cout << "Original map file bounds: " << gmtry2i::to_string(tiles_in.get_bounds()) << std::endl;
		std::string width_str = std::to_string(1 << log2_w);
		maps2::map_fstream<log2_w, ocpncy::btile<log2_w>> map_file("map" + width_str + "x" + width_str, any_tile_origin);
		map_file.write(&tiles_in);

		delete[] pixels;
	}

	template <unsigned int log2_w>
	void print_map_file_item(const gmtry2i::vector2i& item_to_print_origin, unsigned int item_to_print_depth) {
		std::string width_str = std::to_string(1 << log2_w);
		maps2::map_fstream<log2_w, ocpncy::btile<log2_w>> map_file("map" + width_str + "x" + width_str, gmtry2i::vector2i());
		maps2::lim_tile_istream<ocpncy::btile<log2_w>>* tiles_out = 
			map_file.read(gmtry2i::aligned_box2i(item_to_print_origin, 1 << (log2_w + item_to_print_depth)));
		std::cout << "Bounds of region to print: " << gmtry2i::to_string(tiles_out->get_bounds()) << std::endl;
		std::cout << tiles_out;

		delete tiles_out;
	}

	template <unsigned int log2_w>
	void print_map_file_tiles() {
		std::string width_str = std::to_string(1 << log2_w);
		maps2::map_fstream<log2_w, ocpncy::btile<log2_w>> map_file("map" + width_str + "x" + width_str, gmtry2i::vector2i());
		maps2::tile_istream<ocpncy::btile<log2_w>>* tiles_out = map_file.read();
		int num_tiles_read = 0;
		const ocpncy::btile<log2_w>* tile;
		while (tile = tiles_out->next()) {
			//ocpncy::PrintTile(*tile);
			std::cout << "Tile origin: " << gmtry2i::to_string(tiles_out->last_origin()) << std::endl;
			num_tiles_read++;
		}
		std::cout << "Total number of tiles read: " << num_tiles_read << std::endl;

		delete tiles_out;
	}
}