#pragma once

#include "World.h"

enum GameState {
	GAME_STATE_PLAYING
};

struct Game {
	union {
		World world{};
	};

	GameState state = GAME_STATE_PLAYING;

	void Init();
	void Quit();
	void Run();
	void Frame();
};

extern Game* game;
