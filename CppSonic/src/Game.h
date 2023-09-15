#pragma once

#include "World.h"

#include <SDL_mixer.h>

#define GAME_W 424
#define GAME_H 240
#define GAME_FPS 60

enum GameState {
	GAME_STATE_PLAYING
};

struct Game {
	union {
		World world_instance{};
	};

	GameState state;
	SDL_Window* window;
	SDL_Renderer* renderer;
	bool quit;

	void Init();
	void Quit();
	void Run();
	void Frame();
	void Update(float delta);
	void Draw(float delta);
};

extern Game* game;
extern World* world;
