#include "World.h"

#include "Game.h"

#include <SDL_image.h>
#include <math.h>
#include <stdio.h>
#include <string.h>

#include "mathh.h"
#include "stb_sprintf.h"

#define GRAVITY 0.21875f

#define PLAYER_WIDTH_RADIUS 9.0f
#define PLAYER_HEIGHT_RADIUS 19.0f
#define PLAYER_PUSH_RADIUS 10.0f
#define PLAYER_ACC 0.046875f
#define PLAYER_DEC 0.5f
#define PLAYER_FRICTION 0.046875f
#define PLAYER_TOP_SPEED 6.0f
#define PLAYER_JUMP_FORCE 6.5f
#define PLAYER_SLOPE_FACTOR 0.125f

World::World() {
	world = this;
}

void World::load_sonic1_level() {
	std::vector<uint8_t> level_data;
	std::vector<uint16_t> level_chunk_data;
	std::vector<uint8_t> collision_array;
	std::vector<uint8_t> collision_array_rot;
	std::vector<uint8_t> level_col_indicies;
	std::vector<uint16_t> startpos;
	std::vector<uint8_t> angle_map;

	{
		size_t size;
		uint8_t* data = (uint8_t*) SDL_LoadFile("levels/ghz1.bin", &size);
		level_data.assign(data, data + size / sizeof(*data));
		SDL_free(data);
	}

	{
		size_t size;
		uint16_t* data = (uint16_t*) SDL_LoadFile("map256/GHZ.bin", &size);
		level_chunk_data.assign(data, data + size / sizeof(*data));
		SDL_free(data);
	}

	{
		size_t size;
		uint8_t* data = (uint8_t*) SDL_LoadFile("collide/Collision Array (Normal).bin", &size);
		collision_array.assign(data, data + size / sizeof(*data));
		SDL_free(data);
	}

	{
		size_t size;
		uint8_t* data = (uint8_t*) SDL_LoadFile("collide/Collision Array (Rotated).bin", &size);
		collision_array_rot.assign(data, data + size / sizeof(*data));
		SDL_free(data);
	}

	{
		size_t size;
		uint8_t* data = (uint8_t*) SDL_LoadFile("collide/GHZ.bin", &size);
		level_col_indicies.assign(data, data + size / sizeof(*data));
		SDL_free(data);
	}

	{
		size_t size;
		uint16_t* data = (uint16_t*) SDL_LoadFile("startpos/ghz1.bin", &size);
		startpos.assign(data, data + size / sizeof(*data));
		SDL_free(data);
	}

	{
		size_t size;
		uint8_t* data = (uint8_t*) SDL_LoadFile("collide/Angle Map.bin", &size);
		angle_map.assign(data, data + size / sizeof(*data));
		SDL_free(data);
	}

	int level_width_in_chunks = (int)level_data[0] + 1;
	int level_height_in_chunks = (int)level_data[1] + 1;
	level_width = level_width_in_chunks * 16;
	level_height = level_height_in_chunks * 16;

	for (size_t i = 0; i < level_chunk_data.size(); i++) {
		level_chunk_data[i] = (level_chunk_data[i] << 8) | (level_chunk_data[i] >> 8);
	}

	startpos[0] = (startpos[0] << 8) | (startpos[0] >> 8);
	startpos[1] = (startpos[1] << 8) | (startpos[1] >> 8);
	player.x = (float) startpos[0];
	player.y = (float) startpos[1];
	camera_x = player.x - (float)GAME_W / 2.0f;
	camera_y = player.y - (float)GAME_H / 2.0f;

	tiles.resize(level_width * level_height);
	heights.resize(level_col_indicies.size());
	widths.resize(level_col_indicies.size());
	angles.resize(level_col_indicies.size());

	for (int y = 0; y < level_height; y++) {
		for (int x = 0; x < level_width; x++) {
			int chunk_x = x / 16;
			int chunk_y = y / 16;

			uint8_t chunk_index = level_data[2 + chunk_x + chunk_y * level_width_in_chunks] & 0b0111'1111;
			bool loop = level_data[2 + chunk_x + chunk_y * level_width_in_chunks] & 0b1000'0000;
			if (chunk_index == 0) {
				continue;
			}

			if (loop) {
				chunk_index++;
			}

			uint16_t* tiles = &level_chunk_data[(chunk_index - 1) * 256];

			int tile_in_chunk_x = x % 16;
			int tile_in_chunk_y = y % 16;

			uint16_t tile = tiles[tile_in_chunk_x + tile_in_chunk_y * 16];

			this->tiles[x + y * level_width].index = tile & 0b0000'0011'1111'1111;
			this->tiles[x + y * level_width].hflip = tile & 0b0000'1000'0000'0000;
			this->tiles[x + y * level_width].vflip = tile & 0b0001'0000'0000'0000;
			this->tiles[x + y * level_width].top_solid = tile & 0b0010'0000'0000'0000;
			this->tiles[x + y * level_width].left_right_bottom_solid = tile & 0b0100'0000'0000'0000;
		}
	}

	for (size_t i = 0; i < heights.size(); i++) {
		uint8_t index = level_col_indicies[i];
		uint8_t* height = &collision_array[(size_t)index * 16];
		uint8_t* width = &collision_array_rot[(size_t)index * 16];
		for (size_t j = 0; j < 16; j++) {
			heights[i].height[j] = height[j];
			widths[i].height[j] = width[j];
		}

		uint8_t angle = angle_map[index];
		if (angle == 0xFF) { // flagged
			angles[i] = -1.0f;
		} else {
			angles[i] = ((float)(256 - (int)angle) / 256.0f) * 360.0f;
		}
	}

	tex_ghz16 = IMG_LoadTexture(game->renderer, "ghz16.png");

	{
		SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, 16 * 16, ((heights.size() + 15) / 16) * 16, 32, SDL_PIXELFORMAT_ARGB8888);
		SDL_FillRect(surf, nullptr, 0x00000000);
		SDL_Surface* wsurf = SDL_CreateRGBSurfaceWithFormat(0, 16 * 16, ((heights.size() + 15) / 16) * 16, 32, SDL_PIXELFORMAT_ARGB8888);
		SDL_FillRect(wsurf, nullptr, 0x00000000);

		for (size_t tile_index = 0; tile_index < heights.size(); tile_index++) {
			uint8_t* height = get_height(tile_index);
			uint8_t* width = get_width(tile_index);
			for (int i = 0; i < 16; i++) {
				int x = tile_index % 16;
				int y = tile_index / 16;

				if (height[i] != 0) {
					if (height[i] <= 0x10) {
						SDL_Rect line = {
							x * 16 + i,
							y * 16 + 16 - height[i],
							1,
							height[i]
						};
						SDL_FillRect(surf, &line, 0xffffffff);
					} else if (height[i] >= 0xF0) {
						SDL_Rect line = {
							x * 16 + i,
							y * 16,
							1,
							16 - (height[i] - 0xF0)
						};
						SDL_FillRect(surf, &line, 0xffff0000);
					}
				}

				if (width[i] != 0) {
					if (width[i] <= 0x10) {
						SDL_Rect line = {
							x * 16 + 16 - width[i],
							y * 16 + i,
							width[i],
							1
						};
						SDL_FillRect(wsurf, &line, 0xffffffff);
					} else {
						SDL_Rect line = {
							x * 16,
							y * 16 + i,
							16 - (width[i] - 0xF0),
							1
						};
						SDL_FillRect(wsurf, &line, 0xffff0000);
					}
				}
			}
		}

		height_texture = SDL_CreateTextureFromSurface(game->renderer, surf);
		width_texture = SDL_CreateTextureFromSurface(game->renderer, wsurf);

		SDL_FreeSurface(surf);
		SDL_FreeSurface(wsurf);
	}
}

void World::Init() {
	load_sonic1_level();
}

void World::Quit() {
	SDL_DestroyTexture(width_texture);
	SDL_DestroyTexture(height_texture);
	SDL_DestroyTexture(tex_ghz16);
}

void World::player_get_ground_sensors_positions(float* sensor_a_x, float* sensor_a_y, float* sensor_b_x, float* sensor_b_y) {
	switch (player.mode) {
		default:
		case PlayerMode::FLOOR: {
			*sensor_a_x = player.x - PLAYER_WIDTH_RADIUS;
			*sensor_a_y = player.y + PLAYER_HEIGHT_RADIUS;
			*sensor_b_x = player.x + PLAYER_WIDTH_RADIUS;
			*sensor_b_y = player.y + PLAYER_HEIGHT_RADIUS;
			break;
		}
		case PlayerMode::RIGHT_WALL: {
			*sensor_a_x = player.x + PLAYER_HEIGHT_RADIUS;
			*sensor_a_y = player.y + PLAYER_WIDTH_RADIUS;
			*sensor_b_x = player.x + PLAYER_HEIGHT_RADIUS;
			*sensor_b_y = player.y - PLAYER_WIDTH_RADIUS;
			break;
		}
		case PlayerMode::CEILING: {
			*sensor_a_x = player.x + PLAYER_WIDTH_RADIUS;
			*sensor_a_y = player.y - PLAYER_HEIGHT_RADIUS;
			*sensor_b_x = player.x - PLAYER_WIDTH_RADIUS;
			*sensor_b_y = player.y - PLAYER_HEIGHT_RADIUS;
			break;
		}
		case PlayerMode::LEFT_WALL: {
			*sensor_a_x = player.x - PLAYER_HEIGHT_RADIUS;
			*sensor_a_y = player.y - PLAYER_WIDTH_RADIUS;
			*sensor_b_x = player.x - PLAYER_HEIGHT_RADIUS;
			*sensor_b_y = player.y + PLAYER_WIDTH_RADIUS;
			break;
		}
	}
}

int World::sensor_check_ground_floor(float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y) {
	*out_found_tile = false;

	auto _get_height = [this](Tile tile, int ix, int iy) {
		if (!tile.top_solid) {
			return 0;
		}

		int height = 0;
		if (uint8_t* h = get_height(tile.index)) {
			if (tile.hflip && tile.vflip) {
				int hh = h[15 - ix % 16];
				if (hh >= 0xF0) height = 16 - (hh - 0xF0);
				else if (hh == 16) height = 16;
			} else if (!tile.hflip && tile.vflip) {
				int hh = h[ix % 16];
				if (hh >= 0xF0) height = 16 - (hh - 0xF0);
				else if (hh == 16) height = 16;
			}
			else if (tile.hflip && !tile.vflip) {
				int hh = h[15 - ix % 16];
				if (hh <= 0x10) height = hh;
			}
			else if (!tile.hflip && !tile.vflip) {
				int hh = h[ix % 16];
				if (hh <= 0x10) height = hh;
			}
		}
		return height;
	};

	int ix = (int) x;
	int iy = (int) y;

	if (ix < 0 || iy < 0) {
		return 0;
	}

	int tile_x = ix / 16;
	int tile_y = iy / 16;

	if (tile_x >= level_width || tile_y >= level_height) {
		return 0;
	}

	Tile tile = get_tile(tile_x, tile_y);
	int height = _get_height(tile, ix, iy);

	if (height == 0) {
		if (tile_y + 1 < level_height) {
			tile_y++;
			tile = get_tile(tile_x, tile_y);
			height = _get_height(tile, ix, iy);

			*out_tile = tile;
			*out_found_tile = true;
			*out_tile_x = tile_x;
			*out_tile_y = tile_y;
		} else {
			height = 0;
		}

		return (32 - (iy % 16)) - (height + 1);
	} else if (height == 16) {
		if (tile_y - 1 >= 0) {
			tile_y--;
			tile = get_tile(tile_x, tile_y);
			height = _get_height(tile, ix, iy);

			*out_tile = tile;
			*out_found_tile = true;
			*out_tile_x = tile_x;
			*out_tile_y = tile_y;
		} else {
			height = 0;
		}

		return -(iy % 16) - (height + 1);
	} else {
		*out_tile = tile;
		*out_found_tile = true;
		*out_tile_x = tile_x;
		*out_tile_y = tile_y;

		return (16 - (iy % 16)) - (height + 1);
	}

	return 0;
}

int World::sensor_check_ground_right(float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y) {
	*out_found_tile = false;

	auto _get_height = [this](Tile tile, int ix, int iy) {
		if (!tile.left_right_bottom_solid) {
			return 0;
		}

		int height = 0;
		if (uint8_t* h = get_width(tile.index)) {
			if (tile.hflip && tile.vflip) {
				int hh = h[15 - iy % 16];
				if (hh == 16) height = 16;
			} else if (!tile.hflip && tile.vflip) {
				int hh = h[15 - iy % 16];
				if (hh <= 0x10) height = hh;
			} else if (tile.hflip && !tile.vflip) {
				int hh = h[iy % 16];
				if (hh == 16) height = 16;
			} else if (!tile.hflip && !tile.vflip) {
				int hh = h[iy % 16];
				if (hh <= 0x10) height = hh;
			}
		}
		return height;
	};

	int ix = (int) x;
	int iy = (int) y;

	if (ix < 0 || iy < 0) {
		return 0;
	}

	int tile_x = ix / 16;
	int tile_y = iy / 16;

	if (tile_x >= level_width || tile_y >= level_height) {
		return 0;
	}

	Tile tile = get_tile(tile_x, tile_y);
	int height = _get_height(tile, ix, iy);

	if (height == 0) {
		if (tile_x + 1 < level_width) {
			tile_x++;
			tile = get_tile(tile_x, tile_y);
			height = _get_height(tile, ix, iy);

			*out_tile = tile;
			*out_found_tile = true;
			*out_tile_x = tile_x;
			*out_tile_y = tile_y;
		} else {
			height = 0;
		}

		return (32 - (ix % 16)) - (height + 1);
	} else if (height == 16) {
		if (tile_x - 1 >= 0) {
			tile_x--;
			tile = get_tile(tile_x, tile_y);
			height = _get_height(tile, ix, iy);

			*out_tile = tile;
			*out_found_tile = true;
			*out_tile_x = tile_x;
			*out_tile_y = tile_y;
		} else {
			height = 0;
		}

		return -(ix % 16) - (height + 1);
	} else {
		*out_tile = tile;
		*out_found_tile = true;
		*out_tile_x = tile_x;
		*out_tile_y = tile_y;

		return (16 - (ix % 16)) - (height + 1);
	}

	return 0;
}

int World::sensor_check_ground_ceil(float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y) {
	*out_found_tile = false;

	auto _get_height = [this](Tile tile, int ix, int iy) {
		if (!tile.left_right_bottom_solid) {
			return 0;
		}

		int height = 0;
		if (uint8_t* h = get_height(tile.index)) {
			if (tile.hflip && tile.vflip) {
				int hh = h[15 - ix % 16];
				if (hh == 16) height = 16;
			} else if (!tile.hflip && tile.vflip) {
				int hh = h[ix % 16];
				if (hh == 16) height = 16;
			} else if (tile.hflip && !tile.vflip) {
				int hh = h[15 - ix % 16];
				if (hh >= 0xF0) height = 16 - (hh - 0xF0);
				else if (hh == 16) height = 16;
			} else if (!tile.hflip && !tile.vflip) {
				int hh = h[ix % 16];
				if (hh >= 0xF0) height = 16 - (hh - 0xF0);
				else if (hh == 16) height = 16;
			}
		}
		return height;
	};

	int ix = (int) x;
	int iy = (int) y;

	if (ix < 0 || iy < 0) {
		return 0;
	}

	int tile_x = ix / 16;
	int tile_y = iy / 16;

	if (tile_x >= level_width || tile_y >= level_height) {
		return 0;
	}

	Tile tile = get_tile(tile_x, tile_y);
	int height = _get_height(tile, ix, iy);

	if (height == 0) {
		if (tile_y - 1 >= 0) {
			tile_y--;
			tile = get_tile(tile_x, tile_y);
			height = _get_height(tile, ix, iy);

			*out_tile = tile;
			*out_found_tile = true;
			*out_tile_x = tile_x;
			*out_tile_y = tile_y;
		} else {
			height = 0;
		}

		return 16 + (iy % 16) - (height + 1);
	} else if (height == 16) {
		if (tile_y + 1 < level_height) {
			tile_y++;
			tile = get_tile(tile_x, tile_y);
			height = _get_height(tile, ix, iy);

			*out_tile = tile;
			*out_found_tile = true;
			*out_tile_x = tile_x;
			*out_tile_y = tile_y;
		} else {
			height = 0;
		}

		return -16 + (iy % 16) - (height + 1);
	} else {
		*out_tile = tile;
		*out_found_tile = true;
		*out_tile_x = tile_x;
		*out_tile_y = tile_y;

		return (iy % 16) - (height + 1);
	}

	return 0;
}

int World::sensor_check_ground_left(float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y) {
	*out_found_tile = false;

	auto _get_height = [this](Tile tile, int ix, int iy) {
		if (!tile.left_right_bottom_solid) {
			return 0;
		}

		int height = 0;
		if (uint8_t* h = get_width(tile.index)) {
			if (tile.hflip && tile.vflip) {
				int hh = h[15 - iy % 16];
				if (hh <= 0x10) height = hh;
			} else if (!tile.hflip && tile.vflip) {
				int hh = h[15 - iy % 16];
				if (hh == 16) height = 16;
			} else if (tile.hflip && !tile.vflip) {
				int hh = h[iy % 16];
				if (hh <= 0x10) height = hh;
			} else if (!tile.hflip && !tile.vflip) {
				int hh = h[iy % 16];
				if (hh == 16) height = 16;
			}
		}
		return height;
	};

	int ix = (int) x;
	int iy = (int) y;

	if (ix < 0 || iy < 0) {
		return 0;
	}

	int tile_x = ix / 16;
	int tile_y = iy / 16;

	if (tile_x >= level_width || tile_y >= level_height) {
		return 0;
	}

	Tile tile = get_tile(tile_x, tile_y);
	int height = _get_height(tile, ix, iy);

	if (height == 0) {
		if (tile_x - 1 >= 0) {
			tile_x--;
			tile = get_tile(tile_x, tile_y);
			height = _get_height(tile, ix, iy);

			*out_tile = tile;
			*out_found_tile = true;
			*out_tile_x = tile_x;
			*out_tile_y = tile_y;
		} else {
			height = 0;
		}

		return 16 + (ix % 16) - (height + 1);
	} else if (height == 16) {
		if (tile_x + 1 < level_width) {
			tile_x++;
			tile = get_tile(tile_x, tile_y);
			height = _get_height(tile, ix, iy);

			*out_tile = tile;
			*out_found_tile = true;
			*out_tile_x = tile_x;
			*out_tile_y = tile_y;
		} else {
			height = 0;
		}

		return -16 + (ix % 16) - (height + 1);
	} else {
		*out_tile = tile;
		*out_found_tile = true;
		*out_tile_x = tile_x;
		*out_tile_y = tile_y;

		return (ix % 16) - (height + 1);
	}

	return 0;
}

int World::sensor_check_ground(float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y) {
	switch (player.mode) {
		default:
		case PlayerMode::FLOOR:
			return sensor_check_ground_floor(x, y, out_tile, out_found_tile, out_tile_x, out_tile_y);
		case PlayerMode::RIGHT_WALL:
			return sensor_check_ground_right(x, y, out_tile, out_found_tile, out_tile_x, out_tile_y);
		case PlayerMode::CEILING:
			return sensor_check_ground_ceil(x, y, out_tile, out_found_tile, out_tile_x, out_tile_y);
		case PlayerMode::LEFT_WALL:
			return sensor_check_ground_left(x, y, out_tile, out_found_tile, out_tile_x, out_tile_y);
	}
}

void World::Update(float delta) {
	const Uint8* key = SDL_GetKeyboardState(nullptr);

	int input_h = key[SDL_SCANCODE_RIGHT] - key[SDL_SCANCODE_LEFT];

	if (debug) {
		goto l_skip_player_update;
	}

	switch (player.state) {
		case PlayerState::GROUND: {
			// Adjust Ground Speed based on current Ground Angle (Slope Factor).
			player.ground_speed -= PLAYER_SLOPE_FACTOR * dsin(player.ground_angle) * delta;

			// Check for starting a jump.


			// Update Ground Speed based on directional input and apply friction/deceleration.
			if (input_h == 0) {
				player.ground_speed = approach(player.ground_speed, 0.0f, PLAYER_FRICTION * delta);
			} else {
				if (input_h == -sign_int(player.ground_speed)) {
					player.ground_speed += (float)input_h * PLAYER_DEC * delta;
				} else {
					player.ground_speed += (float)input_h * PLAYER_ACC * delta;
				}
			}

			player.ground_speed = clamp(player.ground_speed, -PLAYER_TOP_SPEED, PLAYER_TOP_SPEED);

			// Move the Player object
			player.xspeed =  dcos(player.ground_angle) * player.ground_speed;
			player.yspeed = -dsin(player.ground_angle) * player.ground_speed;

			player.x += player.xspeed * delta;
			player.y += player.yspeed * delta;

			// Grounded Ground Sensor collision occurs.
			float sensor_a_x;
			float sensor_a_y;
			float sensor_b_x;
			float sensor_b_y;
			player_get_ground_sensors_positions(&sensor_a_x, &sensor_a_y,
												&sensor_b_x, &sensor_b_y);

			Tile tile;
			Tile tile_b;
			bool found;
			bool found_b;
			int tile_x;
			int tile_y;
			int tile_x_b;
			int tile_y_b;
			float sensor_x = sensor_a_x;
			float sensor_y = sensor_a_y;
			int dist = sensor_check_ground(sensor_a_x, sensor_a_y, &tile, &found, &tile_x, &tile_y);
			int dist_b = sensor_check_ground(sensor_b_x, sensor_b_y, &tile_b, &found_b, &tile_x_b, &tile_y_b);
			if (dist_b < dist) {
				tile = tile_b;
				found = found_b;
				dist = dist_b;
				tile_x = tile_x_b;
				tile_y = tile_y_b;
				sensor_x = sensor_b_x;
				sensor_y = sensor_b_y;
			}

			switch (player.mode) {
				default:
				case PlayerMode::FLOOR:      player.y += (float) dist; break;
				case PlayerMode::RIGHT_WALL: player.x += (float) dist; break;
				case PlayerMode::CEILING:    player.y -= (float) dist; break;
				case PlayerMode::LEFT_WALL:  player.x -= (float) dist; break;
			}

			if (found) {
				float angle = get_angle(tile.index);
				if (angle == -1.0f) { // flagged
					float a = angle_wrap(player.ground_angle);
					if (a <= 45.0f) {
						player.ground_angle = 0.0f;
					} else if (a <= 134.0f) {
						player.ground_angle = 90.0f;
					} else if (a <= 225.0f) {
						player.ground_angle = 180.0f;
					} else if (a <= 314.0f) {
						player.ground_angle = 270.0f;
					} else {
						player.ground_angle = 0.0f;
					}
				} else {
					if (tile_x * 16 <= (int)sensor_x && (int)sensor_x < (tile_x + 1) * 16
						&& tile_y * 16 <= (int)sensor_y && (int)sensor_y < (tile_y + 1) * 16)
					{
						if (tile.hflip && tile.vflip) player.ground_angle = -(180.0f - angle);
						else if (!tile.hflip && tile.vflip) player.ground_angle = 180.0f - angle;
						else if (tile.hflip && !tile.vflip) player.ground_angle = -angle;
						else if (!tile.hflip && !tile.vflip) player.ground_angle = angle;
						else player.ground_angle = 0.0f;
					}
				}

				float a = angle_wrap(player.ground_angle);
				if (a <= 45.0f) {
					player.mode = PlayerMode::FLOOR;
				} else if (a <= 134.0f) {
					player.mode = PlayerMode::RIGHT_WALL;
				} else if (a <= 225.0f) {
					player.mode = PlayerMode::CEILING;
				} else if (a <= 314.0f) {
					player.mode = PlayerMode::LEFT_WALL;
				} else {
					player.mode = PlayerMode::FLOOR;
				}
			}
			break;
		}

		case PlayerState::AIR: {
			
			break;
		}
	}

	camera_x = (float) ((int)player.x - GAME_W / 2);
	camera_y = (float) ((int)player.y - GAME_H / 2);

l_skip_player_update:

	if (game->key_pressed[SDL_SCANCODE_ESCAPE]) {
		debug ^= true;
	}

	if (debug) {
		float spd = 10.0f;
		if (key[SDL_SCANCODE_LEFT])  {player.x -= spd * delta; camera_x -= spd * delta;}
		if (key[SDL_SCANCODE_RIGHT]) {player.x += spd * delta; camera_x += spd * delta;}
		if (key[SDL_SCANCODE_UP])    {player.y -= spd * delta; camera_y -= spd * delta;}
		if (key[SDL_SCANCODE_DOWN])  {player.y += spd * delta; camera_y += spd * delta;}

		if (key[SDL_SCANCODE_J]) {camera_x -= spd * delta;}
		if (key[SDL_SCANCODE_L]) {camera_x += spd * delta;}
		if (key[SDL_SCANCODE_I]) {camera_y -= spd * delta;}
		if (key[SDL_SCANCODE_K]) {camera_y += spd * delta;}

		player.xspeed = 0.0f;
		player.yspeed = 0.0f;
		player.ground_speed = 0.0f;
	}
}

Tile World::get_tile(int tile_x, int tile_y) {
	if (tile_x < 0 || tile_y < 0) {
		return {};
	}

	if (tile_x >= level_width || tile_y >= level_height) {
		return {};
	}

	return tiles[tile_x + tile_y * level_width];
}

Tile World::get_tile_world(float x, float y) {
	int ix = (int) x;
	int iy = (int) y;

	if (ix < 0 || iy < 0) {
		return {};
	}

	int tile_x = ix / 16;
	int tile_y = iy / 16;

	if (tile_x >= level_width || tile_y >= level_height) {
		return {};
	}

	return tiles[tile_x + tile_y * level_width];
}

uint8_t* World::get_height(int tile_index) {
	if (0 <= tile_index && tile_index < heights.size()) {
		return heights[tile_index].height;
	}
	return nullptr;
}

uint8_t* World::get_width(int tile_index) {
	if (0 <= tile_index && tile_index < widths.size()) {
		return widths[tile_index].height;
	}
	return nullptr;
}

float World::get_angle(int tile_index) {
	if (0 <= tile_index && tile_index < angles.size()) {
		return angles[tile_index];
	}
	return 0.0f;
}

void World::Draw(float delta) {
	const Uint8* key = SDL_GetKeyboardState(nullptr);

	int target_w;
	int target_h;
	SDL_RenderGetLogicalSize(game->renderer, &target_w, &target_h);
	if (target_w == 0 || target_h == 0) {
		SDL_Texture* target = SDL_GetRenderTarget(game->renderer);
		if (target) {
			SDL_QueryTexture(target, nullptr, nullptr, &target_w, &target_h);
		} else {
			SDL_GetWindowSize(game->window, &target_w, &target_h);
		}
	}

	// draw tilemap
	int start_x = max((int)camera_x / 16, 0);
	int start_y = max((int)camera_y / 16, 0);

	int end_x = min((int(camera_x) + target_w) / 16, level_width - 1);
	int end_y = min((int(camera_y) + target_h) / 16, level_height - 1);

	for (int y = start_y; y <= end_y; y++) {
		for (int x = start_x; x <= end_x; x++) {
			Tile tile = get_tile(x, y);

			SDL_Rect src = {
				(tile.index % 16) * 16,
				(tile.index / 16) * 16,
				16,
				16
			};

			SDL_Rect dest = {
				x * 16 - (int)camera_x,
				y * 16 - (int)camera_y,
				16,
				16
			};

			int flip = SDL_FLIP_NONE;
			if (tile.hflip) flip |= SDL_FLIP_HORIZONTAL;
			if (tile.vflip) flip |= SDL_FLIP_VERTICAL;
			SDL_RenderCopyEx(game->renderer, tex_ghz16, &src, &dest, 0.0, nullptr, (SDL_RendererFlip) flip);

			if (key[SDL_SCANCODE_3]) {
				if (tile.hflip || tile.vflip) {
					SDL_SetTextureColorMod(height_texture, 128, 128, 128);
				} else {
					SDL_SetTextureColorMod(height_texture, 255, 255, 255);
				}
				SDL_RenderCopyEx(game->renderer, height_texture, &src, &dest, 0.0, nullptr, (SDL_RendererFlip) flip);
			}
			if (key[SDL_SCANCODE_4]) {
				if (tile.hflip || tile.vflip) {
					SDL_SetTextureColorMod(width_texture, 128, 128, 128);
				} else {
					SDL_SetTextureColorMod(width_texture, 255, 255, 255);
				}
				SDL_RenderCopyEx(game->renderer, width_texture, &src, &dest, 0.0, nullptr, (SDL_RendererFlip) flip);
			}
			if (key[SDL_SCANCODE_5]) {
				float angle = get_angle(tile.index);
				if (angle == -1.0f && (tile.top_solid || tile.left_right_bottom_solid)) {
					DrawTextShadow(&game->fnt_CP437, "*", dest.x + 8, dest.y + 8, HALIGN_CENTER, VALIGN_MIDDLE);
				}
			}
			if (key[SDL_SCANCODE_6]) {
				if (tile.top_solid) {
					// SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
					// SDL_RenderDrawLine(game->renderer, dest.x, dest.y, dest.x + 16, dest.y);
					SDL_RenderCopyEx(game->renderer, height_texture, &src, &dest, 0.0, nullptr, (SDL_RendererFlip) flip);
				}
			}
			if (key[SDL_SCANCODE_7]) {
				if (tile.left_right_bottom_solid) {
					SDL_RenderCopyEx(game->renderer, height_texture, &src, &dest, 0.0, nullptr, (SDL_RendererFlip) flip);
				}
			}
		}
	}

	// draw player hitbox
	{
		SDL_Rect rect;
		switch (player.mode) {
			default:
			case PlayerMode::FLOOR:
			case PlayerMode::CEILING: {
				rect = {
					(int) (player.x - PLAYER_WIDTH_RADIUS  - camera_x),
					(int) (player.y - PLAYER_HEIGHT_RADIUS - camera_y),
					(int) (PLAYER_WIDTH_RADIUS  * 2.0f + 1.0f),
					(int) (PLAYER_HEIGHT_RADIUS * 2.0f + 1.0f)
				};
				break;
			}
			case PlayerMode::RIGHT_WALL:
			case PlayerMode::LEFT_WALL: {
				rect = {
					(int) (player.x - PLAYER_HEIGHT_RADIUS - camera_x),
					(int) (player.y - PLAYER_WIDTH_RADIUS  - camera_y),
					(int) (PLAYER_HEIGHT_RADIUS * 2.0f + 1.0f),
					(int) (PLAYER_WIDTH_RADIUS  * 2.0f + 1.0f)
				};
				break;
			}
		}
		SDL_SetRenderDrawColor(game->renderer, 128, 128, 255, 255);
		SDL_RenderDrawRect(game->renderer, &rect);
	}

	// draw player ground sensors
	{
		float sensor_a_x;
		float sensor_a_y;
		float sensor_b_x;
		float sensor_b_y;
		player_get_ground_sensors_positions(&sensor_a_x, &sensor_a_y, &sensor_b_x, &sensor_b_y);
		SDL_SetRenderDrawColor(game->renderer, 255, 0, 0, 255);
		SDL_RenderDrawPoint(game->renderer,
							(int) (sensor_a_x - camera_x),
							(int) (sensor_a_y - camera_y));
		SDL_SetRenderDrawColor(game->renderer, 0, 0, 255, 255);
		SDL_RenderDrawPoint(game->renderer,
							(int) (sensor_b_x - camera_x),
							(int) (sensor_b_y - camera_y));
	}

	if (debug) {
		float x = game->mouse_x + camera_x;
		float y = game->mouse_y + camera_y;
		int tile_x = (int)x / 16;
		int tile_y = (int)y / 16;
		tile_x = clamp(tile_x, 0, level_width  - 1);
		tile_y = clamp(tile_y, 0, level_height - 1);
		
		SDL_Rect rect = {tile_x * 16 - (int)camera_x, tile_y * 16 - (int)camera_y, 16, 16};
		SDL_SetRenderDrawColor(game->renderer, 196, 196, 196, 255);
		SDL_RenderDrawRect(game->renderer, &rect);
	}

	if (key[SDL_SCANCODE_1]) {
		SDL_Rect dest = {};
		SDL_QueryTexture(height_texture, nullptr, nullptr, &dest.w, &dest.h);
		SDL_RenderCopy(game->renderer, height_texture, nullptr, &dest);
	}
	if (key[SDL_SCANCODE_2]) {
		SDL_Rect dest = {};
		SDL_QueryTexture(height_texture, nullptr, nullptr, &dest.w, &dest.h);
		SDL_RenderCopy(game->renderer, width_texture, nullptr, &dest);
	}
}
