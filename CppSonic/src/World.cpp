#include "World.h"

#include "Game.h"

#include <SDL_image.h>

#include <stdio.h>

void World::load_sonic1_genesis_level() {
	tex_ghz16 = IMG_LoadTexture(game->renderer, "ghz16.png");

	if (!tex_ghz16) {
		return;
	}

	uint8_t* level_data;
	uint16_t* level_chunk_data;
	uint8_t* collision_array;
	uint8_t* level_col_indicies;
	size_t level_col_indicies_count;
	int level_width;
	int level_height;

	{
		size_t filesize;
		uint8_t* filedata = (uint8_t*) SDL_LoadFile("levels/ghz1.bin", &filesize);

		if (!filedata) {
			SDL_Log("couldn't open levels/ghz1.bin");
			return;
		}

		printf("levels/ghz1.bin filesize: %zu\n", filesize);

		if (filesize == 0) {
			SDL_Log("levels/ghz1.bin is empty");
			SDL_free(filedata);
			return;
		}

		level_width = (int)(*filedata) + 1;
		filedata++;

		level_height = (int)(*filedata) + 1;
		filedata++;

		printf("width: %d\n", level_width);
		printf("height: %d\n", level_height);

		if (level_width <= 0 || level_height <= 0) {
			SDL_Log("levels/ghz1.bin is corrupted");
			SDL_free(filedata);
			return;
		}

		if (filesize != level_width * level_height + 2) {
			SDL_Log("levels/ghz1.bin is corrupted");
			SDL_free(filedata);
			return;
		}

		level_data = (uint8_t*) malloc(level_width * level_height * sizeof(*level_data));

		if (!level_data) {
			SDL_Log("Out of memory");
			SDL_free(filedata);
			return;
		}

		for (int i = 0; i < level_width * level_height; i++) {
			// if ((*filedata) & 0x80) printf("LOOP TILE\n");
			level_data[i] = (*filedata) & 0x7F;
			filedata++;

			// printf("%d\n", (int) level_data[i]);
		}

		printf("\n");
	}
	
	{
		size_t filesize;
		uint16_t* filedata = (uint16_t*) SDL_LoadFile("map256/GHZ.bin", &filesize);

		if (!filedata) {
			SDL_Log("Couldn't open map256/GHZ.bin");
			return;
		}

		printf("map256/GHZ.bin filesize: %zu\n", filesize);

		if (filesize == 0) {
			SDL_Log("map256/GHZ.bin is empty");
			SDL_free(filedata);
			return;
		}

		printf("filesize %% (2 * 256) %zu\n", filesize % (2 * 256));
		printf("filesize / (2 * 256) %zu\n", filesize / (2 * 256));

		// int chunk_count = filesize / (2 * 256);

		level_chunk_data = (uint16_t*) malloc(filesize);

		if (!level_chunk_data) {
			SDL_Log("Out of memory");
			SDL_free(filedata);
			return;
		}

		for (int i = 0; i < filesize / 2; i++) {
			level_chunk_data[i] = *filedata;
			level_chunk_data[i] = (level_chunk_data[i] << 8) | (level_chunk_data[i] >> 8);
			level_chunk_data[i] &= 0x3FF;
			filedata++;
		}

		for (int i = 0; i < 16; i++) {
			printf("%d\n", level_chunk_data[256 + 240 + i]);
		}

		printf("\n");

		SDL_free(filedata);
	}
	
	{
		size_t filesize;
		uint8_t* filedata = (uint8_t*) SDL_LoadFile("collide/Collision Array (Normal).bin", &filesize);

		if (!filedata) {
			SDL_Log("Couldn't open collide/Collision Array (Normal).bin");
			return;
		}

		printf("collide/Collision Array (Normal).bin filesize: %d\n", filesize);

		if (filesize == 0) {
			SDL_Log("collide/Collision Array (Normal).bin is empty");
			SDL_free(filedata);
			return;
		}

		collision_array = filedata;

		for (int i = 0; i < 16; i++) {
			printf("%d\n", collision_array[i]);
		}

		printf("\n");
	}
	
	{
		size_t filesize;
		uint8_t* filedata = (uint8_t*) SDL_LoadFile("collide/GHZ.bin", &filesize);

		if (!filedata) {
			SDL_Log("Couldn't open collide/GHZ.bin");
			return;
		}

		printf("collide/GHZ.bin filesize: %d\n", filesize);

		if (filesize == 0) {
			SDL_Log("collide/GHZ.bin is empty");
			SDL_free(filedata);
			return;
		}

		level_col_indicies = filedata;
		level_col_indicies_count = filesize;

		for (int i = 0; i < 16; i++) printf("%d ", level_col_indicies[i]);
		printf("\n");

		printf("\n");
	}

	tiles = (Tile*) malloc(level_width * level_height * 256 * sizeof(*tiles));
	heights = (TileHeight*) malloc(level_col_indicies_count * sizeof(*heights));

	for (int y = 0; y < level_height; y++) {
		for (int x = 0; x < level_width; x++) {
			int i = x + y * level_width;

			if (level_data[i] == 0) {
				continue;
			}

			uint16_t* chunk = level_chunk_data + ((level_data[i] - 1) * 256);
			int xx = x;
			int yy = y;
			for (int y = 0; y < 16; y++) {
				for (int x = 0; x < 16; x++) {
					uint16_t tile = chunk[x + y * 16];
					if (tile >= 410) {
						continue;
					}
					uint8_t index = level_col_indicies[tile];
					// uint8_t index = 20;
					uint8_t* heights = collision_array + index * 16;

					int tile_index = xx * 16 + x + (yy * 16 * level_width * 16) + (y * 16 * level_width);

					tiles[tile_index].index = tile;

					for (int i = 0; i < 16; i++) {
						int drawx = xx * 256 + x * 16 + i - (int)camera_x;
						int drawy = yy * 256 + y * 16 - (int)camera_y;

						// this->heights[tile_index].height[i] = heights[i];
					}
				}
			}
		}
	}

	this->level_width = level_width * 16;
	this->level_height = level_height * 16;

	SDL_free(collision_array);
	SDL_free(level_col_indicies);
	free(level_chunk_data);
	free(level_data);
}

void World::Init() {
	load_sonic1_genesis_level();
}

void World::Quit() {
	
}

void World::Update(float delta) {
	const Uint8* key = SDL_GetKeyboardState(nullptr);

	if (key[SDL_SCANCODE_LEFT])  camera_x -= 20.0f * delta;
	if (key[SDL_SCANCODE_RIGHT]) camera_x += 20.0f * delta;
	if (key[SDL_SCANCODE_UP])    camera_y -= 20.0f * delta;
	if (key[SDL_SCANCODE_DOWN])  camera_y += 20.0f * delta;
}

void World::Draw(float delta) {
	// uint16_t t = 0;

	/*
	for (int y = 0; y < level_height; y++) {
		for (int x = 0; x < level_width; x++) {
			int i = x + y * level_width;

			if (level_data[i] == 0) {
				continue;
			}

			SDL_Rect src = {
				(((level_data[i]) & 0x7F) % 8) * 256,
				(((level_data[i]) & 0x7F) / 8) * 256,
				256,
				256
			};

			SDL_Rect dest = {
				x * 256 - (int)camera_x,
				y * 256 - (int)camera_y,
				256,
				256
			};

			SDL_RenderCopy(game->renderer, tex_ghz256, &src, &dest);

			if (level_data[i] - 1 >= 82) {
				printf("%d\n", level_data[i] - 1);
				continue;
			}

			uint16_t* chunk = level_chunk_data + ((level_data[i] - 1) * 256);
			int xx = x;
			int yy = y;
			for (int y = 0; y < 16; y++) {
				for (int x = 0; x < 16; x++) {
					uint16_t tile = chunk[x + y * 16];
					if (tile >= 410) {
						continue;
					}
					uint8_t index = level_col_indicies[tile];
					// uint8_t index = 20;
					uint8_t* heights = collision_array + index * 16;
					if (index == 255) {
						SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
						SDL_Rect rect = {
							xx * 256 + x * 16 - (int)camera_x,
							yy * 256 + y * 16 - (int)camera_y,
							16,
							16
						};
						SDL_RenderFillRect(game->renderer, &rect);
						continue;
					}
					for (int i = 0; i < 16; i++) {
						if (heights[i] == 0) {
							continue;
						}
						if (heights[i] > 0x10) {
							continue;
						}
						SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
						int drawx = xx * 256 + x * 16 + i - (int)camera_x;
						int drawy = yy * 256 + y * 16 - (int)camera_y;
						if (drawx < 0 || drawy < 0) continue;
						if (drawx > 1920 || drawy > 1080) continue;
						SDL_RenderDrawLine(
							game->renderer,
							drawx,
							drawy + 16 - heights[i],
							drawx,
							drawy + 16
						);

						// t = index;
					}
				}
			}
		}
	}
	*/

	for (int y = 0; y < level_height; y++) {
		for (int x = 0; x < level_width; x++) {
			Tile* tile = &tiles[x + y * level_width];

			SDL_Rect src = {
				(tile->index % 16) * 16,
				(tile->index / 16) * 16,
				16,
				16
			};

			SDL_Rect dest = {
				x * 16,
				y * 16,
				16,
				16
			};

			SDL_RenderCopy(game->renderer, tex_ghz16, &src, &dest);
		}
	}

	// printf("%d\n", t);
}
