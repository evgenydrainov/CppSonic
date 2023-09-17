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

enum struct PlayerState {
	GROUND,
	AIR
};

enum struct PlayerMode {
	FLOOR,
	RIGHT_WALL,
	CEILING,
	LEFT_WALL
};

struct Player {
	float x;
	float y;
	float xspeed;
	float yspeed;
	float ground_speed;
	float ground_angle;
	PlayerState state;
	PlayerMode mode;
};

struct World {
	Player player;
	bool player_debug;

	float camera_x;
	float camera_y;

	int level_width;
	int level_height;
	Tile* tiles;
	int tile_count;
	TileHeight* heights;
	int height_count;
	float* angles;
	int angle_count;

	SDL_Texture* tex_ghz16;
	SDL_Texture* height_texture;

	void Init();
	void Quit();
	void Update(float delta);
	void Draw(float delta);

	void load_sonic1_genesis_level();
	bool player_check_collision(float x, float y);
	int sensor_check(float x, float y, Tile* out_tile, bool* out_found_tile);
	void player_get_ground_sensors_positions(float* sensor_a_x, float* sensor_a_y, float* sensor_b_x, float* sensor_b_y);
	Tile get_tile(int tile_x, int tile_y);
	Tile get_tile_world(float x, float y);
	uint8_t* get_height(int tile_index);
	float get_angle(int tile_index);
};
