#include "Game.h"

#include "misc.h"
#include "mathh.h"
#include "stb_sprintf.h"

#include <SDL_image.h>
#include <string.h> // for memset

Game* game;

void Game::Init() {
	SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);

	if (SDL_Init(SDL_INIT_VIDEO
				 | SDL_INIT_AUDIO) != 0) {
		char buf[256];
		stb_snprintf(buf, sizeof(buf), "SDL Couldn't initialize: %s", SDL_GetError());
		SDL_ShowSimpleMessageBox(0, "ERROR", buf, nullptr);
		exit(1);
	}

	IMG_Init(IMG_INIT_PNG);

	window = SDL_CreateWindow("CppSonic",
							  SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
							  GAME_W, GAME_H,
							  SDL_WINDOW_RESIZABLE);

	renderer = SDL_CreateRenderer(window, -1, 0);

	SDL_SetRenderDrawBlendMode(renderer, SDL_BLENDMODE_BLEND);

	game_texture = SDL_CreateTexture(renderer,
									 SDL_PIXELFORMAT_ARGB8888,
									 SDL_TEXTUREACCESS_TARGET,
									 GAME_W, GAME_H);

	load_all_assets();

	world = &world_instance;
	world->Init();
}

void Game::Quit() {
	switch (state) {
		case GameState::PLAYING: world->Quit(); break;
	}

	free_all_assets();

	SDL_DestroyTexture(game_texture);
	SDL_DestroyRenderer(renderer);
	SDL_DestroyWindow(window);

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
	
	fps = 1.0 / (t - prev_time);
	prev_time = t;

	double frame_end_time = t + (1.0 / double(GAME_FPS));

	memset(key_pressed, 0, sizeof(key_pressed));
	skip_frame = frame_advance;

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

					switch (scancode) {
						case SDL_SCANCODE_F5: {
							frame_advance = true;
							skip_frame = false;
							break;
						}

						case SDL_SCANCODE_F6: {
							frame_advance = false;
							break;
						}
					}
					break;
				}
			}
		}
	}

	{
		float delta = 60.0f / float(GAME_FPS);

		Update(delta);

		Draw(delta);
	}

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
	double t = GetTime();

	switch (state) {
		case GameState::PLAYING: {
			if (!skip_frame) {
				world->Update(delta);
			}

			if (key_pressed[SDL_SCANCODE_ESCAPE]) {
				world->debug ^= true;
			}
			break;
		}
	}

	update_took = (GetTime() - t) * 1000.0;
}

void Game::Draw(float delta) {
	double t = GetTime();

	// draw game to game texture
	SDL_SetRenderTarget(renderer, game_texture);
	{
		SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
		SDL_RenderClear(renderer);

		switch (state) {
			case GameState::PLAYING: world->Draw(delta); break;
		}
	}

	// draw game texture and other stuff to screen
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

		{
			char buf[100];
			stb_snprintf(buf,
						 sizeof(buf),
						 "%ffps\n"
						 "update: %fms\n"
						 "draw: %fms\n"
						 "\n",
						 fps,
						 update_took,
						 draw_took);
			draw_y = DrawTextShadow(renderer, &fnt_cp437, buf, draw_x, draw_y).y;
		}

		switch (state) {
			case GameState::PLAYING: {
				float x = mouse_x + world->camera_x;
				float y = mouse_y + world->camera_y;
				int tile_x = int(x) / 16;
				int tile_y = int(y) / 16;
				tile_x = clamp(tile_x, 0, world->tilemap.width  - 1);
				tile_y = clamp(tile_y, 0, world->tilemap.height - 1);

				Tile tile = world->tilemap.GetTileA(tile_x, tile_y);
				uint8_t* height = world->tileset.GetTileHeight(tile.index);
				float angle = world->tileset.GetTileAngle(tile.index);

				char buf[200];
				stb_snprintf(buf,
							 sizeof(buf),
							 "x: %f\n"
							 "y: %f\n"
							 "xspeed: %f\n"
							 "yspeed: %f\n"
							 "ground speed: %f\n"
							 "ground angle: %f\n"
							 "%s\n"
							 "\n",
							 world->player.x,
							 world->player.y,
							 world->player.xspeed,
							 world->player.yspeed,
							 world->player.ground_speed,
							 world->player.ground_angle,
							 anim_get_name(world->player.anim));
				draw_y = DrawTextShadow(renderer, &fnt_cp437, buf, draw_x, draw_y, 0, 0, {220, 220, 220, 255}).y;

				if (world->debug) {
					char buf[200];
					stb_snprintf(buf,
								 sizeof(buf),
								 "mouse x: %d\n"
								 "mouse y: %d\n"
								 "mouse tile: %d\n"
								 "angle: %f\n"
								 "hflip: %d vflip: %d\n"
								 "top solid: %d\n"
								 "left right bottom solid: %d\n",
								 tile_x,
								 tile_y,
								 tile.index,
								 angle,
								 tile.hflip, tile.vflip,
								 tile.top_solid,
								 tile.left_right_bottom_solid);
					draw_y = DrawTextShadow(renderer, &fnt_cp437, buf, draw_x, draw_y, 0, 0, {220, 220, 220, 255}).y;

					if (height) {
						char buf[100];
						stb_snprintf(buf,
									 sizeof(buf),
									 "%02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x %02x \n"
									 "\n",
									 height[ 0], height[ 1], height[ 2], height[ 3], height[ 4], height[ 5], height[ 6], height[ 7],
									 height[ 8], height[ 9], height[10], height[11], height[12], height[13], height[14], height[15]);
						draw_y = DrawTextShadow(renderer, &fnt_cp437, buf, draw_x, draw_y, 0, 0, {220, 220, 220, 255}).y;
					}
				}

				break;
			}
		}
	}

	SDL_RenderPresent(renderer);

	draw_took = (GetTime() - t) * 1000.0;
}
