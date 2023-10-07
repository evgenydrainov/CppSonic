#pragma once

#include <stdint.h>
#include <vector>
#include <SDL.h>

class TileSet {
public:
	struct Tile {
		uint8_t height[16];
		uint8_t width[16];
		float angle;
	};

	~TileSet() { Clear(); }

	Tile GetTile(uint32_t tile_index) {
		if (tile_index < tiles.size()) {
			return tiles[tile_index];
		}
		return {};
	}

	uint8_t* GetTileHeight(uint32_t tile_index) {
		if (tile_index < tiles.size()) {
			return tiles[tile_index].height;
		}
		return height_stub;
	}

	uint8_t* GetTileWidth(uint32_t tile_index) {
		if (tile_index < tiles.size()) {
			return tiles[tile_index].width;
		}
		return height_stub;
	}

	float GetTileAngle(uint32_t tile_index) {
		if (tile_index < tiles.size()) {
			return tiles[tile_index].angle;
		}
		return 0.0f;
	}

	SDL_Rect GetTextureSrcRect(uint32_t tile_index) {
		if (tile_index < tiles.size()) {
			int tile_x = int(tile_index) % tiles_in_row;
			int tile_y = int(tile_index) / tiles_in_row;
			return {tile_x * 16, tile_y * 16, 16, 16};
		}
		return {0, 0, 16, 16};
	}

	size_t TileCount() { return tiles.size(); }

	void Clear() {
		tiles.clear();

		if (texture) SDL_DestroyTexture(texture);
		texture = nullptr;

		if (height_texture) SDL_DestroyTexture(height_texture);
		height_texture = nullptr;

		if (width_texture) SDL_DestroyTexture(width_texture);
		width_texture = nullptr;
	}

	void LoadFromFile(const char* binary_filepath, const char* texture_filepath);

	void GenCollisionTextures();

	std::vector<Tile> tiles;
	uint8_t height_stub[16] = {};
	SDL_Texture* texture        = nullptr;
	SDL_Texture* height_texture = nullptr;
	SDL_Texture* width_texture  = nullptr;
	int tiles_in_row = 16;
};
