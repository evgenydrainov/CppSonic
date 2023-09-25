#include "TileSet.h"

#include "Game.h"

#include <SDL_image.h>

void TileSet::LoadFromFile(const char* binary_filepath, const char* texture_filepath) {
	Clear();

	if (SDL_RWops* f = SDL_RWFromFile(binary_filepath, "rb")) {
		int tile_count;
		SDL_RWread(f, &tile_count, sizeof(tile_count), 1);

		tiles.resize(tile_count);

		SDL_RWread(f, tiles.data(), sizeof(*tiles.data()), tile_count);

		SDL_RWclose(f);
	}

	if (texture = IMG_LoadTexture(game->renderer, texture_filepath)) {
		GenCollisionTextures();
	}
}

void TileSet::GenCollisionTextures() {
	int texture_w;
	int texture_h;
	SDL_QueryTexture(texture, nullptr, nullptr, &texture_w, &texture_h);

	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, texture_w, texture_h, 32, SDL_PIXELFORMAT_ARGB8888);
	SDL_FillRect(surf, nullptr, 0x00000000);
	SDL_Surface* wsurf = SDL_CreateRGBSurfaceWithFormat(0, texture_w, texture_h, 32, SDL_PIXELFORMAT_ARGB8888);
	SDL_FillRect(wsurf, nullptr, 0x00000000);

	for (size_t tile_index = 0; tile_index < TileCount(); tile_index++) {
		uint8_t* height = GetTileHeight(tile_index);
		uint8_t* width  = GetTileWidth(tile_index);
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

	world->tileset.height_texture = SDL_CreateTextureFromSurface(game->renderer, surf);
	world->tileset.width_texture = SDL_CreateTextureFromSurface(game->renderer, wsurf);

	SDL_FreeSurface(surf);
	SDL_FreeSurface(wsurf);
}
