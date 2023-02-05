#pragma once 

//#define DEBUG
#include "ocpncy/ocpncy_streams.hpp"
#include "ocpncy/ocpncy_display.hpp"
#include "util/prjctn_display.hpp"

#ifndef OUTPUT_FILEPATH 
#	define OUTPUT_FILEPATH (std::string(""))
#endif
#ifndef RESOURCES_FILEPATH
#	define RESOURCES_FILEPATH (std::string(""))
#endif

namespace oc_tests {
	ocpncy::otile<3> smileytile, frownytile;

	ocpncy::otile<4> cattile = ocpncy::otile<4>();
	gmtry2i::vector2i cattileorigin(-70, 30);
	ocpncy::otile<4> dogtile = ocpncy::otile<4>();
	gmtry2i::vector2i dogtileorigin(-25, 5);
	bool forgy[300] = { 0 };
	unsigned int forgydims[2] = { 20, 15 };
	gmtry2i::vector2i forgyorigin = gmtry2i::vector2i(-69, 9); // (5, 5)

	int render_faces() {
		smileytile.minis[0] =
			(0b00000000ULL << 48) +
			(0b00100100ULL << 40) +
			(0b00100100ULL << 32) +
			(0b00000000ULL << 24) +
			(0b01000010ULL << 16) +
			(0b00111100ULL << 8) +
			(0b00000000ULL << 0);
		frownytile.minis[0] =
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
		cattile = ocpncy::otile<4>();
		int circlex = 4;
		int circley = 9;
		int circler = 2;
		for (int x = 0; x < 16; x++) for (int y = 0; y < 16; y++)
			if (((x - circlex) * (x - circlex) + (y - circley) * (y - circley) - circler * circler) >> 1 == 0)
				ocpncy::set_occ(x, y, cattile, true);
		for (int x = circlex + circler; x < 15; x++) ocpncy::set_occ(x, circley, cattile, true);
		for (int leg = 0; leg < 4; leg++) {
			int x = (((10 - circlex + circler) * leg) / 4) + circlex + circler;
			for (int y = circley; y > 0; y--) ocpncy::set_occ(x, y, cattile, true);
		}
		ocpncy::set_occ(3, 12, cattile, true);
		ocpncy::set_occ(5, 12, cattile, true);
		//PrintTile(cattile);
		return 0;
	}

	int render_dog() {
		dogtile = ocpncy::otile<4>();
		int circlex = 4;
		int circley = 9;
		int circler = 2;
		for (int x = 0; x < 16; x++) for (int y = 0; y < 16; y++)
			if (((x - circlex) * (x - circlex) + (y - circley) * (y - circley) - circler * circler) >> 1 == 0)
				ocpncy::set_occ(x, y, dogtile, true);
		for (int x = circlex + circler; x < 15; x++) ocpncy::set_occ(x, circley, dogtile, true);
		for (int leg = 0; leg < 4; leg++) {
			int x = (((10 - circlex + circler) * leg) / 4) + circlex + circler;
			for (int y = circley; y > 0; y--) ocpncy::set_occ(x, y, dogtile, true);
		}
		ocpncy::set_occ(3, 12, dogtile, true); ocpncy::set_occ(3, 13, dogtile, true);
		ocpncy::set_occ(5, 12, dogtile, true); ocpncy::set_occ(5, 13, dogtile, true);
		for (int x = circlex - circler; x >= 0; x--) {
			ocpncy::set_occ(x, circley, dogtile, true);
			ocpncy::set_occ(x, circley - 1, dogtile, true);
		}
		std::cout << dogtile;
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
		ascii_dsp::ascii_image img = maps2::make_tile_image(4, gmtry2i::vector2i(42, 35), 
		                                                    gmtry2i::aligned_box2i(forgyorigin, 1 << (4 + 2)), -1);
		std::cout << (img << ascii_dsp::decorated_rect(iterator.get_bounds(), 0, 0, 'x') << &iterator);
		//

		/* Prints just the matrix (no tiles)
		maps2::ascii_image img({ {0, 0}, gmtry2i::vector2i(forgydims[0], forgydims[1])});
		for (int x = 0; x < forgydims[0]; x++) for (int y = 0; y < forgydims[1]; y++)
			if (forgy[x + y * forgydims[0]]) img(x, y) = '@';
		std::cout << img;
		*/

		return 0;
	}

	// writes the smileytile to the mymap file at (14, 14), reads it back, and prints it
	// writes the frowneytile to the mymap file at (5, 4), reads it back, and prints it
	int occupancy_test0() { // PASSED!
		std::cout << "OCCUPANCY TEST 0" << std::endl;
		render_faces();
		maps2::map_fstream<3, ocpncy::otile<3>> mapstream(OUTPUT_FILEPATH + "mymap", gmtry2i::vector2i(4, 5));
		const ocpncy::otile<3>* mytile = 0;

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

	typedef maps2::map_fstream<4, ocpncy::otile<4>> bmap_fstream4;
	typedef maps2::map_buffer<4, ocpncy::otile<4>> bmap_buffer4;
	typedef maps2::spatial_item<4, void> bmap_item4;
	typedef maps2::lim_tile_istream<ocpncy::otile<4>> btile_lim_stream4;
	typedef maps2::tile_istream<ocpncy::otile<4>> btile_stream4;

	// Writes cattile to the mymap4 file at (-70, 30), reads the tile back, and prints it
	int occupancy_test3() { // PASSED!
		std::cout << "OCCUPANCY TEST 3" << std::endl;
		render_cat();

		bmap_fstream4 mapstream(OUTPUT_FILEPATH + "mymap4", gmtry2i::vector2i(42, 35));
		const ocpncy::otile<4>* mytile = 0;

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
		bmap_fstream4 mapstream(OUTPUT_FILEPATH + "mymap4", gmtry2i::vector2i());
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

		bmap_fstream4 mapstream(OUTPUT_FILEPATH + "mymap4", gmtry2i::vector2i());

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

		bmap_fstream4 mapstream(OUTPUT_FILEPATH + "mymap4", gmtry2i::vector2i());
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

	unsigned char* get_map_image(std::uint32_t* width, std::uint32_t* height) {
		std::fstream file;
		file.open(RESOURCES_FILEPATH + "map.b");
		if (!file.is_open()) return 0;
		file.read(reinterpret_cast<char*>(width), 4);
		file.read(reinterpret_cast<char*>(height), 4);
		unsigned int area = (*width) * (*height);
		unsigned char* pixels = new unsigned char[area];
		file.read(reinterpret_cast<char*>(pixels), area);
		file.close();
		return pixels;
	}

	std::string get_map_file_name(unsigned int log2_w) {
		std::string width_str = std::to_string(1 << log2_w);
		return OUTPUT_FILEPATH + "map" + width_str + "x" + width_str;
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
		maps2::map_fstream<log2_w, ocpncy::otile<log2_w>> map_file(get_map_file_name(log2_w), any_tile_origin);
		map_file.write(&tiles_in);

		delete[] pixels;
	}

	template <unsigned int log2_w>
	void print_map_file_item(const gmtry2i::vector2i& item_to_print_origin, unsigned int item_to_print_depth) {
		maps2::map_fstream<log2_w, ocpncy::otile<log2_w>> map_file(get_map_file_name(log2_w), gmtry2i::vector2i());
		maps2::lim_tile_istream<ocpncy::otile<log2_w>>* tiles_out = 
			map_file.read(gmtry2i::aligned_box2i(item_to_print_origin, 1 << (log2_w + item_to_print_depth)));
		std::cout << "Bounds of region to print: " << gmtry2i::to_string(tiles_out->get_bounds()) << std::endl;
		std::cout << tiles_out;

		delete tiles_out;
	}

	template <unsigned int log2_w>
	void print_map_file() {
		maps2::map_fstream<log2_w, ocpncy::otile<log2_w>> map_file(get_map_file_name(log2_w), {});
		maps2::tile_istream<ocpncy::otile<log2_w>>* tiles_out = map_file.read();
		std::cout << tiles_out;
		delete tiles_out;
	}

	template <unsigned int log2_w>
	void print_map_file_tiles() {
		maps2::map_fstream<log2_w, ocpncy::otile<log2_w>> map_file(get_map_file_name(log2_w), {});
		maps2::tile_istream<ocpncy::otile<log2_w>>* tiles_out = map_file.read();
		int num_tiles_read = 0;
		const ocpncy::otile<log2_w>* tile;
		while (tile = tiles_out->next()) {
			std::cout << *tile;
			std::cout << "Tile origin: " << gmtry2i::to_string(tiles_out->last_origin()) << std::endl;
			num_tiles_read++;
		}
		std::cout << "Total number of tiles read: " << num_tiles_read << std::endl;

		delete tiles_out;
	}

	// occupancy tile neighborhood with which lines may collide
	template <unsigned int log2_w>
	class collidable_oc_nbrhd : public prjctn::collidable_object {
		maps2::tile_nbrhd<log2_w, ocpncy::gradient_otile<log2_w>> nbrhd;
		gmtry2i::aligned_box2i boxes[3][3];
	public:
		void set_tile(maps2::nbrng_tile<ocpncy::gradient_otile<log2_w>>* tile, const gmtry2i::vector2i& origin) {
			nbrhd = maps2::tile_nbrhd<log2_w, ocpncy::gradient_otile<log2_w>>(origin, tile);
			for (int nbrhd_x = 0; nbrhd_x < 3; nbrhd_x++) for (int nbrhd_y = 0; nbrhd_y < 3; nbrhd_y++) {
				if (nbrhd(nbrhd_x, nbrhd_y)) boxes[nbrhd_y][nbrhd_x] = 
					gmtry2i::aligned_box2i(nbrhd.origin + (gmtry2i::vector2i(nbrhd_x, nbrhd_y) << log2_w), 1 << log2_w);
			}
		}
		collidable_oc_nbrhd(maps2::nbrng_tile<ocpncy::gradient_otile<log2_w>>* tile, const gmtry2i::vector2i& origin) {
			set_tile(tile, origin);
		}
		float get_distance(const gmtry3::ray3& r, float max_distance) const {
			gmtry2::vector2 direction = gmtry2::normalize(r.d);
			gmtry2::line_segment2 l(r.p, r.p + direction * max_distance);
			const float step_size = 0.125;

			float min_dst = max_distance;
			//for (int nbrhd_x = 0; nbrhd_x < 1; nbrhd_x++) for (int nbrhd_y = 1; nbrhd_y < 2; nbrhd_y++) {
			for (int nbrhd_x = 0; nbrhd_x < 3; nbrhd_x++) for (int nbrhd_y = 0; nbrhd_y < 3; nbrhd_y++) {
				const ocpncy::gradient_otile<log2_w>* intrsctd_tile = nbrhd(nbrhd_x, nbrhd_y);
				if (intrsctd_tile == 0) continue;
				bool no_intersection = false;
				gmtry2::line_segment2 intrsctd_line =
					gmtry2i::intersection(l, boxes[nbrhd_y][nbrhd_x], &no_intersection);
				if (!no_intersection) {
					if (!gmtry2i::contains(boxes[nbrhd_y][nbrhd_x], intrsctd_line))
						std::cout << "Bad line intersection!" << std::endl;
					gmtry2::vector2 nbr_origin = gmtry2i::to_vector2(boxes[nbrhd_y][nbrhd_x].min);
					// Convert to tile-space
					intrsctd_line = intrsctd_line - nbr_origin;
					// Make sure intrsctd_line points are ordered in ascending distance
					if (gmtry2::dot(intrsctd_line.a, direction) > gmtry2::dot(intrsctd_line.b, direction)) {
						gmtry2::vector2 old_a = intrsctd_line.a;
						intrsctd_line.a = intrsctd_line.b;
						intrsctd_line.b = old_a;
					}
					gmtry2i::line_stepper2i stepper(intrsctd_line, step_size);
					bool collided = false;
					for (int t = 0; t < stepper.waypoints; t++) {
						if (ocpncy::get_occ(stepper.p.x, stepper.p.y, *intrsctd_tile)) {
							collided = true;
							break;
						}
						stepper.step();
					}
					float dst_to_occupancy = gmtry2::dot(gmtry2i::to_point2(stepper.p) + nbr_origin - l.a, 
					                                     direction);
					
					if (!gmtry2i::contains(gmtry2i::aligned_box2i({}, 1 << log2_w), stepper.p) && collided)
						std::cout << "Bad stepper intersection" << std::endl;

					if ((stepper.p.y + boxes[nbrhd_y][nbrhd_x].min.y >= 25) && collided)
						std::cout << "Bad stepper intersection 2" << std::endl;
					if (collided) min_dst = std::min(min_dst, dst_to_occupancy);
				}
			}
			return min_dst;
		}
	};

	// Produces an explorable virtual world with the cattile and dogtile
	int occupancy_test8() { // PASSED
		render_cat();
		render_dog();
		std::cout << cattile << std::endl;
		prjctn::ray_marcher marcher;
		//prjctn::sphere a({ { 0, 10, 0 }, 3 }), b({ { 5, 20, 5 }, 9 });
		//marcher.add_object(&a); marcher.add_object(&b);
		maps2::nbrng_tile<ocpncy::gradient_otile<4>> nbrng_cattile(cattile);
		maps2::nbrng_tile<ocpncy::gradient_otile<4>> nbrng_dogtile(dogtile);
		nbrng_cattile.nbrs[3] = &nbrng_dogtile;
		collidable_oc_nbrhd<4> nbrhd(&nbrng_cattile, {-10, 10});
		marcher.add_object(&nbrhd);
		prjctn::cam_info config(PI / 2, 48, 32, gmtry3::transform3(gmtry3::make_rotation(2, 11 * PI / 12), 
		                                                           {-20.5606F, 32.0225F, -1.9503F}));
		prjctn::observed_point_drawer2 drawer(48, 32);
		prjctn::world_explorer explora(config, &marcher, &drawer);
		explora.add_listener(&drawer);
		ascii_dsp::ascii_console console(&explora, std::cout);
		console.execute_commands(std::cin);
		return 0;
	}

	void prep_tests_0to7() {
		std::string filename = OUTPUT_FILEPATH + "mymap";
		std::cout << "Prepping test 0... " << (!remove(filename.c_str()) ? "Success!" : "Failed") << std::endl;
		filename += "4";
		std::cout << "Prepping tests 3-7... " << (!remove(filename.c_str()) ? "Success!" : "Failed") << std::endl;
	}

	template <unsigned int log2_w>
	void prep_real_map_tests() {
		std::string map_name = get_map_file_name(log2_w);
		std::cout << "Prepping real map tests... " << 
			(!remove(map_name.c_str()) ? "Success!" : "Failed") << std::endl;
	}

	int run_all_tests() {
		prep_tests_0to7();
		prep_real_map_tests<6>();

		oc_tests::occupancy_test0();
		oc_tests::occupancy_test3();
		//oc_tests::occupancy_test4();
		oc_tests::occupancy_test5();
		oc_tests::occupancy_test7();

		oc_tests::generate_map_file<6>(gmtry2i::vector2i(), gmtry2i::vector2i());
		//oc_tests::print_map_file_item<6>({ 0, 0 }, 3);
		oc_tests::print_map_file<6>();

		return 0;
	}
}