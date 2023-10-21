#pragma once

#include "Font.h"
#include <SDL.h>

#define LIST_OF_ALL_ANIMS \
	X(anim_idle,       1, 66, 66) \
	X(anim_walk,       6, 66, 66) \
	X(anim_run,        4, 66, 66) \
	X(anim_roll,       8, 66, 66) \
	X(anim_push,       4, 66, 66) \
	X(anim_look_up,    1, 66, 66) \
	X(anim_crouch,     1, 66, 66) \
	X(anim_balance,    2, 66, 66) \
	X(anim_skid,       2, 66, 66) \
	X(anim_bounce,     1, 66, 66) \
	X(anim_spindash,  10, 59, 59)

// #define LIST_OF_ALL_ANIMS \
// 	X(anim_idle,       1, 59, 59) \
// 	X(anim_walk,       6, 59, 59) \
// 	X(anim_run,        4, 59, 59) \
// 	X(anim_roll,       8, 59, 59) \
// 	X(anim_look_up,    1, 59, 59) \
// 	X(anim_crouch,     1, 59, 59) \
// 	X(anim_skid,       2, 59, 59) \
// 	X(anim_spindash,  10, 59, 59)

typedef int anim_index;

#define X(name, _1, _2, _3) name,
enum {
	LIST_OF_ALL_ANIMS
	ANIM_COUNT
};
#undef X

extern Font fnt_cp437;

bool load_all_assets();
void free_all_assets();

int anim_get_frame_count(anim_index anim);
SDL_Texture* anim_get_texture(anim_index anim);
const char* anim_get_name(anim_index anim);
int anim_get_width(anim_index anim);
int anim_get_height(anim_index anim);
