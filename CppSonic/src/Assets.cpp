#include "Assets.h"

#include "Game.h"
#include <SDL_image.h>
#include <SDL_ttf.h>

#define X(_, frame_count) frame_count,
static int anim_frame_count[ANIM_COUNT] = {
	LIST_OF_ALL_ANIMS
};
#undef X

static SDL_Texture* anim_textures[ANIM_COUNT];

Font fnt_cp437;

static bool load_anim(anim_index anim, const char* fname) {
	SDL_Texture* texture = IMG_LoadTexture(game->renderer, fname);

	if (!texture) {
		return false;
	}

	anim_textures[anim] = texture;

	return true;
}

bool load_all_assets() {
	bool error = false;

	#define X(name, _) if (!load_anim(name, "anim/" #name ".png")) error = true;
	LIST_OF_ALL_ANIMS
	#undef X

	TTF_Init();
	{
		LoadFontFromFileTTF(&fnt_cp437, "PerfectDOSVGA437Win.ttf", 16, game->renderer);
		if (!fnt_cp437.texture) error = true;
	}
	TTF_Quit();

	return !error;
}

void free_all_assets() {
	for (int i = 0; i < ANIM_COUNT; i++) {
		SDL_DestroyTexture(anim_textures[i]);
	}

	DestroyFont(&fnt_cp437);
}

int anim_get_frame_count(anim_index anim) {
	if (0 <= anim && anim < ANIM_COUNT) {
		return anim_frame_count[anim];
	}
	return 0;
}

SDL_Texture* anim_get_texture(anim_index anim) {
	if (0 <= anim && anim < ANIM_COUNT) {
		return anim_textures[anim];
	}
	return nullptr;
}
