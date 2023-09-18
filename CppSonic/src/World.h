#pragma once

#include <SDL.h>

#include <vector>

struct Tile {
	int index;
	bool hflip;
	bool vflip;
	bool top_solid;
	bool left_right_bottom_solid;
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

class World {
public:
	World();

	void Init();
	void Quit();
	void Update(float delta);
	void Draw(float delta);

	Tile get_tile(int tile_x, int tile_y);
	Tile get_tile_world(float x, float y);
	uint8_t* get_height(int tile_index);
	uint8_t* get_width(int tile_index);
	float get_angle(int tile_index);

	Player player = {};

	float camera_x = 0.0f;
	float camera_y = 0.0f;

	int level_width = 0;
	int level_height = 0;
	std::vector<Tile> tiles;
	std::vector<TileHeight> heights;
	std::vector<TileHeight> widths;
	std::vector<float> angles;

	SDL_Texture* tex_ghz16 = nullptr;
	SDL_Texture* height_texture = nullptr;
	SDL_Texture* width_texture = nullptr;

	bool debug = false;

private:
	void load_sonic1_level();

	void player_get_ground_sensors_positions(float* sensor_a_x, float* sensor_a_y, float* sensor_b_x, float* sensor_b_y);
	int sensor_check_ground      (float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y);
	int sensor_check_ground_floor(float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y);
	int sensor_check_ground_right(float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y);
	int sensor_check_ground_ceil (float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y);
	int sensor_check_ground_left (float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y);
};
