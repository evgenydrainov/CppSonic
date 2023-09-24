#include "Game.h"

#include <SDL_image.h>
#include <SDL_ttf.h>
#include <string.h>
#include <math.h>

#include "misc.h"
#include "stb_sprintf.h"
#include "mathh.h"

Game* game;
World* world;

Game::Game() {
	game = this;
}

void Game::Init() {
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

	SDL_Init(SDL_INIT_VIDEO
			 | SDL_INIT_AUDIO);
	IMG_Init(IMG_INIT_PNG);
	TTF_Init();
	Mix_Init(0);

	window = SDL_CreateWindow("CppSonic",
							  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							  GAME_W, GAME_H,
							  SDL_WINDOW_RESIZABLE);

	renderer = SDL_CreateRenderer(window, -1, 0);

	game_texture = SDL_CreateTexture(renderer,
									 SDL_PIXELFORMAT_ARGB8888,
									 SDL_TEXTUREACCESS_TARGET,
									 GAME_W, GAME_H);

	LoadFont(&fnt_CP437, "PerfectDOSVGA437Win.ttf", 16);

	std::get<0>(state).Init();
}

void Game::Quit() {
	switch (state.index()) {
		case 0: std::get<0>(state).Quit(); break;
	}

	DestroyFont(&fnt_CP437);

	SDL_DestroyTexture(game_texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

	Mix_Quit();
	TTF_Quit();
	IMG_Quit();
	SDL_Quit();
}

void Game::Run() {
	while (!quit) {
		Frame();
	}
}

void Game::Frame() {
	double t = GetTime();

	double frame_end_time = t + (1.0 / (double)GAME_FPS);

	memset(&key_pressed, 0, sizeof(key_pressed));

	{
		SDL_Event ev;
		while (SDL_PollEvent(&ev)) {
			switch (ev.type) {
				case SDL_QUIT: {
					quit = true;
					break;
				}

				case SDL_KEYDOWN: {
					SDL_Scancode scancode = ev.key.keysym.scancode;
					if (0 <= scancode && scancode < ArrayLength(key_pressed)) {
						key_pressed[scancode] = true;
					}
					break;
				}
			}
		}
	}

	float delta = 60.0f / (float)GAME_FPS;

	Update(delta);

	Draw(delta);

#ifndef __EMSCRIPTEN__
	t = GetTime();
	double time_left = frame_end_time - t;
	if (time_left > 0.0) {
		SDL_Delay((Uint32) (time_left * (0.95 * 1000.0)));
		while (GetTime() < frame_end_time) {}
	}
#endif
}

void Game::Update(float delta) {
	switch (state.index()) {
		case 0: std::get<0>(state).Update(delta); break;
	}
}

void Game::Draw(float delta) {
	SDL_SetRenderTarget(renderer, game_texture);
	{
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		switch (state.index()) {
			case 0: std::get<0>(state).Draw(delta); break;
		}
	}

	SDL_SetRenderTarget(renderer, nullptr);
	{
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		SDL_RenderSetLogicalSize(renderer, GAME_W, GAME_H);
		{
			SDL_RenderCopy(renderer, game_texture, nullptr, nullptr);

			int mouse_x_window;
			int mouse_y_window;
			SDL_GetMouseState(&mouse_x_window, &mouse_y_window);
			SDL_RenderWindowToLogical(renderer, mouse_x_window, mouse_y_window, &mouse_x, &mouse_y);
		}
		SDL_RenderSetLogicalSize(renderer, 0, 0);

		int draw_x = 0;
		int draw_y = 0;

		switch (state.index()) {
			case (size_t) GameState::PLAYING: {
				float x = mouse_x + world->camera_x;
				float y = mouse_y + world->camera_y;
				int tile_x = (int)x / 16;
				int tile_y = (int)y / 16;
				tile_x = clamp(tile_x, 0, world->level_width  - 1);
				tile_y = clamp(tile_y, 0, world->level_height - 1);

				Tile tile = world->get_tile(tile_x, tile_y);
				uint8_t* height = world->get_height(tile.index);
				float angle = world->get_angle(tile.index);

				char buf[200];
				stb_snprintf(buf,
							 sizeof(buf),
							 "x: %f\n"
							 "y: %f\n"
							 "xspeed: %f\n"
							 "yspeed: %f\n"
							 "ground speed: %f\n"
							 "ground angle: %f\n",
							 world->player.x,
							 world->player.y,
							 world->player.xspeed,
							 world->player.yspeed,
							 world->player.ground_speed,
							 world->player.ground_angle);
				draw_y = DrawTextShadow(&fnt_CP437, buf, draw_x, draw_y, 0, 0, {220, 220, 220, 255}).y;

				if (world->debug) {
					char buf[100];
					stb_snprintf(buf,
								 sizeof(buf),
								 "mouse x: %d\n"
								 "mouse y: %d\n"
								 "mouse tile: %d\n"
								 "angle: %f\n"
								 "hflip: %d vflip: %d\n",
								 tile_x,
								 tile_y,
								 tile.index,
								 angle,
								 tile.hflip, tile.vflip);
					draw_y = DrawTextShadow(&fnt_CP437, buf, draw_x, draw_y, 0, 0, {220, 220, 220, 255}).y;

					if (height) {
						char buf[100];
						stb_snprintf(buf,
									 sizeof(buf),
									 "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \n",
									 height[ 0], height[ 1], height[ 2], height[ 3], height[ 4], height[ 5], height[ 6], height[ 7],
									 height[ 8], height[ 9], height[10], height[11], height[12], height[13], height[14], height[15]);
						draw_y = DrawTextShadow(&fnt_CP437, buf, draw_x, draw_y, 0, 0, {220, 220, 220, 255}).y;
					}
				}

				break;
			}
		}
	}

	SDL_RenderPresent(renderer);
}
