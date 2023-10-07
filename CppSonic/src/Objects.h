#pragma once

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
	Animation* anim;
	Animation* next_anim;
	int frame_index;
	int frame_duration;
	int frame_timer;
	int facing = 1;
};
