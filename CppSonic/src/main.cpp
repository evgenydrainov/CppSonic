#include "Game.h"

#ifndef EDITOR

int main(int argc, char* argv[]) {
	Game game_instance{};
	game = &game_instance;

	game->Init();
	game->Run();
	game->Quit();

	return 0;
}

#else
#include <direct.h>

int main(int argc, char* argv[]) {
	extern int editor_main(int, char*[]);

	_chdir("../editor/");

	return editor_main(argc, argv);
}

#endif

#define STB_SPRINTF_IMPLEMENTATION
#include "stb_sprintf.h"
#undef STB_SPRINTF_IMPLEMENTATION
