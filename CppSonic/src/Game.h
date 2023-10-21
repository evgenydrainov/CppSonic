#pragma once

// #define EDITOR

#include "World.h"

#define GAME_W 424
#define GAME_H 240
#define GAME_FPS 60

struct Game;
extern Game* game;

enum struct GameState {
	PLAYING
};

struct Game {
	GameState state;
	union {
		World world_instance{};
	};

	SDL_Window* window;
	SDL_Renderer* renderer;
	SDL_Texture* game_texture;
	bool key_pressed[SDL_SCANCODE_UP + 1];
	float mouse_x;
	float mouse_y;
	bool quit;
	bool skip_frame;
	bool frame_advance;
	double update_took;
	double draw_took;
	double prev_time;
	double fps;

	void Init();
	void Quit();

	void Run();
	void Frame();
	void Update(float delta);
	void Draw(float delta);
};
