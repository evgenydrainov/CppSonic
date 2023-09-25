#pragma once

#include "Objects.h"
#include "TileSet.h"
#include "TileMap.h"

enum {
	INPUT_RIGHT = 1,
	INPUT_UP    = 1 << 1,
	INPUT_LEFT  = 1 << 2,
	INPUT_DOWN  = 1 << 3,
	INPUT_JUMP  = 1 << 4
};

class World {
public:
	using Tile = TileMap::Tile;

	World();

	void Init();
	void Quit();
	void Update(float delta);
	void Draw(float delta);

	Player player = {};

	float camera_x = 0.0f;
	float camera_y = 0.0f;

	uint32_t input = 0;
	uint32_t input_press = 0;
	uint32_t input_release = 0;

	TileSet tileset;
	TileMap tilemap;

	int target_w = 0;
	int target_h = 0;

	bool debug = false;

private:
	void player_get_ground_sensors_positions(float* sensor_a_x, float* sensor_a_y, float* sensor_b_x, float* sensor_b_y);
	int  ground_sensor_check      (float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y);
	int  ground_sensor_check_floor(float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y);
	int  ground_sensor_check_right(float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y);
	int  ground_sensor_check_ceil (float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y);
	int  ground_sensor_check_left (float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y);
	void player_ground_sensor_collision();

	friend int editor_main(int, char**);
};
