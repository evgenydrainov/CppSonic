#pragma once

#include <SDL.h>
#include <stdint.h>

struct TileSet {
	uint8_t* tile_heights;
	uint8_t* tile_widths;
	float* tile_angles;

	int tile_count;
	int tiles_in_row = 16;
	uint8_t height_stub[16];

	SDL_Texture* texture;
	SDL_Texture* height_texture;
	SDL_Texture* width_texture;

	void LoadFromFile(const char* binary_filepath, const char* texture_filepath);
	void Destroy();

	uint8_t* GetTileHeight(int tile_index) {
		if (tile_index < tile_count) {
			return &tile_heights[tile_index * 16];
		}
		return height_stub;
	}

	uint8_t* GetTileWidth(int tile_index) {
		if (tile_index < tile_count) {
			return &tile_widths[tile_index * 16];
		}
		return height_stub;
	}

	float GetTileAngle(int tile_index) {
		if (tile_index < tile_count) {
			return tile_angles[tile_index];
		}
		return 0.0f;
	}

	SDL_Rect GetTextureSrcRect(int tile_index) {
		if (tile_index < tile_count) {
			int tile_x = tile_index % tiles_in_row;
			int tile_y = tile_index / tiles_in_row;

			// int u = tile_x * 16;
			// int v = tile_y * 16;

			int u = 1 + tile_x * 18;
			int v = 1 + tile_y * 18;

			return {u, v, 16, 16};
		}
		return {0, 0, 16, 16};
	}

	void GenCollisionTextures();
};
