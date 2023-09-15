#pragma once

#include <SDL.h>

struct Tile {
	int index;
	bool hflip;
	bool vflip;
};

struct TileHeight {
	uint8_t height[16];
};

struct World {
	float camera_x;
	float camera_y;

	int level_width;
	int level_height;
	Tile* tiles;
	TileHeight* heights;

	SDL_Texture* tex_ghz16;

	void Init();
	void Quit();
	void Update(float delta);
	void Draw(float delta);

	void load_level_data();
	void load_chunk_data();
	void load_collision_array();
	void load_collision_indicies();
	void load_sonic1_genesis_level();
};
