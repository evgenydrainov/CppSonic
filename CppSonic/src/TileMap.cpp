#include "TileMap.h"

#include <SDL.h>
#include "misc.h"

void TileMap::LoadFromFile(const char* fname) {
	SDL_RWops* f = nullptr;

	{
		f = SDL_RWFromFile(fname, "rb");

		if (!f) {
			ErrorMessageBox("Couldn't open tilemap.");
			goto out;
		}

		int width;
		int height;
		SDL_RWread(f, &width,  sizeof(width),  1);
		SDL_RWread(f, &height, sizeof(height), 1);

		if (width <= 0 || height <= 0) {
			ErrorMessageBox("Tilemap size is invalid.");
			goto out;
		}

		float start_x;
		float start_y;
		SDL_RWread(f, &start_x, sizeof(start_x), 1);
		SDL_RWread(f, &start_y, sizeof(start_y), 1);

		if (start_x < 0.0f || start_y < 0.0f) {
			ErrorMessageBox("Start position is invalid.");
		}

		this->width  = width;
		this->height = height;
		this->tile_count = width * height;

		this->start_x = start_x;
		this->start_y = start_y;

		tiles_a = (Tile*) ecalloc(tile_count, sizeof(*tiles_a));
		tiles_b = (Tile*) ecalloc(tile_count, sizeof(*tiles_b));

		SDL_RWread(f, tiles_a, sizeof(*tiles_a), tile_count);
		SDL_RWread(f, tiles_b, sizeof(*tiles_b), tile_count);
	}

out:
	if (f) SDL_RWclose(f);
}

void TileMap::Destroy() {
	if (tiles_b) free(tiles_b);
	tiles_b = nullptr;

	if (tiles_a) free(tiles_a);
	tiles_a = nullptr;
}
