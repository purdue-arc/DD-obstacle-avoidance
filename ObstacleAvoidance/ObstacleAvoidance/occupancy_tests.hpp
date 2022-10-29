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
ocpncy::btile<4> dogtile = ocpncy::btile<4>();
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

	ocpncy::occ_mat_iterator<4, bool> iterator(forgy, forgydims[0], forgydims[1], forgyorigin, gmtry2i::vector2i(42, 35));
	ocpncy::bimage img(4, 2, gmtry2i::vector2i(42 - 16 * 7, 35 - 16 * 4));
	WriteImage(img, &iterator);
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
		ocpncy::bmap_fstream<3> mapstream("mymap", gmtry2i::vector2i(4, 5));
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

// Reads the mymap file's bmap_file_header without going through the bmap_fstream
int occupancy_test1() { // PASSED!
	std::cout << "OCCUPANCY TEST 1" << std::endl;
	FILE* file;
	fopen_s(&file, "mymap", "r");
	if (file == 0) return -1;

	ocpncy::bmap_info<3> info;
	fseek(file, 0, SEEK_SET);
	fread(&info, 1, sizeof(info), file);
	std::cout << "Recorded log2_tile_w: " << info.log2_tile_w << std::endl;
	std::cout << "Recorded depth: " << info.depth << std::endl;
	std::cout << "Recorded origin: " << info.origin.x << ", " << info.origin.y << std::endl;

	unsigned long root;
	fread(&root, 1, sizeof(root), file);
	std::cout << "Recorded root: " << root << std::endl;

	unsigned long size;
	fread(&size, 1, sizeof(size), file);
	std::cout << "Recorded file size: " << size << std::endl;

	fpos_t final_position;
	fgetpos(file, &final_position);
	std::cout << "Position of tile0: " << final_position << std::endl;

	fclose(file);
	return 0;
}

// RUN AFTER TEST 0
// Reads the mymap file and prints part of its contents
int occupancy_test2() { // PASSED!
	std::cout << "OCCUPANCY TEST 2" << std::endl;
	ocpncy::bmap_fstream<3> mapstream("mymap", gmtry2i::vector2i(4, 5));
	ocpncy::bmap<3> map;

	std::cout << "Reading Success: " << mapstream.read(gmtry2i::vector2i(0, 0), 2, &map) << std::endl;
	ocpncy::bmap_item<3> item(map.root, map.info.origin, map.info.depth);
	std::cout << "Retrieved Item Depth: " << item.depth << std::endl;
	std::cout << "Retrieved Item Origin: " << item.origin.x << ", " << item.origin.y << std::endl;
	ocpncy::PrintItem(item);

	return 0;
}

// Writes cattile to the mymap4 file at (-70, 30), reads the tile back, and prints it
int occupancy_test3() { // PASSED!
	std::cout << "OCCUPANCY TEST 3" << std::endl;
	render_cat();

	ocpncy::bmap_fstream<4> mapstream("mymap4", gmtry2i::vector2i(42, 35));
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
	ocpncy::bmap_fstream<4> mapstream("mymap4", gmtry2i::vector2i());
	ocpncy::bmap<4> map;

	std::cout << "Reading Success: " << mapstream.read(gmtry2i::vector2i(-20, 0), 2, &map) << std::endl;
	ocpncy::bmap_item<4> item(map.root, map.info.origin, map.info.depth);
	std::cout << "Retrieved Item Depth: " << item.depth << std::endl;
	std::cout << "Retrieved Item Origin: " << item.origin.x << ", " << item.origin.y << std::endl;
	ocpncy::PrintItem(item);

	return 0;
}

// RUN AFTER TEST 3
// Writes dogtile to a map at (-25, 5), writes the map to the mymap4 file, reads the file, and prints part of its contents
int occupancy_test5() { // PASSED!
	std::cout << "OCCUPANCY TEST 5" << std::endl;
	render_dog();

	ocpncy::bmap_fstream<4> mapstream("mymap4", gmtry2i::vector2i());

	gmtry2i::vector2i dogtilecoords(-25, 5);
	ocpncy::bmap<4> map(gmtry2i::vector2i(26, 19));
	ocpncy::fit_bmap(&map, dogtilecoords);
	ocpncy::bmap_item<4> map_dogtile = ocpncy::alloc_bmap_item(&map, dogtilecoords, 0);
	*static_cast<ocpncy::btile<4>*>(map_dogtile.ptr) = dogtile;
	ocpncy::bmap_item<4> item(map.root, map.info.origin, map.info.depth);
	ocpncy::bmap_item_iterator<4> iterator(item);

	std::cout << "Tile to Write: " << std::endl;
	PrintTile(dogtile);
	std::cout << "Writing Success: " << mapstream.write(&iterator) << std::endl;

	std::cout << "Reading Success: " << mapstream.read(gmtry2i::vector2i(-20, 0), 2, &map) << std::endl;
	item = ocpncy::bmap_item<4>(map.root, map.info.origin, map.info.depth);
	std::cout << "Retrieved Item Depth: " << item.depth << std::endl;
	std::cout << "Retrieved Item Origin: " << item.origin.x << ", " << item.origin.y << std::endl;
	ocpncy::PrintItem(item);

	return 0;
}

// RUN AFTER TEST 3 OR 3 AND 5
// Uses a tile stream to read tiles out from the boolean occupancy array (a matrix stored in row-major order)
// Writes the tiles to the file as they are read out from the array
// Reads back both the pre-existing and new parts of the map file and prints them
int occupancy_test6() { // PASSED!
	std::cout << "OCCUPANCY TEST 6" << std::endl;

	ocpncy::bmap_fstream<4> mapstream("mymap4", gmtry2i::vector2i());
	mapstream.mode() = ocpncy::BMAP_OSTREAM_ADD_MODE;

	std::cout << "Tiles to Write: " << std::endl;
	render_forgy();
	ocpncy::occ_mat_iterator<4, bool> iterator(forgy, forgydims[0], forgydims[1], forgyorigin, mapstream.get_bounds().min);

	mapstream.write(&iterator);

	ocpncy::bmap<4> map;
	std::cout << "Reading Success: " << mapstream.read(gmtry2i::vector2i(5, 5), 2, &map) << std::endl;
	ocpncy::bmap_item<4> item(map.root, map.info.origin, map.info.depth);
	ocpncy::PrintItem(item);

	std::cout << "Reading Success: " << mapstream.read(gmtry2i::vector2i(-20, 0), 2, &map) << std::endl;
	item = ocpncy::bmap_item<4>(map.root, map.info.origin, map.info.depth);
	ocpncy::PrintItem(item);

	return 0;
}