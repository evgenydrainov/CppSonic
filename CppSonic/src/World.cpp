#include "World.h"

#include "Game.h"
#include "mathh.h"
#include "stb_sprintf.h"

#include <SDL_image.h>
#include <math.h>
#include <string.h>

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

World* world;

void World::load_objects(const char* fname) {
	SDL_RWops* f = nullptr;

	{
		f = SDL_RWFromFile(fname, "rb");

		if (!f) {
			SDL_ShowSimpleMessageBox(0, "ERROR", "Couldn't open objects file.", nullptr);
			goto out;
		}

		int object_count;
		SDL_RWread(f, &object_count, sizeof(object_count), 1);

		if (object_count < 0 || object_count >= MAX_OBJECTS) {
			SDL_ShowSimpleMessageBox(0, "ERROR", "Invalid objects file.", nullptr);
			goto out;
		}

		this->object_count = object_count;

		SDL_RWread(f, objects, sizeof(*objects), object_count);

		for (int i = 0; i < object_count; i++) {
			objects[i].id = next_id++;
		}
	}

out:
	if (f) SDL_RWclose(f);
}

void World::Init() {
	objects = (Object*) calloc(MAX_OBJECTS, sizeof(*objects));

	if (!objects) {
		SDL_ShowSimpleMessageBox(0, "ERROR", "Out of memory.", nullptr);
		exit(1);
	}

	target_w = GAME_W;
	target_h = GAME_H;

	Player* p = &player;

#ifdef EDITOR
	tileset.LoadFromFile("levels/export/tileset.bin",
						 "levels/export/tileset_padded.png");
	tilemap.LoadFromFile("levels/export/tilemap.bin");
	load_objects("levels/export/objects.bin");
#else
	tileset.LoadFromFile("levels/GHZ1/tileset.bin",
						 "levels/GHZ1/tileset_padded.png");
	tilemap.LoadFromFile("levels/GHZ1/tilemap.bin");
	load_objects("levels/GHZ1/objects.bin");
#endif

	p->x = tilemap.start_x;
	p->y = tilemap.start_y;

	camera_x = p->x - float(GAME_W) / 2.0f;
	camera_y = p->y - float(GAME_H) / 2.0f;

	p->anim = anim_idle;
	p->next_anim = anim_idle;
}

void World::Quit() {
	tilemap.Destroy();
	tileset.Destroy();
}

void World::GetGroundSensorsPositions(Player* p, float* sensor_a_x, float* sensor_a_y, float* sensor_b_x, float* sensor_b_y) {
	switch (p->mode) {
		case PlayerMode::FLOOR: {
			*sensor_a_x = p->x - PLAYER_WIDTH_RADIUS;
			*sensor_a_y = p->y + PLAYER_HEIGHT_RADIUS;
			*sensor_b_x = p->x + PLAYER_WIDTH_RADIUS;
			*sensor_b_y = p->y + PLAYER_HEIGHT_RADIUS;
			break;
		}
		case PlayerMode::RIGHT_WALL: {
			*sensor_a_x = p->x + PLAYER_HEIGHT_RADIUS;
			*sensor_a_y = p->y + PLAYER_WIDTH_RADIUS;
			*sensor_b_x = p->x + PLAYER_HEIGHT_RADIUS;
			*sensor_b_y = p->y - PLAYER_WIDTH_RADIUS;
			break;
		}
		case PlayerMode::CEILING: {
			*sensor_a_x = p->x + PLAYER_WIDTH_RADIUS;
			*sensor_a_y = p->y - PLAYER_HEIGHT_RADIUS;
			*sensor_b_x = p->x - PLAYER_WIDTH_RADIUS;
			*sensor_b_y = p->y - PLAYER_HEIGHT_RADIUS;
			break;
		}
		case PlayerMode::LEFT_WALL: {
			*sensor_a_x = p->x - PLAYER_HEIGHT_RADIUS;
			*sensor_a_y = p->y - PLAYER_WIDTH_RADIUS;
			*sensor_b_x = p->x - PLAYER_HEIGHT_RADIUS;
			*sensor_b_y = p->y + PLAYER_WIDTH_RADIUS;
			break;
		}
		default: {
			*sensor_a_x = p->x;
			*sensor_a_y = p->y;
			*sensor_b_x = p->x;
			*sensor_b_y = p->y;
			break;
		}
	}
}

void World::GetPushSensorsPositions(Player* p, float* sensor_e_x, float* sensor_e_y, float* sensor_f_x, float* sensor_f_y) {
	switch (p->mode) {
		case PlayerMode::FLOOR: {
			*sensor_e_x = p->x - PLAYER_PUSH_RADIUS;
			*sensor_e_y = p->y;
			*sensor_f_x = p->x + PLAYER_PUSH_RADIUS;
			*sensor_f_y = p->y;
			if (p->state == PlayerState::GROUND && p->ground_angle == 0.0f) {
				*sensor_e_y += 8.0f;
				*sensor_f_y += 8.0f;
			}
			break;
		}
		case PlayerMode::RIGHT_WALL: {
			*sensor_e_x = p->x;
			*sensor_e_y = p->y + PLAYER_PUSH_RADIUS;
			*sensor_f_x = p->x;
			*sensor_f_y = p->y - PLAYER_PUSH_RADIUS;
			if (p->state == PlayerState::GROUND && p->ground_angle == 0.0f) {
				*sensor_e_x += 8.0f;
				*sensor_f_x += 8.0f;
			}
			break;
		}
		case PlayerMode::CEILING: {
			*sensor_e_x = p->x + PLAYER_PUSH_RADIUS;
			*sensor_e_y = p->y;
			*sensor_f_x = p->x - PLAYER_PUSH_RADIUS;
			*sensor_f_y = p->y;
			if (p->state == PlayerState::GROUND && p->ground_angle == 0.0f) {
				*sensor_e_y -= 8.0f;
				*sensor_f_y -= 8.0f;
			}
			break;
		}
		case PlayerMode::LEFT_WALL: {
			*sensor_e_x = p->x;
			*sensor_e_y = p->y - PLAYER_PUSH_RADIUS;
			*sensor_f_x = p->x;
			*sensor_f_y = p->y + PLAYER_PUSH_RADIUS;
			if (p->state == PlayerState::GROUND && p->ground_angle == 0.0f) {
				*sensor_e_x -= 8.0f;
				*sensor_f_x -= 8.0f;
			}
			break;
		}
		default: {
			*sensor_e_x = p->x;
			*sensor_e_y = p->y;
			*sensor_f_x = p->x;
			*sensor_f_y = p->y;
			break;
		}
	}
}

SensorResult World::SensorCheckDown(float x, float y, int layer) {
	SensorResult result = {};

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
		return result;
	}

	int tile_x = ix / 16;
	int tile_y = iy / 16;

	if (tile_x >= tilemap.width || tile_y >= tilemap.height) {
		return result;
	}

	Tile tile = tilemap.GetTile(tile_x, tile_y, layer);
	int height = _get_height(tile, ix, iy);

	if (height == 0) {
		if (tile_y + 1 < tilemap.height) {
			tile_y++;
			tile = tilemap.GetTile(tile_x, tile_y, layer);
			height = _get_height(tile, ix, iy);

			result.tile = tile;
			result.found = true;
			result.tile_x = tile_x;
			result.tile_y = tile_y;
		} else {
			height = 0;
		}

		result.dist = (32 - (iy % 16)) - (height + 1);
		return result;
	} else if (height == 16) {
		if (tile_y - 1 >= 0) {
			tile_y--;
			tile = tilemap.GetTile(tile_x, tile_y, layer);
			height = _get_height(tile, ix, iy);

			result.tile = tile;
			result.found = true;
			result.tile_x = tile_x;
			result.tile_y = tile_y;
		} else {
			height = 0;
		}

		result.dist = -(iy % 16) - (height + 1);
		return result;
	} else {
		result.tile = tile;
		result.found = true;
		result.tile_x = tile_x;
		result.tile_y = tile_y;

		result.dist = (16 - (iy % 16)) - (height + 1);
		return result;
	}

	return result;
}

SensorResult World::SensorCheckRight(float x, float y, int layer) {
	SensorResult result = {};

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
		return result;
	}

	int tile_x = ix / 16;
	int tile_y = iy / 16;

	if (tile_x >= tilemap.width || tile_y >= tilemap.height) {
		return result;
	}

	Tile tile = tilemap.GetTile(tile_x, tile_y, layer);
	int height = _get_height(tile, ix, iy);

	if (height == 0) {
		if (tile_x + 1 < tilemap.width) {
			tile_x++;
			tile = tilemap.GetTile(tile_x, tile_y, layer);
			height = _get_height(tile, ix, iy);

			result.tile = tile;
			result.found = true;
			result.tile_x = tile_x;
			result.tile_y = tile_y;
		} else {
			height = 0;
		}

		result.dist = (32 - (ix % 16)) - (height + 1);
		return result;
	} else if (height == 16) {
		if (tile_x - 1 >= 0) {
			tile_x--;
			tile = tilemap.GetTile(tile_x, tile_y, layer);
			height = _get_height(tile, ix, iy);

			result.tile = tile;
			result.found = true;
			result.tile_x = tile_x;
			result.tile_y = tile_y;
		} else {
			height = 0;
		}

		result.dist = -(ix % 16) - (height + 1);
		return result;
	} else {
		result.tile = tile;
		result.found = true;
		result.tile_x = tile_x;
		result.tile_y = tile_y;

		result.dist = (16 - (ix % 16)) - (height + 1);
		return result;
	}

	return result;
}

SensorResult World::SensorCheckUp(float x, float y, int layer) {
	SensorResult result = {};

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
		return result;
	}

	int tile_x = ix / 16;
	int tile_y = iy / 16;

	if (tile_x >= tilemap.width || tile_y >= tilemap.height) {
		return result;
	}

	Tile tile = tilemap.GetTile(tile_x, tile_y, layer);
	int height = _get_height(tile, ix, iy);

	if (height == 0) {
		if (tile_y - 1 >= 0) {
			tile_y--;
			tile = tilemap.GetTile(tile_x, tile_y, layer);
			height = _get_height(tile, ix, iy);

			result.tile = tile;
			result.found = true;
			result.tile_x = tile_x;
			result.tile_y = tile_y;
		} else {
			height = 0;
		}

		result.dist = 16 + (iy % 16) - (height + 1);
		return result;
	} else if (height == 16) {
		if (tile_y + 1 < tilemap.height) {
			tile_y++;
			tile = tilemap.GetTile(tile_x, tile_y, layer);
			height = _get_height(tile, ix, iy);

			result.tile = tile;
			result.found = true;
			result.tile_x = tile_x;
			result.tile_y = tile_y;
		} else {
			height = 0;
		}

		result.dist = -16 + (iy % 16) - (height + 1);
		return result;
	} else {
		result.tile = tile;
		result.found = true;
		result.tile_x = tile_x;
		result.tile_y = tile_y;

		result.dist = (iy % 16) - (height + 1);
		return result;
	}

	return result;
}

SensorResult World::SensorCheckLeft(float x, float y, int layer) {
	SensorResult result = {};

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
		return result;
	}

	int tile_x = ix / 16;
	int tile_y = iy / 16;

	if (tile_x >= tilemap.width || tile_y >= tilemap.height) {
		return result;
	}

	Tile tile = tilemap.GetTile(tile_x, tile_y, layer);
	int height = _get_height(tile, ix, iy);

	if (height == 0) {
		if (tile_x - 1 >= 0) {
			tile_x--;
			tile = tilemap.GetTile(tile_x, tile_y, layer);
			height = _get_height(tile, ix, iy);

			result.tile = tile;
			result.found = true;
			result.tile_x = tile_x;
			result.tile_y = tile_y;
		} else {
			height = 0;
		}

		result.dist = 16 + (ix % 16) - (height + 1);
		return result;
	} else if (height == 16) {
		if (tile_x + 1 < tilemap.width) {
			tile_x++;
			tile = tilemap.GetTile(tile_x, tile_y, layer);
			height = _get_height(tile, ix, iy);

			result.tile = tile;
			result.found = true;
			result.tile_x = tile_x;
			result.tile_y = tile_y;
		} else {
			height = 0;
		}

		result.dist = -16 + (ix % 16) - (height + 1);
		return result;
	} else {
		result.tile = tile;
		result.found = true;
		result.tile_x = tile_x;
		result.tile_y = tile_y;

		result.dist = (ix % 16) - (height + 1);
		return result;
	}

	return result;
}

SensorResult World::GroundSensorCheck(Player* p, float x, float y) {
	switch (p->mode) {
		case PlayerMode::FLOOR:
			return SensorCheckDown(x, y, p->layer);
		case PlayerMode::RIGHT_WALL:
			return SensorCheckRight(x, y, p->layer);
		case PlayerMode::CEILING:
			return SensorCheckUp(x, y, p->layer);
		case PlayerMode::LEFT_WALL:
			return SensorCheckLeft(x, y, p->layer);
	}
	return {};
}

SensorResult World::PushSensorECheck(Player* p, float x, float y) {
	switch (p->mode) {
		case PlayerMode::FLOOR:
			return SensorCheckLeft(x, y, p->layer);
		case PlayerMode::RIGHT_WALL:
			return SensorCheckDown(x, y, p->layer);
		case PlayerMode::CEILING:
			return SensorCheckRight(x, y, p->layer);
		case PlayerMode::LEFT_WALL:
			return SensorCheckUp(x, y, p->layer);
	}
	return {};
}

SensorResult World::PushSensorFCheck(Player* p, float x, float y) {
	switch (p->mode) {
		case PlayerMode::FLOOR:
			return SensorCheckRight(x, y, p->layer);
		case PlayerMode::RIGHT_WALL:
			return SensorCheckUp(x, y, p->layer);
		case PlayerMode::CEILING:
			return SensorCheckLeft(x, y, p->layer);
		case PlayerMode::LEFT_WALL:
			return SensorCheckDown(x, y, p->layer);
	}
	return {};
}

bool World::AreGroundSensorsActive(Player* p) {
	switch (p->state) {
		case PlayerState::GROUND:
			return true;
		case PlayerState::AIR:
			return true;
	}
	return false;
}

bool World::IsPushSensorEActive(Player* p) {
	switch (p->state) {
		case PlayerState::GROUND: {
			float a = angle_wrap(p->ground_angle);
			if (!(a <= 90.0f || a >= 270.0f)) {
				return false;
			}

			return (p->ground_speed < 0.0f);
		}
		case PlayerState::AIR:
			return true;
	}
	return false;
}

bool World::IsPushSensorFActive(Player* p) {
	switch (p->state) {
		case PlayerState::GROUND: {
			float a = angle_wrap(p->ground_angle);
			if (!(a <= 90.0f || a >= 270.0f)) {
				return false;
			}

			return (p->ground_speed > 0.0f);
		}
		case PlayerState::AIR:
			return true;
	}
	return false;
}

void World::GroundSensorCollision(Player* p) {
	if (!AreGroundSensorsActive(p)) {
		return;
	}

	float sensor_a_x;
	float sensor_a_y;
	float sensor_b_x;
	float sensor_b_y;
	GetGroundSensorsPositions(p,
							  &sensor_a_x, &sensor_a_y,
							  &sensor_b_x, &sensor_b_y);

	SensorResult res   = GroundSensorCheck(p, sensor_a_x, sensor_a_y);
	SensorResult res_b = GroundSensorCheck(p, sensor_b_x, sensor_b_y);
	float sensor_x = sensor_a_x;
	float sensor_y = sensor_a_y;
	if (res_b.dist < res.dist) {
		res = res_b;
		sensor_x = sensor_b_x;
		sensor_y = sensor_b_y;
	}

	if ((p->state == PlayerState::GROUND && res.dist <= 14)
		|| (p->state == PlayerState::AIR && res.dist <= 0)) {
		switch (p->mode) {
			case PlayerMode::FLOOR:      p->y += (float) res.dist; break;
			case PlayerMode::RIGHT_WALL: p->x += (float) res.dist; break;
			case PlayerMode::CEILING:    p->y -= (float) res.dist; break;
			case PlayerMode::LEFT_WALL:  p->x -= (float) res.dist; break;
		}

		if (res.found) {
			float angle = tileset.GetTileAngle(res.tile.index);
			if (angle == -1.0f) { // flagged
				// float a = angle_wrap(p->ground_angle);
				// if (a <= 45.0f) {
				// 	p->ground_angle = 0.0f;
				// } else if (a <= 134.0f) {
				// 	p->ground_angle = 90.0f;
				// } else if (a <= 225.0f) {
				// 	p->ground_angle = 180.0f;
				// } else if (a <= 314.0f) {
				// 	p->ground_angle = 270.0f;
				// } else {
				// 	p->ground_angle = 0.0f;
				// }

				switch (p->mode) {
					case PlayerMode::FLOOR:      p->ground_angle = 0.0f;   break;
					case PlayerMode::RIGHT_WALL: p->ground_angle = 90.0f;  break;
					case PlayerMode::CEILING:    p->ground_angle = 180.0f; break;
					case PlayerMode::LEFT_WALL:  p->ground_angle = 270.0f; break;
				}
			} else {
				// if (res.tile_x * 16 <= (int)sensor_x && (int)sensor_x < (res.tile_x + 1) * 16
				// 	&& res.tile_y * 16 <= (int)sensor_y && (int)sensor_y < (res.tile_y + 1) * 16)
				{
					if (res.tile.hflip && res.tile.vflip) p->ground_angle = -(180.0f - angle);
					else if (!res.tile.hflip && res.tile.vflip) p->ground_angle = 180.0f - angle;
					else if (res.tile.hflip && !res.tile.vflip) p->ground_angle = -angle;
					else if (!res.tile.hflip && !res.tile.vflip) p->ground_angle = angle;
					else p->ground_angle = 0.0f;
				}
			}
		}

		float a = angle_wrap(p->ground_angle);
		if (a <= 45.0f) {
			p->mode = PlayerMode::FLOOR;
		} else if (a <= 134.0f) {
			p->mode = PlayerMode::RIGHT_WALL;
		} else if (a <= 225.0f) {
			p->mode = PlayerMode::CEILING;
		} else if (a <= 314.0f) {
			p->mode = PlayerMode::LEFT_WALL;
		} else {
			p->mode = PlayerMode::FLOOR;
		}

		float player_move_dir = angle_wrap(point_direction(0.0f, 0.0f, p->xspeed, p->yspeed));

		if (p->state == PlayerState::AIR) {
			p->state = PlayerState::GROUND;
			float a = angle_wrap(p->ground_angle);
			if (a <= 23.0f || a >= 339.0f) {
				// flat

				p->ground_speed = p->xspeed;
			} else if (a <= 45.0f || a >= 316.0f) {
				// slope

				if (player_move_dir <= 45.0f || player_move_dir >= 316.0f || (136.0f <= player_move_dir && player_move_dir <= 225.0f)) {
					// mostly right or mostly left
					p->ground_speed = p->xspeed;
				} else {
					p->ground_speed = p->yspeed * 0.5f * -sign(dsin(p->ground_angle));
				}
			} else {
				// steep

				if (player_move_dir <= 45.0f || player_move_dir >= 316.0f || (136.0f <= player_move_dir && player_move_dir <= 225.0f)) {
					// mostly right or mostly left
					p->ground_speed = p->xspeed;
				} else {
					p->ground_speed = p->yspeed * -sign(dsin(p->ground_angle));
				}
			}
		}
	} else {
		p->state = PlayerState::AIR;
	}
}

void World::PushSensorCollision(Player* p) {
	if (IsPushSensorFActive(p)) {
		float sensor_e_x;
		float sensor_e_y;
		float sensor_f_x;
		float sensor_f_y;
		GetPushSensorsPositions(p, &sensor_e_x, &sensor_e_y, &sensor_f_x, &sensor_f_y);

		SensorResult res = PushSensorFCheck(p, sensor_f_x, sensor_f_y);
		if (res.found) {
			if (res.dist <= 0) {
				switch (p->mode) {
					case PlayerMode::FLOOR:
						p->x += (float) res.dist;
						break;
					case PlayerMode::RIGHT_WALL:
						p->y += (float) res.dist;
						break;
					case PlayerMode::CEILING:
						p->x -= (float) res.dist;
						break;
					case PlayerMode::LEFT_WALL:
						p->y -= (float) res.dist;
						break;
				}
				p->ground_speed = 0.0f;
			}
		}
	}

	if (IsPushSensorEActive(p)) {
		float sensor_e_x;
		float sensor_e_y;
		float sensor_f_x;
		float sensor_f_y;
		GetPushSensorsPositions(p, &sensor_e_x, &sensor_e_y, &sensor_f_x, &sensor_f_y);

		SensorResult res = PushSensorECheck(p, sensor_e_x, sensor_e_y);
		if (res.found) {
			if (res.dist <= 0) {
				switch (p->mode) {
					case PlayerMode::FLOOR:
						p->x -= (float) res.dist;
						break;
					case PlayerMode::RIGHT_WALL:
						p->y -= (float) res.dist;
						break;
					case PlayerMode::CEILING:
						p->x += (float) res.dist;
						break;
					case PlayerMode::LEFT_WALL:
						p->y += (float) res.dist;
						break;
				}
				p->ground_speed = 0.0f;
			}
		}
	}
}

void World::UpdatePlayer(Player* p, float delta) {
	int input_h = 0;
	if (p->control_lock == 0.0f) {
		if (input & INPUT_LEFT)  input_h--;
		if (input & INPUT_RIGHT) input_h++;
	}

	float left = PLAYER_WIDTH_RADIUS  + 1.0f;
	float top  = PLAYER_HEIGHT_RADIUS + 1.0f;
	float right  = (float)tilemap.width  * 16.0f - PLAYER_WIDTH_RADIUS  - 1.0f;
	float bottom = (float)tilemap.height * 16.0f - PLAYER_HEIGHT_RADIUS - 1.0f;

	switch (p->state) {
		case PlayerState::GROUND: {
			// Check for starting a spindash while crouched.


			// Adjust Ground Speed based on current Ground Angle (Slope Factor).
			p->ground_speed -= PLAYER_SLOPE_FACTOR * dsin(p->ground_angle) * delta;

			// Update Ground Speed based on directional input and apply friction/deceleration.
			if (input_h == 0) {
				p->ground_speed = approach(p->ground_speed, 0.0f, PLAYER_FRICTION * delta);
			} else {
				if (input_h == -sign_int(p->ground_speed)) {
					p->ground_speed += (float)input_h * PLAYER_DEC * delta;
				} else {
					p->ground_speed += (float)input_h * PLAYER_ACC * delta;
				}
			}

			p->ground_speed = clamp(p->ground_speed, -PLAYER_TOP_SPEED, PLAYER_TOP_SPEED);

			// Check for starting crouching, balancing on ledges, etc.


			// Move the Player object
			p->xspeed =  dcos(p->ground_angle) * p->ground_speed;
			p->yspeed = -dsin(p->ground_angle) * p->ground_speed;

			p->x += p->xspeed * delta;
			p->y += p->yspeed * delta;

			if (p->x < left)   {p->x = left;  p->ground_speed = 0.0f;}
			if (p->x > right)  {p->x = right; p->ground_speed = 0.0f;}
			if (p->y < top)    {p->y = top;}
			if (p->y > bottom) {p->y = bottom;}

			// Push Sensor collision occurs.
			PushSensorCollision(p);

			// Grounded Ground Sensor collision occurs.
			GroundSensorCollision(p);

			if (p->ground_speed == 0.0f) {
				p->next_anim = anim_idle;
				p->frame_duration = 1;
			} else {
				if (fabsf(p->ground_speed) >= PLAYER_TOP_SPEED) {
					p->next_anim = anim_run;
				} else {
					p->next_anim = anim_walk;
				}
				p->frame_duration = (int) max(0.0f, 8.0f - fabsf(p->ground_speed));
			}

			if (p->ground_speed != 0.0f) {
				p->facing = sign_int(p->ground_speed);
			}

			// Check for slipping/falling when Ground Speed is too low on walls/ceilings.
			{
				float a = angle_wrap(p->ground_angle);
				if (a >= 46.0f && a <= 315.0f) {
					if (fabsf(p->ground_speed) < 2.5f) {
						p->state = PlayerState::AIR;
						p->ground_speed = 0.0f;
						p->control_lock = 30.0f;
					}
				}
			}

			// Check for starting a jump.
			if (input_press & INPUT_JUMP) {
				p->xspeed -= PLAYER_JUMP_FORCE * dsin(p->ground_angle);
				p->yspeed -= PLAYER_JUMP_FORCE * dcos(p->ground_angle);
				p->state = PlayerState::AIR;
				p->ground_angle = 0.0f;
				p->next_anim = anim_roll;
				p->frame_duration = (int) max(0.0f, 4.0f - fabsf(p->ground_speed));
			}

			p->control_lock = max(p->control_lock - delta, 0.0f);
			break;
		}

		case PlayerState::AIR: {
			p->mode = PlayerMode::FLOOR;

			// 1. Check for jump button release (variable jump velocity).
			if (!(input & INPUT_JUMP)) {
				if (p->yspeed < -4.0f) {
					p->yspeed = -4.0f;
				}
			}

			// 3. Update X Speed based on directional input.
			p->xspeed += (float)input_h * PLAYER_ACC * delta;

			p->xspeed = clamp(p->xspeed, -PLAYER_TOP_SPEED, PLAYER_TOP_SPEED);

			// 4. Apply air drag.
			if (-4.0f < p->yspeed && p->yspeed < 0.0f) {
				p->xspeed -= floorf(p->xspeed * 8.0f) / 256.0f * delta;
			}

			// 5. Move the Player object
			p->x += p->xspeed * delta;
			p->y += p->yspeed * delta;

			if (p->x < left)   {p->x = left;   p->xspeed = 0.0f;}
			if (p->x > right)  {p->x = right;  p->xspeed = 0.0f;}
			if (p->y < top)    {p->y = top;    p->yspeed = 0.0f;}
			if (p->y > bottom) {p->y = bottom; p->yspeed = 0.0f;}

			// 7. Apply gravity.
			p->yspeed += GRAVITY * delta;

			// 10. Rotate Ground Angle back to 0.
			p->ground_angle -= clamp(angle_difference(p->ground_angle, 0.0f), -2.8125f * delta, 2.8125f * delta);

			// 11. All air collision checks occur here.
			GroundSensorCollision(p);

			if (p->xspeed != 0.0f) {
				p->facing = sign_int(p->xspeed);
			}
			break;
		}
	}

	// animate player
	if (p->anim != p->next_anim) {
		p->anim = p->next_anim;
		p->frame_timer = p->frame_duration;
		p->frame_index = 0;
	} else {
		if (p->frame_timer > 0) {
			p->frame_timer--;
		} else {
			p->frame_index++;
			if (p->frame_index >= anim_get_frame_count(p->anim)) p->frame_index = 0;

			p->frame_timer = p->frame_duration;
		}
	}
}

void World::Update(float delta) {
	const Uint8* key = SDL_GetKeyboardState(nullptr);
	Player* p = &player;

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

	if (!debug) {
		UpdatePlayer(p, delta);

		for (int i = 0; i < object_count; i++) {
			Object* inst = &objects[i];
			switch (inst->type) {
				case ObjType::VERTICAL_LAYER_SWITCHER: {
					bool active = true;

					if (inst->flags & FLAG_LAYER_SWITCHER_GROUNDED_ONLY) {
						if (p->state != PlayerState::GROUND) {
							active = false;
						}
					}

					if (!(inst->y - inst->radius <= p->y && p->y < inst->y + inst->radius)) {
						active = false;
					}

					if (active) {
						if (inst->current_side == 0) {
							if (p->x >= inst->x) {
								p->layer = inst->layer_2;
							}
						} else if (inst->current_side == 1) {
							if (p->x < inst->x) {
								p->layer = inst->layer_1;
							}
						}
					}

					if (p->x >= inst->x) {
						inst->current_side = 1;
					} else {
						inst->current_side = 0;
					}
					break;
				}
			}
		}

		camera_x = (float) (int(p->x) - target_w / 2);
		camera_y = (float) (int(p->y) - target_h / 2);

		camera_x = clamp(camera_x, 0.0f, float(tilemap.width  * 16));
		camera_y = clamp(camera_y, 0.0f, float(tilemap.height * 16));
	} else {
		float spd = 10.0f;
		if (key[SDL_SCANCODE_LEFT])  {p->x -= spd * delta; camera_x -= spd * delta;}
		if (key[SDL_SCANCODE_RIGHT]) {p->x += spd * delta; camera_x += spd * delta;}
		if (key[SDL_SCANCODE_UP])    {p->y -= spd * delta; camera_y -= spd * delta;}
		if (key[SDL_SCANCODE_DOWN])  {p->y += spd * delta; camera_y += spd * delta;}

		if (key[SDL_SCANCODE_J]) {camera_x -= spd * delta;}
		if (key[SDL_SCANCODE_L]) {camera_x += spd * delta;}
		if (key[SDL_SCANCODE_I]) {camera_y -= spd * delta;}
		if (key[SDL_SCANCODE_K]) {camera_y += spd * delta;}

		p->xspeed = 0.0f;
		p->yspeed = 0.0f;
		p->ground_speed = 0.0f;
	}
}

#define SENSOR_A_COLOR 0, 240, 0, 255
#define SENSOR_B_COLOR 56, 255, 162, 255
#define SENSOR_C_COLOR 0, 174, 239, 255
#define SENSOR_D_COLOR 255, 242, 56, 255
#define SENSOR_E_COLOR 255, 56, 255, 255
#define SENSOR_F_COLOR 255, 84, 84, 255

void World::Draw(float delta) {
	const Uint8* key = SDL_GetKeyboardState(nullptr);

	// draw tilemap
	{
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
						DrawTextShadow(game->renderer, &fnt_cp437, "*", dest.x + 8, dest.y + 8, HALIGN_CENTER, VALIGN_MIDDLE);
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
	}

	Player* p = &player;

	// draw objects
	for (int i = 0; i < object_count; i++) {
		Object* inst = &objects[i];
		switch (inst->type) {
			case ObjType::VERTICAL_LAYER_SWITCHER: {
				SDL_SetRenderDrawColor(game->renderer, 128, 128, 255, 128);
				if (inst->current_side == 1) {
					SDL_Rect rect = {
						int(inst->x - 32.0f) - int(camera_x),
						int(inst->y - inst->radius) - int(camera_y),
						32,
						int(inst->radius * 2.0f)
					};
					SDL_RenderFillRect(game->renderer, &rect);
				} else if (inst->current_side == 0) {
					SDL_Rect rect = {
						int(inst->x) - int(camera_x),
						int(inst->y - inst->radius) - int(camera_y),
						32,
						int(inst->radius * 2.0f)
					};
					SDL_RenderFillRect(game->renderer, &rect);
				}
				break;
			}
		}
	}

	// draw player
	{
		SDL_Rect src = {
			p->frame_index * 66,
			0,
			66,
			66
		};
		SDL_Rect dest = {
			int(p->x) - int(camera_x) - 33,
			int(p->y) - int(camera_y) - 33,
			66,
			66
		};
		SDL_RendererFlip flip = (p->facing == 1) ? SDL_FLIP_NONE : SDL_FLIP_HORIZONTAL;
		double a = (double) -p->ground_angle; // round_to(-p->ground_angle, 45.0f);
		if (p->anim == anim_idle) {
			a = 0.0;
		}
		SDL_RenderCopyEx(game->renderer, anim_get_texture(p->anim), &src, &dest, a, nullptr, flip);
	}

	// draw player hitbox
	{
		SDL_Rect rect;
		switch (p->mode) {
			default:
			case PlayerMode::FLOOR:
			case PlayerMode::CEILING: {
				rect = {
					int(p->x - PLAYER_WIDTH_RADIUS)  - int(camera_x),
					int(p->y - PLAYER_HEIGHT_RADIUS) - int(camera_y),
					int(PLAYER_WIDTH_RADIUS  * 2.0f) + 1,
					int(PLAYER_HEIGHT_RADIUS * 2.0f) + 1
				};
				break;
			}
			case PlayerMode::RIGHT_WALL:
			case PlayerMode::LEFT_WALL: {
				rect = {
					int(p->x - PLAYER_HEIGHT_RADIUS) - int(camera_x),
					int(p->y - PLAYER_WIDTH_RADIUS)  - int(camera_y),
					int(PLAYER_HEIGHT_RADIUS * 2.0f) + 1,
					int(PLAYER_WIDTH_RADIUS  * 2.0f) + 1
				};
				break;
			}
		}
		SDL_SetRenderDrawColor(game->renderer, 128, 128, 255, 255);
		SDL_RenderDrawRect(game->renderer, &rect);
	}

	// draw sensors
	{
		if (AreGroundSensorsActive(p)) {
			float sensor_a_x;
			float sensor_a_y;
			float sensor_b_x;
			float sensor_b_y;
			GetGroundSensorsPositions(p, &sensor_a_x, &sensor_a_y, &sensor_b_x, &sensor_b_y);
			SDL_SetRenderDrawColor(game->renderer, SENSOR_A_COLOR);
			SDL_RenderDrawPoint(game->renderer,
								int(sensor_a_x) - int(camera_x),
								int(sensor_a_y) - int(camera_y));
			SDL_SetRenderDrawColor(game->renderer, SENSOR_B_COLOR);
			SDL_RenderDrawPoint(game->renderer,
								int(sensor_b_x) - int(camera_x),
								int(sensor_b_y) - int(camera_y));
		}

		if (IsPushSensorEActive(p)) {
			float sensor_e_x;
			float sensor_e_y;
			float sensor_f_x;
			float sensor_f_y;
			GetPushSensorsPositions(p, &sensor_e_x, &sensor_e_y, &sensor_f_x, &sensor_f_y);
			SDL_SetRenderDrawColor(game->renderer, SENSOR_E_COLOR);
			SDL_RenderDrawLine(game->renderer,
							   int(p->x) - int(camera_x),
							   int(sensor_e_y) - int(camera_y),
							   int(sensor_e_x) - int(camera_x),
							   int(sensor_e_y) - int(camera_y));
		}

		if (IsPushSensorFActive(p)) {
			float sensor_e_x;
			float sensor_e_y;
			float sensor_f_x;
			float sensor_f_y;
			GetPushSensorsPositions(p, &sensor_e_x, &sensor_e_y, &sensor_f_x, &sensor_f_y);
			SDL_SetRenderDrawColor(game->renderer, SENSOR_F_COLOR);
			SDL_RenderDrawLine(game->renderer,
							   int(p->x) - int(camera_x),
							   int(sensor_f_y) - int(camera_y),
							   int(sensor_f_x) - int(camera_x),
							   int(sensor_f_y) - int(camera_y));
		}
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

Object* World::CreateObject(ObjType type) {
	if (object_count == MAX_OBJECTS) {
		object_count--;
	}

	Object* result = &objects[object_count];

	*result = {};
	result->id = next_id++;
	result->type = type;
	object_count++;

	return result;
}
