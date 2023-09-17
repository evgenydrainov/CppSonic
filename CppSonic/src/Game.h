#pragma once

#include "World.h"
#include "Font.h"

#include <SDL_mixer.h>
#include <variant>

#define GAME_W 424
#define GAME_H 240
#define GAME_FPS 60

enum struct GameState {
	PLAYING
};

class Game {
public:
	Game();

	void Init();
	void Quit();
	void Run();

	SDL_Window* window = nullptr;
	SDL_Renderer* renderer = nullptr;
	SDL_Texture* game_texture = nullptr;
	bool key_pressed[SDL_SCANCODE_UP + 1] = {};
	float mouse_x = 0.0f;
	float mouse_y = 0.0f;

	Font fnt_CP437 = {};

private:
	void Frame();
	void Update(float delta);
	void Draw(float delta);

	std::variant<
		World
	> state;

	bool quit = false;
	bool skip_frame = false;
	bool frame_advance = false;
};

extern Game* game;
extern World* world;
