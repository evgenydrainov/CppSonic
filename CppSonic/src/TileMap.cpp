#include "TileMap.h"

#include <SDL.h>

void TileMap::LoadFromFile(const char* fname) {
	Clear();

	if (SDL_RWops* f = SDL_RWFromFile(fname, "rb")) {
		int width;
		int height;
		SDL_RWread(f, &width,  sizeof(width),  1);
		SDL_RWread(f, &height, sizeof(height), 1);

		float start_x;
		float start_y;
		SDL_RWread(f, &start_x, sizeof(start_x), 1);
		SDL_RWread(f, &start_y, sizeof(start_y), 1);

		this->width  = width;
		this->height = height;

		this->start_x = start_x;
		this->start_y = start_y;

		tiles_a.resize(width * height);
		tiles_b.resize(width * height);

		SDL_RWread(f, tiles_a.data(), sizeof(*tiles_a.data()), width * height);
		SDL_RWread(f, tiles_b.data(), sizeof(*tiles_b.data()), width * height);

		SDL_RWclose(f);
	}
}
