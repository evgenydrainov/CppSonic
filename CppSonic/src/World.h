#pragma once

#include <SDL.h>

struct World {
	float camera_x;
	float camera_y;

	int level_width;
	int level_height;
	uint8_t* level_data;
	uint8_t* level_col_indicies;
	uint16_t* level_chunk_data;
	uint8_t* collision_array;

	SDL_Texture* tex_ghz256;

	void Init();
	void Quit();
	void Update(float delta);
	void Draw(float delta);

	void load_level_data();
	void load_chunk_data();
	void load_collision_array();
	void load_collision_indicies();
};
