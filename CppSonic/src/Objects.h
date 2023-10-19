#pragma once

#include "Assets.h"

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
	anim_index anim;
	anim_index next_anim;
	int frame_index;
	int frame_duration;
	int frame_timer;
	int facing = 1;
	int layer;
	float control_lock;
};

#define LIST_OF_ALL_OBJ_TYPES \
	X(VERTICAL_LAYER_SWITCHER)

#define X(name) name,
enum struct ObjType {
	LIST_OF_ALL_OBJ_TYPES
	COUNT
};
#undef X

typedef int instance_id;

enum {
	FLAG_LAYER_SWITCHER_GROUNDED_ONLY = 1 << 16
};

struct Object {
	instance_id id;
	ObjType type;
	uint32_t flags;
	float x;
	float y;

	union {
		struct { // Vertical Layer Switcher
			float radius;
			int layer_1;
			int layer_2;
			int current_side;
		};
	};
};
