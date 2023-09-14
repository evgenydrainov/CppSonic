#include "World.h"

#include "Game.h"

#include <SDL_image.h>

#include <stdio.h>

void World::load_level_data() {
	size_t filesize;
	uint8_t* filedata = (uint8_t*) SDL_LoadFile("levels/ghz1.bin", &filesize);

	printf("levels/ghz1.bin filesize: %d\n", filesize);

	level_width = (int) (*filedata + 1);
	filedata++;

	level_height = (int) (*filedata + 1);
	filedata++;

	printf("width: %d\n", level_width);
	printf("height: %d\n", level_height);

	if (filesize != level_width * level_height + 2) {
		SDL_Log("Invalid level data");
		goto out;
	}

	level_data = (uint8_t*) malloc(level_width * level_height * sizeof(*level_data));

	if (!level_data) {
		goto out;
	}

	for (int i = 0; i < level_width * level_height; i++) {
		// if ((*filedata) & 0x80) printf("LOOP TILE\n");
		level_data[i] = (*filedata) & 0x7F;
		filedata++;

		printf("%d\n", (int) level_data[i]);
	}

	tex_ghz256 = IMG_LoadTexture(game->renderer, "ghz256.png");

	printf("\n");

out:
	SDL_free(filedata);
}

void World::load_chunk_data() {
	size_t filesize;
	uint16_t* filedata = (uint16_t*) SDL_LoadFile("map256/GHZ.bin", &filesize);

	printf("map256/GHZ.bin filesize: %d\n", filesize);

	printf("filesize %% (2 * 256) %d\n", filesize % (2 * 256));
	printf("filesize / (2 * 256) %d\n", filesize / (2 * 256));

	// int chunk_count = filesize / (2 * 256);

	level_chunk_data = (uint16_t*) malloc(filesize);

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

void World::load_collision_array() {
	size_t filesize;
	uint8_t* filedata = (uint8_t*) SDL_LoadFile("collide/Collision Array (Normal).bin", &filesize);

	printf("collide/Collision Array (Normal).bin filesize: %d\n", filesize);

	collision_array = filedata;

	for (int i = 0; i < 16; i++) {
		printf("%d\n", collision_array[i]);
	}

	printf("\n");

	// SDL_free(filedata);
}

void World::load_collision_indicies() {
	size_t filesize;
	uint8_t* filedata = (uint8_t*) SDL_LoadFile("collide/GHZ.bin", &filesize);

	printf("collide/GHZ.bin filesize: %d\n", filesize);

	level_col_indicies = filedata;

	for (int i = 0; i < 16; i++) printf("%d ", level_col_indicies[i]);
	printf("\n");

	printf("\n");

// out:
// 	SDL_free(filedata);
}

// void World::load_collision_array() {}

void World::Init() {
	load_level_data();
	load_chunk_data();
	load_collision_array();
	load_collision_indicies();
}

void World::Quit() {
	SDL_free(collision_array);
	SDL_free(level_col_indicies);
	free(level_chunk_data);
	free(level_data);
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

	// printf("%d\n", t);
}
