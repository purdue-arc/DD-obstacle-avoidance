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

int render_cat() {
	int circlex = 4;
	int circley = 9;
	int circler = 2;
	for (int x = 0; x < 16; x++) for (int y = 0; y < 16; y++) 
		if (((x - circlex) * (x - circlex) + (y - circley) * (y - circley) - circler * circler) >> 1 == 0)
			ocpncy::set_bit(x, y, &cattile, true);
	for (int x = circlex + circler; x < 15; x++) ocpncy::set_bit(x, circley, &cattile, true);
	for (int leg = 0; leg < 4; leg++) {
		int x = (((10 - circlex + circler) * leg) / 4) + circlex + circler;
		for (int y = circley; y > 0; y--) ocpncy::set_bit(x, y, &cattile, true);
	}
	ocpncy::set_bit(3, 12, &cattile, true);
	ocpncy::set_bit(5, 12, &cattile, true);
	//PrintTile(cattile);
	return 0;
}

int occupancy_test0() { // PASSED!
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

int occupancy_test1() { // PASSED!
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
int occupancy_test2() { // PASSED!
	ocpncy::bmap_fstream<3> mapstream("mymap", gmtry2i::vector2i(4, 5));
	ocpncy::bmap<3> map;

	std::cout << "Reading Success: " << mapstream.read(gmtry2i::vector2i(0, 0), 2, &map) << std::endl;
	ocpncy::bmap_item<3> item(map.root, map.info.depth, map.info.origin);
	std::cout << "Retrieved Item Depth: " << item.depth << std::endl;
	std::cout << "Retrieved Item Origin: " << item.origin.x << ", " << item.origin.y << std::endl;
	ocpncy::PrintItem(item);

	return 0;
}

int occupancy_test3() { // PASSED!
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
int occupancy_test4() { // PASSED!
	ocpncy::bmap_fstream<4> mapstream("mymap4", gmtry2i::vector2i());
	ocpncy::bmap<4> map;

	std::cout << "Reading Success: " << mapstream.read(gmtry2i::vector2i(-20, 0), 2, &map) << std::endl;
	ocpncy::bmap_item<4> item(map.root, map.info.depth, map.info.origin);
	std::cout << "Retrieved Item Depth: " << item.depth << std::endl;
	std::cout << "Retrieved Item Origin: " << item.origin.x << ", " << item.origin.y << std::endl;
	ocpncy::PrintItem(item);

	return 0;
}