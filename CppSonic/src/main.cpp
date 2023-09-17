#include "Game.h"

int main(int argc, char* argv[]) {
	Game game;

	game.Init();
	game.Run();
	game.Quit();

	return 0;
}

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
#undef STB_SPRINTF_IMPLEMENTATION
