#include "Game.h"

int main(int argc, char* argv[]) {
	Game game_instance{};
	game = &game_instance;

	game->Init();
	game->Run();
	game->Quit();

	return 0;
}
