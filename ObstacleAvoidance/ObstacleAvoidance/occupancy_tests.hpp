#pragma once 

#include <iostream>
#include "occupancy.hpp"

const ocpncy::btile<3> smileytile = { { {
		(0b00000000ULL << 48) +
		(0b00100100ULL << 40) +
		(0b00100100ULL << 32) +
		(0b00000000ULL << 24) +
		(0b01000010ULL << 16) +
		(0b00111100ULL << 8)  +
		(0b00000000ULL << 0)
} } };

const ocpncy::btile<3> frownytile = { { {
		(0b00000000ULL << 48) +
		(0b00100100ULL << 40) +
		(0b00100100ULL << 32) +
		(0b00000000ULL << 24) +
		(0b00111100ULL << 16) +
		(0b01000010ULL << 8) +
		(0b00000000ULL << 0)
} } };

ocpncy::btile<4> cattile = ocpncy::btile<4>();
gmtry2i::vector2i cattileorigin(-70, 30);
ocpncy::btile<4> dogtile = ocpncy::btile<4>();
gmtry2i::vector2i dogtileorigin(-25, 5);
bool forgy[300] = { 0 };
unsigned int forgydims[2] = { 20, 15 };
gmtry2i::vector2i forgyorigin = gmtry2i::vector2i(-69, 9); // (5, 5)

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
	int ovalx = forgydims[0]/2 - 1;
	int ovaly = 1 + forgydims[1]/2;
	int ovalrx = 5;
	int ovalry = 3;
	for (int x = 0; x < forgydims[0]; x++) for (int y = 0; y < forgydims[1]; y++)
		if (( (x - ovalx) * (x - ovalx) * ovalry * ovalry + (y - ovaly) * (y - ovaly) * ovalrx * ovalrx
			   - ovalrx * ovalrx * ovalry * ovalry ) >> 7 == 0)
			forgy[x + y * forgydims[0]] = true;
	for (int leg = 0; leg < 4; leg++) {
		int x = ovalx - ovalrx/2 + (leg * ovalrx) / 2;
		for (int y = ovaly - ovalry; y > ovaly - ovalry * 2; y--) forgy[x + y * forgydims[0]] = true;
	}
	forgy[7 + (ovaly + 1) * forgydims[0]] = true;
	forgy[6 + (ovaly - 1) * forgydims[0]] = true;
	forgy[7 + (ovaly - 1) * forgydims[0]] = true;
	forgy[8 + (ovaly - 1) * forgydims[0]] = true;

	ocpncy::mat_tile_stream<4, bool> iterator(forgy, forgydims[0], forgydims[1], forgyorigin, gmtry2i::vector2i(42, 35));
	ocpncy::bimage img(4, 2, tmaps2::align_down<4>(forgyorigin, gmtry2i::vector2i(42, 35)));
	WriteImageTiles(img, &iterator);
	// Prints X at min point (inclusive)
	img(iterator.get_bounds().min) = 'X';
	// Prints X at max point (exclusive)
	img(iterator.get_bounds().max) = 'X';
	PrintImage(img);

	return 0;
}

// writes the smileytile to the mymap file at (14, 14), reads it back, and prints it
// writes the frowneytile to the mymap file at (5, 4), reads it back, and prints it
int occupancy_test0() { // PASSED!
	std::cout << "OCCUPANCY TEST 0" << std::endl;
	try {
		tmaps2::map_fstream<3, ocpncy::btile<3>> mapstream("mymap", gmtry2i::vector2i(4, 5));
		ocpncy::btile<3> mytile;

		// Running the program with enable_write = true allows it to write the smiley and frowny tiles
		// You can run it with enable_write = false and get the same final result if it the tile has already been written
		bool enable_write = true;
		if (enable_write) {
			std::cout << "Tile to Write: " << std::endl;
			ocpncy::PrintTile(smileytile);
			std::cout << "Writing Success: " << mapstream.write(gmtry2i::vector2i(14, 14), &smileytile) << std::endl;

			std::cout << "Reading Success: " << mapstream.read(gmtry2i::vector2i(14, 14), &mytile) << std::endl;
			std::cout << "Tile Retrieved from File: " << std::endl;
			PrintTile(mytile);

			std::cout << "Tile to Write: " << std::endl;
			ocpncy::PrintTile(frownytile);
			std::cout << "Writing Success: " << mapstream.write(gmtry2i::vector2i(5, 4), &frownytile) << std::endl;
		}

		std::cout << "Reading Success: " << mapstream.read(gmtry2i::vector2i(5, 4), &mytile) << std::endl;
		std::cout << "Tile Retrieved from File: " << std::endl;
		PrintTile(mytile);

		gmtry2i::aligned_box2i mapbox = mapstream.get_bounds();
		std::cout << "Final Map Bounds: " << mapbox.min.x << ", " << mapbox.min.y << "; " << 
											 mapbox.max.x << ", " << mapbox.max.y << std::endl;
	}
	catch (int i) {
		std::cout << i << std::endl;
	}

	return 0;
}

typedef tmaps2::map_fstream<4, ocpncy::btile<4>> bmap_fstream4;
typedef tmaps2::map<ocpncy::btile<4>> bmap4;
typedef tmaps2::map_item<ocpncy::btile<4>> bmap_item4;
typedef tmaps2::tile_stream<ocpncy::btile<4>> btile_stream4;
typedef tmaps2::map_tstream<4, ocpncy::btile<4>> bmap_tstream4;

// Writes cattile to the mymap4 file at (-70, 30), reads the tile back, and prints it
int occupancy_test3() { // PASSED!
	std::cout << "OCCUPANCY TEST 3" << std::endl;
	render_cat();

	bmap_fstream4 mapstream("mymap4", gmtry2i::vector2i(42, 35));
	ocpncy::btile<4> mytile;

	std::cout << "Tile to Write: " << std::endl;
	ocpncy::PrintTile(cattile);
	std::cout << "Writing Success: " << mapstream.write(gmtry2i::vector2i(-70, 30), &cattile) << std::endl;

	std::cout << "Reading Success: " << mapstream.read(gmtry2i::vector2i(-70, 30), &mytile) << std::endl;
	std::cout << "Tile Retrieved from File: " << std::endl;
	PrintTile(mytile);

	gmtry2i::aligned_box2i mapbox = mapstream.get_bounds();
	std::cout << "Final Map Bounds: " << mapbox.min.x << ", " << mapbox.min.y << "; " << 
										 mapbox.max.x << ", " << mapbox.max.y << std::endl;

	return 0;
}

// RUN AFTER TEST 3
// Reads the mymap4 file and prints part of its contents
int occupancy_test4() { // PASSED!
	std::cout << "OCCUPANCY TEST 4" << std::endl;
	bmap_fstream4 mapstream("mymap4", gmtry2i::vector2i());
	bmap4 map(mapstream.get_bounds().min);

	btile_stream4* file_tile_stream;
	std::cout << "Reading Success: " << mapstream.read(&file_tile_stream) << std::endl;
	file_tile_stream->set_bounds(gmtry2i::aligned_box2i(gmtry2i::vector2i(-20, 0), 1 << (4 + 2)));
	ocpncy::PrintItem(tmaps2::set_map_tiles<4>(&map, file_tile_stream, tmaps2::TILE_ADD_MODE));
	delete file_tile_stream;

	return 0;
}

// RUN AFTER TEST 3
// Writes dogtile to a map at (-25, 5), writes the map to the mymap4 file, reads the file, and prints part of its contents
int occupancy_test5() { // PASSED!
	std::cout << "OCCUPANCY TEST 5" << std::endl;
	render_dog();

	bmap_fstream4 mapstream("mymap4", gmtry2i::vector2i());

	bmap4 map(mapstream.get_bounds().min);
	tmaps2::fit_map<4>(&map, dogtileorigin);
	bmap_item4 map_dogtile = tmaps2::alloc_map_item<4>(bmap_item4(&map), dogtileorigin, 0);
	*static_cast<ocpncy::btile<4>*>(map_dogtile.ptr) = dogtile;
	bmap_item4 item(&map);
	bmap_tstream4 iterator(item);
	dogtileorigin = tmaps2::align_down<4>(dogtileorigin, item.info.origin);

	std::cout << "New Dogtile Origin: " << dogtileorigin.x << ", " << dogtileorigin.y << std::endl;
	iterator.set_bounds(gmtry2i::aligned_box2i(dogtileorigin + gmtry2i::vector2i(15, 15), dogtileorigin + gmtry2i::vector2i(1 << 4, 1 << 4)));
	gmtry2i::aligned_box2i newbounds = iterator.get_bounds();
	std::cout << "New Map Iterator Bounds: " << newbounds.min.x << ", " << newbounds.min.y << "; " <<
												newbounds.max.x << ", " << newbounds.max.y << std::endl;

	std::cout << "Tile to Write: " << std::endl;
	PrintTile(dogtile);
	std::cout << "Writing Success: " << mapstream.write(&iterator) << std::endl;

	tmaps2::delete_map_tree<ocpncy::btile<4>>(map.root, map.info.depth);
	map.root = new tmaps2::map_tree();
	map.info.depth = 1;

	btile_stream4* file_tile_stream;
	std::cout << "Reading Success: " << mapstream.read(&file_tile_stream) << std::endl;
	file_tile_stream->set_bounds(gmtry2i::aligned_box2i(dogtileorigin, 1 << (4 + 2)));
	bmap_tstream4 reading_iterator(tmaps2::set_map_tiles<4>(&map, file_tile_stream, tmaps2::TILE_ADD_MODE));
	reading_iterator.set_bounds(gmtry2i::aligned_box2i(dogtileorigin, 1 << (4 + 2)));
	ocpncy::PrintTiles(&reading_iterator, 2);
	delete file_tile_stream;

	return 0;
}

// RUN AFTER TEST 3 OR 3 AND 5
// Uses a tile stream to read tiles out from the boolean occupancy array (a matrix stored in row-major order)
// Writes the tiles to the file as they are read out from the array
// Uses a stream to read back all of the file's tiles straight to the image before it's printed
int occupancy_test7() {
	std::cout << "OCCUPANCY TEST 7" << std::endl;

	bmap_fstream4 mapstream("mymap4", gmtry2i::vector2i());
	mapstream.write_mode() = tmaps2::TILE_ADD_MODE;

	std::cout << "Tiles to Write: " << std::endl;
	render_forgy();

	ocpncy::mat_tile_stream<4, bool> iterator(forgy, forgydims[0], forgydims[1], forgyorigin, mapstream.get_bounds().min);
	std::cout << "Writing Success: " << mapstream.write(&iterator) << std::endl;

	btile_stream4* file_tile_stream;
	mapstream.read(&file_tile_stream);
	file_tile_stream->set_bounds(gmtry2i::aligned_box2i(forgyorigin, 1 << (6)));
	
	ocpncy::PrintTiles(file_tile_stream, 2);
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

int geometry_test0() {
	float theta = 3.1415 / 6;
	gmtry3::rotor3 r = gmtry3::unit_make_rotor(gmtry3::vector3(0, 1, 0), theta) *
					   gmtry3::unit_make_rotor(gmtry3::vector3(0, 0, 1), 2 * theta);
	gmtry3::matrix3 M = gmtry3::make_rotation(1, theta) * gmtry3::make_rotation(2, 2 * theta);
	std::cout << gmtry3::to_string(r * gmtry3::vector3(1.5, 2, 1)) << std::endl;
	std::cout << gmtry3::to_string(M * gmtry3::vector3(1.5, 2, 1)) << std::endl;

	return 0;
}

// So far only prints out the bottom left tile of the map
// If you try to write the tile to the file, the map_fstream allocates a tree and then writes the tile there
//			THAT'S NOT SUPPOSED TO HAPPEN!!
//			Also sometimes it doesn't record the file's new length in the map file header
template <unsigned int log2_w>
void generate_map_file(const gmtry2i::vector2i& map_origin, const gmtry2i::vector2i& any_tile_origin) {
	std::fstream file;
	file.open("map.b");
	if (!file.is_open()) return;
	std::uint32_t width, height;
	file.read(reinterpret_cast<char*>(&width), 4);
	file.read(reinterpret_cast<char*>(&height), 4);
	std::uint32_t* pixels = new std::uint32_t[width * height];
	file.read(reinterpret_cast<char*>(pixels), width * height * 4);
	file.close();
	ocpncy::mat_tile_stream<log2_w, std::uint32_t> tiles_in(pixels, width, height, map_origin, any_tile_origin);
	tiles_in.set_bounds(gmtry2i::aligned_box2i(map_origin, 
		gmtry2i::vector2i(tiles_in.get_bounds().max.x, tiles_in.get_bounds().max.y)));
	tmaps2::map_fstream<log2_w, ocpncy::btile<log2_w>> map_file("map" + std::to_string(1 << log2_w), any_tile_origin);
	map_file.write(&tiles_in);
	/*
	*/
	tiles_in.reset();
	//ocpncy::PrintTiles(&tiles_in, 0);

	delete[] pixels;
}