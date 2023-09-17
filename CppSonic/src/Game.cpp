#include "Game.h"

#include <SDL_image.h>
#include <SDL_ttf.h>

#include "misc.h"
#include <string.h>

Game* game = nullptr;
World* world = nullptr;

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

	renderer = SDL_CreateRenderer(window, -1,
								  SDL_RENDERER_ACCELERATED
								  | SDL_RENDERER_TARGETTEXTURE);

	SDL_RenderSetLogicalSize(renderer, GAME_W, GAME_H);

	LoadFont(&fnt_CP437, "PerfectDOSVGA437Win.ttf", 16);

	world = &world_instance;
	world->Init();
}

void Game::Quit() {
	switch (state) {
		case GAME_STATE_PLAYING: world->Quit(); break;
	}

	DestroyFont(&fnt_CP437);

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
	switch (state) {
		case GAME_STATE_PLAYING: world->Update(delta); break;
	}
}

void Game::Draw(float delta) {
	SDL_SetRenderDrawColor(renderer, 0, 0, 0, 255);
	SDL_RenderClear(renderer);

	switch (state) {
		case GAME_STATE_PLAYING: world->Draw(delta); break;
	}

	SDL_RenderPresent(renderer);
}
