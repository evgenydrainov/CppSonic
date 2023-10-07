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

World::World() { world = this; }

void World::Init() {
	target_w = GAME_W;
	target_h = GAME_H;

#ifndef EDITOR
	tileset.LoadFromFile("levels/GHZ1/tileset.bin",
						 "levels/GHZ1/tileset.png");
	tilemap.LoadFromFile("levels/GHZ1/tilemap.bin");

	player.x = tilemap.start_x;
	player.y = tilemap.start_y;

	camera_x = player.x - float(GAME_W) / 2.0f;
	camera_y = player.y - float(GAME_H) / 2.0f;
#endif

	anim_idle.texture = IMG_LoadTexture(game->renderer, "anim/anim_idle.png");
	anim_idle.frame_count = 1;
	anim_walk.texture = IMG_LoadTexture(game->renderer, "anim/anim_walk.png");
	anim_walk.frame_count = 6;
	anim_run.texture = IMG_LoadTexture(game->renderer, "anim/anim_run.png");
	anim_run.frame_count = 4;

	player.anim = &anim_idle;
	player.next_anim = &anim_idle;
}

void World::Quit() {}

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

int World::ground_sensor_check_floor(float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y) {
	*out_found_tile = false;

	auto _get_height = [this](Tile tile, int ix, int iy) {
		if (!tile.top_solid) {
			return 0;
		}

		int height = 0;
		if (uint8_t* h = tileset.GetTileHeight(tile.index)) {
			if (tile.hflip && tile.vflip) {
				int hh = h[15 - ix % 16];
				if (hh >= 0xF0) height = 16 - (hh - 0xF0);
				else if (hh == 16) height = 16;
			} else if (!tile.hflip && tile.vflip) {
				int hh = h[ix % 16];
				if (hh >= 0xF0) height = 16 - (hh - 0xF0);
				else if (hh == 16) height = 16;
			} else if (tile.hflip && !tile.vflip) {
				int hh = h[15 - ix % 16];
				if (hh <= 0x10) height = hh;
			} else if (!tile.hflip && !tile.vflip) {
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

	if (tile_x >= tilemap.width || tile_y >= tilemap.height) {
		return 0;
	}

	Tile tile = tilemap.GetTileA(tile_x, tile_y);
	int height = _get_height(tile, ix, iy);

	if (height == 0) {
		if (tile_y + 1 < tilemap.height) {
			tile_y++;
			tile = tilemap.GetTileA(tile_x, tile_y);
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
			tile = tilemap.GetTileA(tile_x, tile_y);
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

int World::ground_sensor_check_right(float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y) {
	*out_found_tile = false;

	auto _get_height = [this](Tile tile, int ix, int iy) {
		if (!tile.left_right_bottom_solid) {
			return 0;
		}

		int height = 0;
		if (uint8_t* h = tileset.GetTileWidth(tile.index)) {
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

	if (tile_x >= tilemap.width || tile_y >= tilemap.height) {
		return 0;
	}

	Tile tile = tilemap.GetTileA(tile_x, tile_y);
	int height = _get_height(tile, ix, iy);

	if (height == 0) {
		if (tile_x + 1 < tilemap.width) {
			tile_x++;
			tile = tilemap.GetTileA(tile_x, tile_y);
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
			tile = tilemap.GetTileA(tile_x, tile_y);
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

int World::ground_sensor_check_ceil(float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y) {
	*out_found_tile = false;

	auto _get_height = [this](Tile tile, int ix, int iy) {
		if (!tile.left_right_bottom_solid) {
			return 0;
		}

		int height = 0;
		if (uint8_t* h = tileset.GetTileHeight(tile.index)) {
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

	if (tile_x >= tilemap.width || tile_y >= tilemap.height) {
		return 0;
	}

	Tile tile = tilemap.GetTileA(tile_x, tile_y);
	int height = _get_height(tile, ix, iy);

	if (height == 0) {
		if (tile_y - 1 >= 0) {
			tile_y--;
			tile = tilemap.GetTileA(tile_x, tile_y);
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
		if (tile_y + 1 < tilemap.height) {
			tile_y++;
			tile = tilemap.GetTileA(tile_x, tile_y);
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

int World::ground_sensor_check_left(float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y) {
	*out_found_tile = false;

	auto _get_height = [this](Tile tile, int ix, int iy) {
		if (!tile.left_right_bottom_solid) {
			return 0;
		}

		int height = 0;
		if (uint8_t* h = tileset.GetTileWidth(tile.index)) {
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

	if (tile_x >= tilemap.width || tile_y >= tilemap.height) {
		return 0;
	}

	Tile tile = tilemap.GetTileA(tile_x, tile_y);
	int height = _get_height(tile, ix, iy);

	if (height == 0) {
		if (tile_x - 1 >= 0) {
			tile_x--;
			tile = tilemap.GetTileA(tile_x, tile_y);
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
		if (tile_x + 1 < tilemap.width) {
			tile_x++;
			tile = tilemap.GetTileA(tile_x, tile_y);
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

int World::ground_sensor_check(float x, float y, Tile* out_tile, bool* out_found_tile, int* out_tile_x, int* out_tile_y) {
	switch (player.mode) {
		default:
		case PlayerMode::FLOOR:
			return ground_sensor_check_floor(x, y, out_tile, out_found_tile, out_tile_x, out_tile_y);
		case PlayerMode::RIGHT_WALL:
			return ground_sensor_check_right(x, y, out_tile, out_found_tile, out_tile_x, out_tile_y);
		case PlayerMode::CEILING:
			return ground_sensor_check_ceil(x, y, out_tile, out_found_tile, out_tile_x, out_tile_y);
		case PlayerMode::LEFT_WALL:
			return ground_sensor_check_left(x, y, out_tile, out_found_tile, out_tile_x, out_tile_y);
	}
}

void World::player_ground_sensor_collision() {
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
	int dist = ground_sensor_check(sensor_a_x, sensor_a_y, &tile, &found, &tile_x, &tile_y);
	int dist_b = ground_sensor_check(sensor_b_x, sensor_b_y, &tile_b, &found_b, &tile_x_b, &tile_y_b);
	if (dist_b < dist) {
		tile = tile_b;
		found = found_b;
		dist = dist_b;
		tile_x = tile_x_b;
		tile_y = tile_y_b;
		sensor_x = sensor_b_x;
		sensor_y = sensor_b_y;
	}

	if (player.state == PlayerState::GROUND && dist <= 14 || player.state == PlayerState::AIR && dist <= 0) {
		switch (player.mode) {
			default:
			case PlayerMode::FLOOR:      player.y += (float) dist; break;
			case PlayerMode::RIGHT_WALL: player.x += (float) dist; break;
			case PlayerMode::CEILING:    player.y -= (float) dist; break;
			case PlayerMode::LEFT_WALL:  player.x -= (float) dist; break;
		}

		if (found) {
			float angle = tileset.GetTileAngle(tile.index);
			if (angle == -1.0f) { // flagged
				// float a = angle_wrap(player.ground_angle);
				// if (a <= 45.0f) {
				// 	player.ground_angle = 0.0f;
				// } else if (a <= 134.0f) {
				// 	player.ground_angle = 90.0f;
				// } else if (a <= 225.0f) {
				// 	player.ground_angle = 180.0f;
				// } else if (a <= 314.0f) {
				// 	player.ground_angle = 270.0f;
				// } else {
				// 	player.ground_angle = 0.0f;
				// }

				switch (player.mode) {
					case PlayerMode::FLOOR:      player.ground_angle = 0.0f;   break;
					case PlayerMode::RIGHT_WALL: player.ground_angle = 90.0f;  break;
					case PlayerMode::CEILING:    player.ground_angle = 180.0f; break;
					case PlayerMode::LEFT_WALL:  player.ground_angle = 270.0f; break;
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

		float player_move_dir = angle_wrap(point_direction(0.0f, 0.0f, player.xspeed, player.yspeed));

		if (player.state == PlayerState::AIR) {
			player.state = PlayerState::GROUND;
			float a = angle_wrap(player.ground_angle);
			if (a <= 23.0f || a >= 339.0f) {
				// flat

				player.ground_speed = player.xspeed;
			} else if (a <= 45.0f || a >= 316.0f) {
				// slope

				if (player_move_dir <= 45.0f || player_move_dir >= 316.0f || (136.0f <= player_move_dir && player_move_dir <= 225.0f)) {
					// mostly right or mostly left
					player.ground_speed = player.xspeed;
				} else {
					player.ground_speed = player.yspeed * 0.5f * -sign(dsin(player.ground_angle));
				}
			} else {
				// steep

				if (player_move_dir <= 45.0f || player_move_dir >= 316.0f || (136.0f <= player_move_dir && player_move_dir <= 225.0f)) {
					// mostly right or mostly left
					player.ground_speed = player.xspeed;
				} else {
					player.ground_speed = player.yspeed * -sign(dsin(player.ground_angle));
				}
			}
		}
	} else {
		player.state = PlayerState::AIR;
	}
}

void World::Update(float delta) {
	const Uint8* key = SDL_GetKeyboardState(nullptr);

	{
		uint32_t prev = input;
		input = 0;

		input |= INPUT_RIGHT * key[SDL_SCANCODE_RIGHT];
		input |= INPUT_UP    * key[SDL_SCANCODE_UP];
		input |= INPUT_LEFT  * key[SDL_SCANCODE_LEFT];
		input |= INPUT_DOWN  * key[SDL_SCANCODE_DOWN];
		input |= INPUT_JUMP  * key[SDL_SCANCODE_Z];

		input_press   = (~prev) & input;
		input_release = prev & (~input);
	}

	int input_h = key[SDL_SCANCODE_RIGHT] - key[SDL_SCANCODE_LEFT];

	float left = PLAYER_WIDTH_RADIUS  + 1.0f;
	float top  = PLAYER_HEIGHT_RADIUS + 1.0f;
	float right  = (float)tilemap.width  * 16.0f - PLAYER_WIDTH_RADIUS  - 1.0f;
	float bottom = (float)tilemap.height * 16.0f - PLAYER_HEIGHT_RADIUS - 1.0f;

	if (debug) {
		goto l_skip_player_update;
	}

	switch (player.state) {
		case PlayerState::GROUND: {
			// 3. Adjust Ground Speed based on current Ground Angle (Slope Factor).
			player.ground_speed -= PLAYER_SLOPE_FACTOR * dsin(player.ground_angle) * delta;

			// 4. Check for starting a jump.
			if (input_press & INPUT_JUMP) {
				player.xspeed -= PLAYER_JUMP_FORCE * dsin(player.ground_angle);
				player.yspeed -= PLAYER_JUMP_FORCE * dcos(player.ground_angle);
				player.state = PlayerState::AIR;
				player.ground_angle = 0.0f;
			}

			// 5. Update Ground Speed based on directional input and apply friction/deceleration.
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

			// 11. Move the Player object
			if (player.state != PlayerState::AIR) {
				player.xspeed =  dcos(player.ground_angle) * player.ground_speed;
				player.yspeed = -dsin(player.ground_angle) * player.ground_speed;
			}

			player.x += player.xspeed * delta;
			player.y += player.yspeed * delta;

			if (player.x < left)   {player.x = left;  player.ground_speed = 0.0f;}
			if (player.x > right)  {player.x = right; player.ground_speed = 0.0f;}
			if (player.y < top)    {player.y = top;}
			if (player.y > bottom) {player.y = bottom;}

			// 13. Grounded Ground Sensor collision occurs.
			player_ground_sensor_collision();

			if (player.ground_speed == 0.0f) {
				player.next_anim = &anim_idle;
				player.frame_duration = 1;
			} else {
				if (fabsf(player.ground_speed) >= PLAYER_TOP_SPEED) {
					player.next_anim = &anim_run;
				} else {
					player.next_anim = &anim_walk;
				}
				player.frame_duration = (int) max(0.0f, 8.0f - fabsf(player.ground_speed));

				player.facing = sign_int(player.ground_speed);
			}
			break;
		}

		case PlayerState::AIR: {
			player.mode = PlayerMode::FLOOR;

			// 1. Check for jump button release (variable jump velocity).
			if (!(input & INPUT_JUMP)) {
				if (player.yspeed < -4.0f) {
					player.yspeed = -4.0f;
				}
			}

			// 3. Update X Speed based on directional input.
			player.xspeed += (float)input_h * PLAYER_ACC * delta;

			player.xspeed = clamp(player.xspeed, -PLAYER_TOP_SPEED, PLAYER_TOP_SPEED);

			// 4. Apply air drag.
			if (-4.0f < player.yspeed && player.yspeed < 0.0f) {
				player.xspeed -= floorf(player.xspeed * 8.0f) / 256.0f * delta;
			}

			// 5. Move the Player object
			player.x += player.xspeed * delta;
			player.y += player.yspeed * delta;

			if (player.x < left)   {player.x = left;   player.xspeed = 0.0f;}
			if (player.x > right)  {player.x = right;  player.xspeed = 0.0f;}
			if (player.y < top)    {player.y = top;    player.yspeed = 0.0f;}
			if (player.y > bottom) {player.y = bottom; player.yspeed = 0.0f;}

			// 7. Apply gravity.
			player.yspeed += GRAVITY * delta;

			// 10. Rotate Ground Angle back to 0.
			player.ground_angle -= clamp(angle_difference(player.ground_angle, 0.0f), -2.8125f * delta, 2.8125f * delta);

			// 11. All air collision checks occur here.
			player_ground_sensor_collision();
			break;
		}
	}

	// animate player
	if (player.anim != player.next_anim) {
		player.anim = player.next_anim;
		player.frame_timer = player.frame_duration;
		player.frame_index = 0;
	} else {
		if (player.frame_timer > 0) {
			player.frame_timer--;
		} else {
			player.frame_index++;
			if (player.frame_index >= player.anim->frame_count) player.frame_index = 0;

			player.frame_timer = player.frame_duration;
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

void World::Draw(float delta) {
	const Uint8* key = SDL_GetKeyboardState(nullptr);

	// draw tilemap
	int start_x = max(int(camera_x) / 16, 0);
	int start_y = max(int(camera_y) / 16, 0);

	int end_x = min((int(camera_x) + target_w) / 16, tilemap.width  - 1);
	int end_y = min((int(camera_y) + target_h) / 16, tilemap.height - 1);

	for (int y = start_y; y <= end_y; y++) {
		for (int x = start_x; x <= end_x; x++) {
			Tile tile = tilemap.GetTileA(x, y);

			SDL_Rect src = tileset.GetTextureSrcRect(tile.index);

			SDL_Rect dest = {
				x * 16 - int(camera_x),
				y * 16 - int(camera_y),
				16,
				16
			};

			int flip = SDL_FLIP_NONE;
			if (tile.hflip) flip |= SDL_FLIP_HORIZONTAL;
			if (tile.vflip) flip |= SDL_FLIP_VERTICAL;
			SDL_RenderCopyEx(game->renderer, tileset.texture, &src, &dest, 0.0, nullptr, (SDL_RendererFlip) flip);

			if (key[SDL_SCANCODE_3]) {
				if (tile.hflip || tile.vflip) {
					SDL_SetTextureColorMod(tileset.height_texture, 128, 128, 128);
				} else {
					SDL_SetTextureColorMod(tileset.height_texture, 255, 255, 255);
				}
				SDL_RenderCopyEx(game->renderer, tileset.height_texture, &src, &dest, 0.0, nullptr, (SDL_RendererFlip) flip);
			}
			if (key[SDL_SCANCODE_4]) {
				if (tile.hflip || tile.vflip) {
					SDL_SetTextureColorMod(tileset.width_texture, 128, 128, 128);
				} else {
					SDL_SetTextureColorMod(tileset.width_texture, 255, 255, 255);
				}
				SDL_RenderCopyEx(game->renderer, tileset.width_texture, &src, &dest, 0.0, nullptr, (SDL_RendererFlip) flip);
			}
			if (key[SDL_SCANCODE_5]) {
				float angle = tileset.GetTileAngle(tile.index);
				if (angle == -1.0f && (tile.top_solid || tile.left_right_bottom_solid)) {
					DrawTextShadow(&game->fnt_CP437, "*", dest.x + 8, dest.y + 8, HALIGN_CENTER, VALIGN_MIDDLE);
				}
			}
			if (key[SDL_SCANCODE_6]) {
				if (tile.top_solid) {
					SDL_RenderCopyEx(game->renderer, tileset.height_texture, &src, &dest, 0.0, nullptr, (SDL_RendererFlip) flip);
				}
			}
			if (key[SDL_SCANCODE_7]) {
				if (tile.left_right_bottom_solid) {
					SDL_RenderCopyEx(game->renderer, tileset.height_texture, &src, &dest, 0.0, nullptr, (SDL_RendererFlip) flip);
				}
			}
		}
	}

	// draw player
	{
		SDL_Rect src = {
			player.frame_index * 66,
			0,
			66,
			66
		};
		SDL_Rect dest = {
			int(player.x) - int(camera_x) - 33,
			int(player.y) - int(camera_y) - 33,
			66,
			66
		};
		SDL_RendererFlip flip = (player.facing == 1) ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
		SDL_RenderCopyEx(game->renderer, player.anim->texture, &src, &dest, -roundf(player.ground_angle / 45.0f) * 45.0f, nullptr, flip);
	}

	// draw player hitbox
	{
		SDL_Rect rect;
		switch (player.mode) {
			default:
			case PlayerMode::FLOOR:
			case PlayerMode::CEILING: {
				rect = {
					int(player.x - PLAYER_WIDTH_RADIUS)  - int(camera_x),
					int(player.y - PLAYER_HEIGHT_RADIUS) - int(camera_y),
					int(PLAYER_WIDTH_RADIUS  * 2.0f) + 1,
					int(PLAYER_HEIGHT_RADIUS * 2.0f) + 1
				};
				break;
			}
			case PlayerMode::RIGHT_WALL:
			case PlayerMode::LEFT_WALL: {
				rect = {
					int(player.x - PLAYER_HEIGHT_RADIUS) - int(camera_x),
					int(player.y - PLAYER_WIDTH_RADIUS)  - int(camera_y),
					int(PLAYER_HEIGHT_RADIUS * 2.0f) + 1,
					int(PLAYER_WIDTH_RADIUS  * 2.0f) + 1
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
							int(sensor_a_x) - int(camera_x),
							int(sensor_a_y) - int(camera_y));
		SDL_SetRenderDrawColor(game->renderer, 0, 0, 255, 255);
		SDL_RenderDrawPoint(game->renderer,
							int(sensor_b_x) - int(camera_x),
							int(sensor_b_y) - int(camera_y));
	}

	if (debug) {
		float x = game->mouse_x + camera_x;
		float y = game->mouse_y + camera_y;
		int tile_x = (int)x / 16;
		int tile_y = (int)y / 16;
		tile_x = clamp(tile_x, 0, tilemap.width  - 1);
		tile_y = clamp(tile_y, 0, tilemap.height - 1);
		
		SDL_Rect rect = {tile_x * 16 - (int)camera_x, tile_y * 16 - (int)camera_y, 16, 16};
		SDL_SetRenderDrawColor(game->renderer, 196, 196, 196, 255);
		SDL_RenderDrawRect(game->renderer, &rect);
	}

	if (key[SDL_SCANCODE_1]) {
		SDL_Rect dest = {};
		SDL_QueryTexture(tileset.height_texture, nullptr, nullptr, &dest.w, &dest.h);
		SDL_RenderCopy(game->renderer, tileset.height_texture, nullptr, &dest);
	}
	if (key[SDL_SCANCODE_2]) {
		SDL_Rect dest = {};
		SDL_QueryTexture(tileset.height_texture, nullptr, nullptr, &dest.w, &dest.h);
		SDL_RenderCopy(game->renderer, tileset.width_texture, nullptr, &dest);
	}
}
