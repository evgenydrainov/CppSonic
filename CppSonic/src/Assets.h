#pragma once

#include "Font.h"
#include <SDL.h>

#define LIST_OF_ALL_ANIMS \
	X(anim_idle,      1) \
	X(anim_walk,      6) \
	X(anim_run,       4) \
	X(anim_roll,      5) \
	X(anim_push,      4) \
	X(anim_look_up,   1) \
	X(anim_look_down, 1) \
	X(anim_balance,   2) \
	X(anim_skid,      2) \
	X(anim_bounce,    1)

typedef int anim_index;

#define X(name, _) name,
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
