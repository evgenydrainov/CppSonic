#include "TileSet.h"

#include "Game.h"

#include <SDL_image.h>
#include "misc.h"

void TileSet::LoadFromFile(const char* binary_filepath, const char* texture_filepath) {
	auto load_binary = [this](const char* binary_filepath) {
		SDL_RWops* f = nullptr;

		{
			f = SDL_RWFromFile(binary_filepath, "rb");

			if (!f) {
				ErrorMessageBox("Couldn't open tileset.");
				goto out;
			}

			int tile_count;
			SDL_RWread(f, &tile_count, sizeof(tile_count), 1);

			if (tile_count <= 0) {
				ErrorMessageBox("Invalid tileset size.");
				goto out;
			}

			this->tile_count = tile_count;

			tile_heights = (uint8_t*) ecalloc(tile_count, sizeof(*tile_heights) * 16);
			tile_widths  = (uint8_t*) ecalloc(tile_count, sizeof(*tile_widths)  * 16);
			tile_angles  = (float*)   ecalloc(tile_count, sizeof(*tile_angles));

			SDL_RWread(f, tile_heights, 16 * sizeof(*tile_heights), tile_count);
			SDL_RWread(f, tile_widths,  16 * sizeof(*tile_widths), tile_count);
			SDL_RWread(f, tile_angles,  sizeof(*tile_angles), tile_count);
		}

	out:
		if (f) SDL_RWclose(f);
	};

	auto load_texture = [this](const char* texture_filepath) {
		texture = IMG_LoadTexture(game->renderer, texture_filepath);

		if (!texture) {
			ErrorMessageBox("Couldn't load tileset texture.");
			return;
		}

		int w;
		int h;
		SDL_QueryTexture(texture, nullptr, nullptr, &w, &h);

		// tiles_in_row = w / 16;
	};

	load_binary(binary_filepath);
	load_texture(texture_filepath);

	if (texture) {
		GenCollisionTextures();
	}
}

void TileSet::Destroy() {
	if (width_texture) SDL_DestroyTexture(width_texture);
	width_texture = nullptr;

	if (height_texture) SDL_DestroyTexture(height_texture);
	height_texture = nullptr;

	if (texture) SDL_DestroyTexture(texture);
	texture = nullptr;

	if (tile_angles) free(tile_angles);
	tile_angles = nullptr;

	if (tile_widths) free(tile_widths);
	tile_widths = nullptr;

	if (tile_heights) free(tile_heights);
	tile_heights = nullptr;
}

void TileSet::GenCollisionTextures() {
	int texture_w;
	int texture_h;
	SDL_QueryTexture(texture, nullptr, nullptr, &texture_w, &texture_h);

	SDL_Surface* surf = SDL_CreateRGBSurfaceWithFormat(0, texture_w, texture_h, 32, SDL_PIXELFORMAT_ARGB8888);
	SDL_FillRect(surf, nullptr, 0x00000000);
	SDL_Surface* wsurf = SDL_CreateRGBSurfaceWithFormat(0, texture_w, texture_h, 32, SDL_PIXELFORMAT_ARGB8888);
	SDL_FillRect(wsurf, nullptr, 0x00000000);

	for (int tile_index = 0; tile_index < tile_count; tile_index++) {
		uint8_t* height = GetTileHeight(tile_index);
		uint8_t* width  = GetTileWidth(tile_index);
		SDL_Rect src = GetTextureSrcRect(tile_index);
		for (int i = 0; i < 16; i++) {
			if (height[i] != 0) {
				if (height[i] <= 0x10) {
					SDL_Rect line = {
						src.x + i,
						src.y + 16 - height[i],
						1,
						height[i]
					};
					SDL_FillRect(surf, &line, 0xffffffff);
				} else if (height[i] >= 0xF0) {
					SDL_Rect line = {
						src.x + i,
						src.y,
						1,
						16 - (height[i] - 0xF0)
					};
					SDL_FillRect(surf, &line, 0xffff0000);
				}
			}

			if (width[i] != 0) {
				if (width[i] <= 0x10) {
					SDL_Rect line = {
						src.x + 16 - width[i],
						src.y + i,
						width[i],
						1
					};
					SDL_FillRect(wsurf, &line, 0xffffffff);
				} else {
					SDL_Rect line = {
						src.x,
						src.y + i,
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
