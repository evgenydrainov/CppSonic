#pragma once

struct Tile {
	int index;
	bool hflip : 1;
	bool vflip : 1;
	bool top_solid : 1;
	bool left_right_bottom_solid : 1;
};

struct TileMap {
	Tile* tiles_a;
	Tile* tiles_b;
	int tile_count;
	int width;
	int height;
	float start_x;
	float start_y;

	void LoadFromFile(const char* fname);
	void Destroy();

	Tile GetTileA(int tile_x, int tile_y) {
		if (0 <= tile_x && tile_x < width && 0 <= tile_y && tile_y < height) {
			int index = tile_x + tile_y * width;
			return tiles_a[index];
		}
		return {};
	}

	Tile GetTileB(int tile_x, int tile_y) {
		if (0 <= tile_x && tile_x < width && 0 <= tile_y && tile_y < height) {
			int index = tile_x + tile_y * width;
			return tiles_b[index];
		}
		return {};
	}

	Tile GetTile(int tile_x, int tile_y, int layer) {
		if (layer == 0) {
			return GetTileA(tile_x, tile_y);
		} else if (layer == 1) {
			return GetTileB(tile_x, tile_y);
		}
		return {};
	}
};
