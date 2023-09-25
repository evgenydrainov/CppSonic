#pragma once

#include <stddef.h>
#include <vector>

class TileMap {
public:
	struct Tile {
		size_t index;
		bool hflip;
		bool vflip;
		bool top_solid;
		bool left_right_bottom_solid;
	};

	~TileMap() { Clear(); }

	Tile GetTileA(int tile_x, int tile_y) {
		if (0 <= tile_x && tile_x < width && 0 <= tile_y && tile_y < height) {
			size_t id = (size_t) (tile_x + tile_y * width);
			return tiles_a[id];
		}
		return {};
	}

	Tile GetTileB(int tile_x, int tile_y) {
		if (0 <= tile_x && tile_x < width && 0 <= tile_y && tile_y < height) {
			size_t id = (size_t) (tile_x + tile_y * width);
			return tiles_b[id];
		}
		return {};
	}

	size_t TileCount() { return tiles_a.size(); }

	void Clear() {
		tiles_a.clear();
		tiles_b.clear();
	}

	void LoadFromFile(const char* fname);

	std::vector<Tile> tiles_a;
	std::vector<Tile> tiles_b;
	int width  = 0;
	int height = 0;
	float start_x = 0.0f;
	float start_y = 0.0f;
};
