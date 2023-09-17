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

void World::load_sonic1_genesis_level() {
	uint8_t* level_data = (uint8_t*) SDL_LoadFile("levels/ghz1.bin", nullptr);

	int level_width_in_chunks = (int)level_data[0] + 1;
	int level_height_in_chunks = (int)level_data[1] + 1;
	level_width = level_width_in_chunks * 16;
	level_height = level_height_in_chunks * 16;

	uint16_t* level_chunk_data = (uint16_t*) SDL_LoadFile("map256/GHZ.bin", nullptr);

	uint8_t* collision_array = (uint8_t*) SDL_LoadFile("collide/Collision Array (Normal).bin", nullptr);

	size_t level_col_indicies_count;
	uint8_t* level_col_indicies = (uint8_t*) SDL_LoadFile("collide/GHZ.bin", &level_col_indicies_count);

	uint16_t* startpos = (uint16_t*) SDL_LoadFile("startpos/ghz1.bin", nullptr);
	startpos[0] = (startpos[0] << 8) | (startpos[0] >> 8);
	startpos[1] = (startpos[1] << 8) | (startpos[1] >> 8);
	player.x = (float) startpos[0];
	player.y = (float) startpos[1];
	camera_x = player.x - (float)GAME_W / 2.0f;
	camera_y = player.y - (float)GAME_H / 2.0f;
	SDL_free(startpos);

	uint8_t* angle_map = (uint8_t*) SDL_LoadFile("collide/Angle Map.bin", nullptr);

	tile_count = level_width * level_height;
	tiles = (Tile*) malloc(tile_count * sizeof(*tiles));
	memset(tiles, 0, tile_count * sizeof(*tiles));

	height_count = (int) level_col_indicies_count;
	heights = (TileHeight*) malloc(height_count * sizeof(*heights));
	memset(heights, 0, height_count * sizeof(*heights));

	angle_count = height_count;
	angles = (float*) malloc(angle_count * sizeof(*angles));
	memset(angles, 0, angle_count * sizeof(*angles));

	for (int y = 0; y < level_height; y++) {
		for (int x = 0; x < level_width; x++) {
			int chunk_x = x / 16;
			int chunk_y = y / 16;

			uint8_t chunk_index = level_data[2 + chunk_x + chunk_y * level_width_in_chunks] & 0b0111'1111;
			if (chunk_index == 0) {
				continue;
			}

			uint16_t* tiles = &level_chunk_data[(chunk_index - 1) * 256];

			int tile_in_chunk_x = x % 16;
			int tile_in_chunk_y = y % 16;

			uint16_t tile = tiles[tile_in_chunk_x + tile_in_chunk_y * 16];
			tile = (tile << 8) | (tile >> 8);

			this->tiles[x + y * level_width].index = tile & 0b0000'0011'1111'1111;
			this->tiles[x + y * level_width].hflip = tile & 0b0000'1000'0000'0000;
			this->tiles[x + y * level_width].vflip = tile & 0b0001'0000'0000'0000;
		}
	}

	for (int i = 0; i < height_count; i++) {
		uint8_t index = level_col_indicies[i];
		uint8_t* height = &collision_array[index * 16];
		for (int j = 0; j < 16; j++) {
			heights[i].height[j] = height[j];
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
		SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, 16 * 16, ((height_count + 15) / 16) * 16, 32, SDL_PIXELFORMAT_ARGB8888);
		SDL_FillRect(surf, nullptr, 0x00000000);

		for (int tile_index = 0; tile_index < height_count; tile_index++) {
			uint8_t* height = get_height(tile_index);
			for (int i = 0; i < 16; i++) {
				if (height[i] == 0) {
					continue;
				}

				int x = tile_index % 16;
				int y = tile_index / 16;

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
		}

		height_texture = SDL_CreateTextureFromSurface(game->renderer, surf);

		SDL_FreeSurface(surf);
	}

out:
	SDL_free(level_data);
	SDL_free(level_chunk_data);
	SDL_free(collision_array);
	SDL_free(level_col_indicies);
	SDL_free(angle_map);
}

void World::Init() {
	load_sonic1_genesis_level();
}

void World::Quit() {
	SDL_DestroyTexture(height_texture);
	SDL_DestroyTexture(tex_ghz16);

	free(angles);
	free(heights);
	free(tiles);
}

void World::Update(float delta) {
	const Uint8* key = SDL_GetKeyboardState(nullptr);

	int input_h = key[SDL_SCANCODE_RIGHT] - key[SDL_SCANCODE_LEFT];

	if (player_debug) {
		goto l_skip_player_update;
	}

	switch (player.state) {
		case PlayerState::GROUND: {
			/*
			

			

			player.xspeed = player.ground_speed;
			player.yspeed = 0.0f;

			if (!player_check_collision(player.x + player.xspeed * delta, player.y)) {
				player.x += player.xspeed * delta;
			} else {
				for (int i = 16; i--;) {
					if (!player_check_collision(player.x + sign(player.xspeed), player.y)) {
						player.x += sign(player.xspeed);
					} else {
						break;
					}
				}
				player.ground_speed = 0.0f;
				player.xspeed = 0.0f;
			}

			if (!player_check_collision(player.x, player.y + player.yspeed * delta)) {
				player.y += player.yspeed * delta;
			} else {
				for (int i = 16; i--;) {
					if (!player_check_collision(player.x, player.y + sign(player.yspeed))) {
						player.y += sign(player.yspeed);
					} else {
						break;
					}
				}
				player.yspeed = 0.0f;
			}

			if (key[SDL_SCANCODE_Z]) {
				player.state = PlayerState::AIR;
				player.yspeed = -PLAYER_JUMP_FORCE;
				player.ground_speed = 0.0f;
			}

			if (!player_check_collision(player.x, player.y + 1.0f)) {
				player.state = PlayerState::AIR;
				player.ground_speed = 0.0f;
			}
			*/

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

			Tile tile_a;
			Tile tile_b;
			bool found_a;
			bool found_b;
			int dist_a = sensor_check(sensor_a_x, sensor_a_y, &tile_a, &found_a);
			int dist_b = sensor_check(sensor_b_x, sensor_b_y, &tile_b, &found_b);

			Tile tile;
			bool found;
			int dist;
			if (dist_a <= dist_b) {
				tile = tile_a;
				found = found_a;
				dist = dist_a;
			} else {
				tile = tile_b;
				found = found_b;
				dist = dist_b;
			}

			player.y += (float) dist;
			if (found) {
				float angle = get_angle(tile.index);
				if (angle == -1.0f) { // flagged
					player.ground_angle = roundf(player.ground_angle / 90.0f) * 90.0f;
				} else {
					player.ground_angle = angle;
				}
			}
			break;
		}

		case PlayerState::AIR: {
			player.xspeed += (float)input_h * PLAYER_ACC * delta;

			player.xspeed = clamp(player.xspeed, -PLAYER_TOP_SPEED, PLAYER_TOP_SPEED);

			player.yspeed += GRAVITY * delta;

			if (!player_check_collision(player.x + player.xspeed * delta, player.y)) {
				player.x += player.xspeed * delta;
			} else {
				for (int i = 16; i--;) {
					if (!player_check_collision(player.x + sign(player.xspeed), player.y)) {
						player.x += sign(player.xspeed);
					} else {
						break;
					}
				}
				player.xspeed = 0.0f;
			}

			if (!player_check_collision(player.x, player.y + player.yspeed * delta)) {
				player.y += player.yspeed * delta;
			} else {
				for (int i = 16; i--;) {
					if (!player_check_collision(player.x, player.y + sign(player.yspeed))) {
						player.y += sign(player.yspeed);
					} else {
						break;
					}
				}
				player.state = PlayerState::GROUND;
				player.ground_speed = player.xspeed;
				player.yspeed = 0.0f;
			}
			break;
		}
	}

	camera_x = player.x - (float)GAME_W / 2.0f;
	camera_y = player.y - (float)GAME_H / 2.0f;

l_skip_player_update:

	if (game->key_pressed[SDL_SCANCODE_ESCAPE]) {
		player_debug ^= true;
	}

	if (player_debug) {
		if (key[SDL_SCANCODE_LEFT])  {player.x -= 5.0f * delta; camera_x -= 5.0f * delta;}
		if (key[SDL_SCANCODE_RIGHT]) {player.x += 5.0f * delta; camera_x += 5.0f * delta;}
		if (key[SDL_SCANCODE_UP])    {player.y -= 5.0f * delta; camera_y -= 5.0f * delta;}
		if (key[SDL_SCANCODE_DOWN])  {player.y += 5.0f * delta; camera_y += 5.0f * delta;}

		if (key[SDL_SCANCODE_J]) {camera_x -= 5.0f * delta;}
		if (key[SDL_SCANCODE_L]) {camera_x += 5.0f * delta;}
		if (key[SDL_SCANCODE_I]) {camera_y -= 5.0f * delta;}
		if (key[SDL_SCANCODE_K]) {camera_y += 5.0f * delta;}

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
	if (0 <= tile_index && tile_index < height_count) {
		return heights[tile_index].height;
	}
	return nullptr;
}

float World::get_angle(int tile_index) {
	if (0 <= tile_index && tile_index < angle_count) {
		return angles[tile_index];
	}
	return 0.0f;
}

int World::sensor_check(float x, float y, Tile* out_tile, bool* out_found_file) {
	*out_found_file = false;

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
	int height = 0;
	if (uint8_t* h = get_height(tile.index)) height = h[ix % 16];

	if (tile.hflip || tile.vflip) {
		height = 0;
	}

	if (height > 0x10) {
		height = 0;
	}

	if (height == 0) {
		if (tile_y + 1 < level_height) {
			tile_y++;
			tile = get_tile(tile_x, tile_y);
			height = 0;
			if (uint8_t* h = get_height(tile.index)) height = h[ix % 16];

			if (tile.hflip || tile.vflip) {
				height = 0;
			}

			if (height > 0x10) {
				height = 0;
			}

			*out_tile = tile;
			*out_found_file = true;
		} else {
			height = 0;
		}

		return (32 - (iy % 16)) - (height + 1);
	} else if (height == 16) {
		if (tile_y - 1 >= 0) {
			tile_y--;
			tile = get_tile(tile_x, tile_y);
			height = 0;
			if (uint8_t* h = get_height(tile.index)) height = h[ix % 16];

			if (tile.hflip || tile.vflip) {
				height = 0;
			}

			if (height > 0x10) {
				height = 0;
			}

			*out_tile = tile;
			*out_found_file = true;
		} else {
			height = 0;
		}

		return -(iy % 16) - (height + 1);
	} else {
		*out_tile = tile;
		*out_found_file = true;

		return (16 - (iy % 16)) - (height + 1);
	}

	return 0;
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
			break;
		}

		case PlayerMode::CEILING: {
			break;
		}

		case PlayerMode::LEFT_WALL: {
			break;
		}
	}
}

bool World::player_check_collision(float x, float y) {
	for (float dy = -PLAYER_HEIGHT_RADIUS; dy <= PLAYER_HEIGHT_RADIUS; dy++) {
		for (float dx = -PLAYER_WIDTH_RADIUS; dx <= PLAYER_WIDTH_RADIUS; dx++) {
			int pixel_x = (int) (x + dx);
			int pixel_y = (int) (y + dy);
			if (pixel_x < 0 || pixel_y < 0) {
				continue;
			}
			int tile_x = pixel_x / 16;
			int tile_y = pixel_y / 16;
			if (tile_x >= level_width || tile_y >= level_height) {
				continue;
			}
			Tile* tile = &tiles[tile_x + tile_y * level_width];
			if (tile->hflip) continue;
			if (tile->vflip) continue;
			if (tile->index >= height_count) {
				continue;
			}
			uint8_t* heights = this->heights[tile->index].height;
			uint8_t height = heights[pixel_x % 16];
			if (height == 0) {
				continue;
			}
			if (height <= 0x10) {
				if (16 - (pixel_y % 16) <= height) {
					return true;
				}
			}
		}
	}
	return false;
}

void World::Draw(float delta) {
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

			if (tile.hflip || tile.vflip) {
				SDL_SetTextureColorMod(height_texture, 128, 128, 128);
			} else {
				SDL_SetTextureColorMod(height_texture, 255, 255, 255);
			}
			SDL_RenderCopyEx(game->renderer, height_texture, &src, &dest, 0.0, nullptr, (SDL_RendererFlip) flip);
		}
	}

	// draw player hitbox
	{
		SDL_Rect rect = {
			(int) (player.x - PLAYER_WIDTH_RADIUS  - camera_x),
			(int) (player.y - PLAYER_HEIGHT_RADIUS - camera_y),
			(int) (PLAYER_WIDTH_RADIUS  * 2.0f + 1.0f),
			(int) (PLAYER_HEIGHT_RADIUS * 2.0f + 1.0f)
		};
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
		SDL_SetRenderDrawColor(game->renderer, 0, 0, 255, 255);
		SDL_RenderDrawPoint(game->renderer,
							(int) (sensor_a_x - camera_x),
							(int) (sensor_a_y - camera_y));
		SDL_RenderDrawPoint(game->renderer,
							(int) (sensor_b_x - camera_x),
							(int) (sensor_b_y - camera_y));
	}

	int draw_x = 0;
	int draw_y = 0;

	{
		int mouse_x_window;
		int mouse_y_window;
		SDL_GetMouseState(&mouse_x_window, &mouse_y_window);
		float mouse_x;
		float mouse_y;
		SDL_RenderWindowToLogical(game->renderer, mouse_x_window, mouse_y_window, &mouse_x, &mouse_y);
		float x = mouse_x + camera_x;
		float y = mouse_y + camera_y;
		int tile_x = (int)x / 16;
		int tile_y = (int)y / 16;
		Tile tile = {};
		uint8_t height_stub[16]{};
		uint8_t* height = height_stub;
		float angle = 0.0f;
		if ((int)x > 0 && (int)y > 0 && tile_x < level_width && tile_y < level_height) {
			tile = get_tile(tile_x, tile_y);
			if (tile.index < height_count) {
				height = get_height(tile.index);
				angle = get_angle(tile.index);
			}

			if (player_debug) {
				SDL_Rect rect = {tile_x * 16 - (int)camera_x, tile_y * 16 - (int)camera_y, 16, 16};
				SDL_SetRenderDrawColor(game->renderer, 128, 128, 128, 255);
				SDL_RenderDrawRect(game->renderer, &rect);
			}
		}

		char buf[200];
		stb_snprintf(buf,
					 sizeof(buf),
					 "x: %f\n"
					 "y: %f\n"
					 "xspeed: %f\n"
					 "yspeed: %f\n"
					 "ground speed: %f\n"
					 "ground angle: %f\n",
					 player.x,
					 player.y,
					 player.xspeed,
					 player.yspeed,
					 player.ground_speed,
					 player.ground_angle);
		draw_y = DrawTextShadow(&game->fnt_CP437, buf, draw_x, draw_y, 0, 0, {220, 220, 220, 255}).y;

		if (player_debug) {
			char buf[200];
			stb_snprintf(buf,
						 sizeof(buf),
						 "mouse x: %d\n"
						 "mouse y: %d\n"
						 "mouse tile: %d\n"
						 "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \n"
						 "angle: %f\n"
						 "hflip: %d vflip: %d\n",
						 tile_x,
						 tile_y,
						 tile.index,
						 height[ 0], height[ 1], height[ 2], height[ 3], height[ 4], height[ 5], height[ 6], height[ 7],
						 height[ 8], height[ 9], height[10], height[11], height[12], height[13], height[14], height[15],
						 angle,
						 tile.hflip, tile.vflip);
			draw_y = DrawTextShadow(&game->fnt_CP437, buf, draw_x, draw_y, 0, 0, {220, 220, 220, 255}).y;
		}
	}

	/*
	{
		SDL_Rect dest = {};
		SDL_QueryTexture(height_texture, nullptr, nullptr, &dest.w, &dest.h);
		SDL_RenderCopy(game->renderer, height_texture, nullptr, &dest);
		for (int i = 0; i < angle_count; i++) {
			uint8_t angle = get_angle(i);
			uint8_t left = angle >> 4;
			uint8_t right = angle & 0b0000'1111;
			SDL_SetRenderDrawColor(game->renderer, 0, 0, 255, 255);
			// SDL_RenderDrawLine(game->renderer,
			// 				   (i % 16) * 16 + 0,
			// 				   (i / 16) * 16 + left,
			// 				   (i % 16) * 16 + 16,
			// 				   (i / 16) * 16 + right);
			float a = get_angle(i);
			SDL_RenderDrawLine(game->renderer,
							   (i % 16) * 16 + 0,
							   (i / 16) * 16 + 16,
							   (i % 16) * 16 + 0  + lengthdir_x(16, a),
							   (i / 16) * 16 + 16 + lengthdir_y(16, a));
		}
	}
	//*/
}
