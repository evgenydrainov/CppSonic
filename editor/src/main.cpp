#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include "../../CppSonic/src/World.h"
#include "../../CppSonic/src/Game.h"

#include "../../CppSonic/src/stb_sprintf.h"
#include <math.h>
#include "../../CppSonic/src/mathh.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "imgui/imgui_internal.h"

#ifdef EDITOR

using Tile = TileMap::Tile;

int selected_tile = 0;
float tilemap_zoom = 1;

int window_w = 1600;
int window_h = 900;
int hover_tile_x = 0;
int hover_tile_y = 0;
int mouse_dx = 0;
int mouse_dy = 0;

enum {
	MODE_TILEMAP,
	MODE_HEIGHTS,
	MODE_WIDTHS,
	MODE_CHUNKS
};

int mode = MODE_TILEMAP;

struct {
	bool show;
	char level_data_path[256] = "levels/ghz1.bin";
	char chunk_data_path[256] = "map256/GHZ.bin";
	char tile_height_data_path[256] = "collide/Collision Array (Normal).bin";
	char tile_width_data_path[256] = "collide/Collision Array (Rotated).bin";
	char tile_angle_data_path[256] = "collide/Angle Map.bin";
	char tile_indicies_path[256] = "collide/GHZ.bin";
	char startpos_file_path[256] = "startpos/ghz1.bin";
	char tileset_texture[256] = "texture/ghz16.png";
} s1_import_window;

struct {
	bool show;
	char tileset_path[256] = "../CppSonic/levels/GHZ1/tileset.bin";
	char tilemap_path[256] = "../CppSonic/levels/GHZ1/tilemap.bin";
} export_window;

struct {
	bool show;
	char tileset_path[256] = "../CppSonic/levels/GHZ1/tileset.bin";
	char tilemap_path[256] = "../CppSonic/levels/GHZ1/tilemap.bin";
	char tileset_texture_path[256] = "../CppSonic/levels/GHZ1/tileset.png";
} import_window;

static bool ButtonCentered(const char* label, float alignment = 0.5f) {
	ImGuiStyle& style = ImGui::GetStyle();

	float size = ImGui::CalcTextSize(label).x + style.FramePadding.x * 2.0f;
	float avail = ImGui::GetContentRegionAvail().x;

	float off = (avail - size) * alignment;
	if (off > 0.0f)
		ImGui::SetCursorPosX(ImGui::GetCursorPosX() + off);

	return ImGui::Button(label);
}

static void import_s1_level() {
	world->tileset.Clear();
	world->tilemap.Clear();

	std::vector<uint8_t> level_data;
	std::vector<uint16_t> level_chunk_data;
	std::vector<uint8_t> collision_array;
	std::vector<uint8_t> collision_array_rot;
	std::vector<uint8_t> level_col_indicies;
	std::vector<uint16_t> startpos;
	std::vector<uint8_t> angle_map;

	{
		size_t size;
		uint8_t* data = (uint8_t*) SDL_LoadFile(s1_import_window.level_data_path, &size);
		if (!data || !size) return;
		level_data.assign(data, data + size / sizeof(*data));
		SDL_free(data);
	}

	{
		size_t size;
		uint16_t* data = (uint16_t*) SDL_LoadFile(s1_import_window.chunk_data_path, &size);
		if (!data || !size) return;
		level_chunk_data.assign(data, data + size / sizeof(*data));
		SDL_free(data);
	}

	{
		size_t size;
		uint8_t* data = (uint8_t*) SDL_LoadFile(s1_import_window.tile_height_data_path, &size);
		if (!data || !size) return;
		collision_array.assign(data, data + size / sizeof(*data));
		SDL_free(data);
	}

	{
		size_t size;
		uint8_t* data = (uint8_t*) SDL_LoadFile(s1_import_window.tile_width_data_path, &size);
		if (!data || !size) return;
		collision_array_rot.assign(data, data + size / sizeof(*data));
		SDL_free(data);
	}

	{
		size_t size;
		uint8_t* data = (uint8_t*) SDL_LoadFile(s1_import_window.tile_indicies_path, &size);
		if (!data || !size) return;
		level_col_indicies.assign(data, data + size / sizeof(*data));
		SDL_free(data);
	}

	{
		size_t size;
		uint16_t* data = (uint16_t*) SDL_LoadFile(s1_import_window.startpos_file_path, &size);
		if (!data || !size) return;
		startpos.assign(data, data + size / sizeof(*data));
		SDL_free(data);
	}

	{
		size_t size;
		uint8_t* data = (uint8_t*) SDL_LoadFile(s1_import_window.tile_angle_data_path, &size);
		if (!data || !size) return;
		angle_map.assign(data, data + size / sizeof(*data));
		SDL_free(data);
	}

	world->tileset.texture = IMG_LoadTexture(game->renderer, s1_import_window.tileset_texture);

	if (!world->tileset.texture) {
		return;
	}

	int texture_w;
	int texture_h;
	SDL_QueryTexture(world->tileset.texture, nullptr, nullptr, &texture_w, &texture_h);

	if (texture_w <= 0 || texture_h <= 0 || texture_w % 16 != 0 || texture_h % 16 != 0) {
		return;
	}

	int level_width_in_chunks = (int)level_data[0] + 1;
	int level_height_in_chunks = (int)level_data[1] + 1;

	if (level_width_in_chunks <= 0 || level_height_in_chunks <= 0) {
		return;
	}

	world->tilemap.width  = level_width_in_chunks  * 16;
	world->tilemap.height = level_height_in_chunks * 16;
	world->tilemap.tiles_a.resize(world->tilemap.width * world->tilemap.height);
	world->tilemap.tiles_b.resize(world->tilemap.width * world->tilemap.height);

	for (size_t i = 0; i < level_chunk_data.size(); i++) {
		level_chunk_data[i] = SDL_Swap16(level_chunk_data[i]);
	}

	startpos[0] = SDL_Swap16(startpos[0]);
	startpos[1] = SDL_Swap16(startpos[1]);

	world->tilemap.start_x = startpos[0];
	world->tilemap.start_y = startpos[1];

	world->player.x = world->tilemap.start_x;
	world->player.y = world->tilemap.start_y;

	world->camera_x = world->player.x - float(GAME_W) / 2.0f;
	world->camera_y = world->player.y - float(GAME_H) / 2.0f;

	world->tileset.tiles.resize((texture_w / 16) * (texture_h / 16));

	for (int y = 0; y < world->tilemap.height; y++) {
		for (int x = 0; x < world->tilemap.width; x++) {
			int chunk_x = x / 16;
			int chunk_y = y / 16;

			uint8_t chunk_index = level_data[2 + chunk_x + chunk_y * level_width_in_chunks] & 0b0111'1111;
			bool loop = level_data[2 + chunk_x + chunk_y * level_width_in_chunks] & 0b1000'0000;

			if (chunk_index == 0) {
				continue;
			}

			// if (loop) {
			// 	chunk_index++;
			// }

			uint16_t* tiles = &level_chunk_data[(chunk_index - 1) * 256];

			int tile_in_chunk_x = x % 16;
			int tile_in_chunk_y = y % 16;

			uint16_t tile = tiles[tile_in_chunk_x + tile_in_chunk_y * 16];

			world->tilemap.tiles_a[x + y * world->tilemap.width].index = tile & 0b0000'0011'1111'1111;
			world->tilemap.tiles_a[x + y * world->tilemap.width].hflip = tile & 0b0000'1000'0000'0000;
			world->tilemap.tiles_a[x + y * world->tilemap.width].vflip = tile & 0b0001'0000'0000'0000;
			world->tilemap.tiles_a[x + y * world->tilemap.width].top_solid = tile & 0b0010'0000'0000'0000;
			world->tilemap.tiles_a[x + y * world->tilemap.width].left_right_bottom_solid = tile & 0b0100'0000'0000'0000;
		}
	}

	for (size_t i = 0; i < level_col_indicies.size(); i++) {
		uint8_t index = level_col_indicies[i];
		uint8_t* height = &collision_array[index * 16];
		uint8_t* width = &collision_array_rot[index * 16];

		for (size_t j = 0; j < 16; j++) {
			world->tileset.tiles[i].height[j] = height[j];
			world->tileset.tiles[i].width[j] = width[j];
		}

		uint8_t angle = angle_map[index];
		if (angle == 0xFF) { // flagged
			world->tileset.tiles[i].angle = -1.0f;
		} else {
			world->tileset.tiles[i].angle = (float(256 - int(angle)) / 256.0f) * 360.0f;
		}
	}

	world->tileset.GenCollisionTextures();
}

static void export_level() {
	{
		SDL_RWops* f = SDL_RWFromFile(export_window.tileset_path, "wb");

		int tile_count = world->tileset.tiles.size();
		SDL_RWwrite(f, &tile_count, sizeof(tile_count), 1);

		SDL_RWwrite(f, world->tileset.tiles.data(), sizeof(*world->tileset.tiles.data()), tile_count);

		SDL_RWclose(f);
	}

	{
		SDL_RWops* f = SDL_RWFromFile(export_window.tilemap_path, "wb");

		int width  = world->tilemap.width;
		int height = world->tilemap.height;
		SDL_RWwrite(f, &width,  sizeof(width),  1);
		SDL_RWwrite(f, &height, sizeof(height), 1);

		float start_x = world->tilemap.start_x;
		float start_y = world->tilemap.start_y;
		SDL_RWwrite(f, &start_x, sizeof(start_x), 1);
		SDL_RWwrite(f, &start_y, sizeof(start_y), 1);

		SDL_RWwrite(f, world->tilemap.tiles_a.data(), sizeof(*world->tilemap.tiles_a.data()), width * height);
		SDL_RWwrite(f, world->tilemap.tiles_b.data(), sizeof(*world->tilemap.tiles_b.data()), width * height);

		SDL_RWclose(f);
	}
}

static void import_level() {
	world->tileset.LoadFromFile(import_window.tileset_path,
								import_window.tileset_texture_path);
	world->tilemap.LoadFromFile(import_window.tilemap_path);

	world->player.x = world->tilemap.start_x;
	world->player.y = world->tilemap.start_y;

	world->camera_x = world->player.x - float(GAME_W) / 2.0f;
	world->camera_y = world->player.y - float(GAME_H) / 2.0f;
}

int editor_main(int argc, char* argv[]) {
	Game game_instance;
	game->Init();

	// SDL_SetWindowSize(game->window, window_w, window_h);
	// SDL_SetWindowPosition(game->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);
	SDL_MaximizeWindow(game->window);

	SDL_RenderSetVSync(game->renderer, 1);

	SDL_Texture* game_texture = SDL_CreateTexture(game->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, window_w, window_h);

	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;     // Enable Keyboard Controls
	// io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad;      // Enable Gamepad Controls
	io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	ImGui::GetStyle().WindowMinSize = ImVec2(64, 96);

	// Setup Platform/Renderer backends
	ImGui_ImplSDL2_InitForSDLRenderer(game->window, game->renderer);
	ImGui_ImplSDLRenderer2_Init(game->renderer);

	// Load Fonts
	io.Fonts->AddFontFromFileTTF("DroidSans.ttf", 20);

	import_level();

	bool show_demo_window = false;

	bool quit = false;
	while (!quit) {
		int mouse_wheel_y = 0;
		Uint8 mouse_button_pressed = 0;

		{
			SDL_Event ev;
			while (SDL_PollEvent(&ev)) {
				ImGui_ImplSDL2_ProcessEvent(&ev);

				switch (ev.type) {
					case SDL_QUIT:
						quit = true;
						break;

					case SDL_MOUSEWHEEL:
						mouse_wheel_y = ev.wheel.y;
						break;

					case SDL_MOUSEBUTTONDOWN:
						mouse_button_pressed = ev.button.button;
						break;
				}
			}
		}

		float delta = 1.0f;

		// Update
		{
			SDL_GetWindowSize(game->window, &window_w, &window_h);

			SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);
		}

		// GUI
		{
			ImGui_ImplSDLRenderer2_NewFrame();
			ImGui_ImplSDL2_NewFrame();
			ImGui::NewFrame();

			if (ImGui::BeginMainMenuBar()) {
				if (ImGui::BeginMenu("File")) {
					if (ImGui::MenuItem("Import")) {
						import_window = {};
						import_window.show = true;
					}

					if (ImGui::MenuItem("Import S1 Level")) {
						s1_import_window = {};
						s1_import_window.show = true;
					}

					if (ImGui::MenuItem("Export")) {
						export_window = {};
						export_window.show = true;
					}

					ImGui::EndMenu();
				}

				if (ImGui::BeginMenu("Log Level")) {
					SDL_LogPriority p = SDL_LogGetPriority(0);

					if (ImGui::MenuItem("Verbose",  nullptr, p == SDL_LOG_PRIORITY_VERBOSE))  SDL_LogSetAllPriority(SDL_LOG_PRIORITY_VERBOSE);
					if (ImGui::MenuItem("Debug",    nullptr, p == SDL_LOG_PRIORITY_DEBUG))    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_DEBUG);
					if (ImGui::MenuItem("Info",     nullptr, p == SDL_LOG_PRIORITY_INFO))     SDL_LogSetAllPriority(SDL_LOG_PRIORITY_INFO);
					if (ImGui::MenuItem("Warn",     nullptr, p == SDL_LOG_PRIORITY_WARN))     SDL_LogSetAllPriority(SDL_LOG_PRIORITY_WARN);
					if (ImGui::MenuItem("Error",    nullptr, p == SDL_LOG_PRIORITY_ERROR))    SDL_LogSetAllPriority(SDL_LOG_PRIORITY_ERROR);
					if (ImGui::MenuItem("Critical", nullptr, p == SDL_LOG_PRIORITY_CRITICAL)) SDL_LogSetAllPriority(SDL_LOG_PRIORITY_CRITICAL);

					ImGui::EndMenu();
				}

				ImGui::MenuItem("Show Demo Window", nullptr, &show_demo_window);

				ImGui::Separator();
				ImGui::Separator();
				ImGui::Separator();

				if (ImGui::MenuItem("Tilemap", nullptr, mode == MODE_TILEMAP)) mode = MODE_TILEMAP;
				if (ImGui::MenuItem("Heights", nullptr, mode == MODE_HEIGHTS)) mode = MODE_HEIGHTS;
				if (ImGui::MenuItem("Widths",  nullptr, mode == MODE_WIDTHS))  mode = MODE_WIDTHS;
				if (ImGui::MenuItem("Chunks",  nullptr, mode == MODE_CHUNKS))  mode = MODE_CHUNKS;

				ImGui::EndMainMenuBar();
			}

			ImGui::DockSpaceOverViewport();

			switch (mode) {
				case MODE_TILEMAP: {
					if (ImGui::Begin("Tilemap")) {
						ImVec2 size = ImGui::GetContentRegionAvail();
						ImVec2 cursor = ImGui::GetCursorScreenPos();
						ImGui::Image(game_texture, size);

						if (ImGui::IsWindowFocused()) {
							// pan
							Uint32 mouse = SDL_GetMouseState(nullptr, nullptr);
							if (mouse & SDL_BUTTON(SDL_BUTTON_MIDDLE)) {
								world->camera_x -= (float)mouse_dx / tilemap_zoom;
								world->camera_y -= (float)mouse_dy / tilemap_zoom;
							}
						}

						if (ImGui::IsWindowHovered()) {
							// zoom
							if (mouse_wheel_y != 0) {
								float camera_center_x = world->camera_x + size.x / tilemap_zoom / 2.0f;
								float camera_center_y = world->camera_y + size.y / tilemap_zoom / 2.0f;

								if (mouse_wheel_y < 0) {
									tilemap_zoom /= 1.5f;
								} else {
									tilemap_zoom *= 1.5f;
								}
								tilemap_zoom = clamp(tilemap_zoom, 0.5f, 10.0f);

								world->camera_x = camera_center_x - size.x / tilemap_zoom / 2.0f;
								world->camera_y = camera_center_y - size.y / tilemap_zoom / 2.0f;
							}

							// hover tile
							float mouse_x = (ImGui::GetMousePos().x - cursor.x) / tilemap_zoom + world->camera_x;
							float mouse_y = (ImGui::GetMousePos().y - cursor.y) / tilemap_zoom + world->camera_y;

							hover_tile_x = int(mouse_x) / 16;
							hover_tile_y = int(mouse_y) / 16;

							hover_tile_x = clamp(hover_tile_x, 0, world->tilemap.width  - 1);
							hover_tile_y = clamp(hover_tile_y, 0, world->tilemap.height - 1);
						}

						// resize game texture
						{
							int w;
							int h;
							SDL_QueryTexture(game_texture, nullptr, nullptr, &w, &h);
							if (w != size.x || h != size.y) {
								SDL_DestroyTexture(game_texture);
								game_texture = SDL_CreateTexture(game->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, size.x, size.y);
							}
						}
					}
					ImGui::End();

					if (ImGui::Begin("Tileset")) {
						int w;
						int h;
						SDL_QueryTexture(world->tileset.texture, nullptr, nullptr, &w, &h);

						ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
						ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
						ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 1));

						ImVec2 cursor = ImGui::GetCursorScreenPos();

						for (int y = 0; y < h / 16; y++) {
							for (int x = 0; x < w / 16; x++) {
								int i = x + y * 16;

								ImVec2 uv0((float)(x * 16) / (float)w, (float)(y * 16) / (float)h);
								ImVec2 uv1((float)((x + 1) * 16) / (float)w, (float)((y + 1) * 16) / (float)h);

								if (i == selected_tile) {
									ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 1));
								} else {
									ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 1));
								}

								char buf[50];
								stb_snprintf(buf, sizeof(buf), "tile button %d", i);
								if (ImGui::ImageButton(buf, world->tileset.texture, ImVec2(32, 32), uv0, uv1, ImVec4(0, 0, 0, 1))) {
									selected_tile = i;
								}

								ImGui::PopStyleColor();

								if (ImGui::GetItemRectMin().x - cursor.x + 32 * 2 < ImGui::GetContentRegionAvail().x) {
									ImGui::SameLine();
								}
							}
						}

						ImGui::PopStyleColor();
						ImGui::PopStyleVar(2);
					}
					ImGui::End();

					if (ImGui::Begin("Info")) {
						ImGui::Text("Tile X: %d", hover_tile_x);
						ImGui::Text("Tile Y: %d", hover_tile_y);
						Tile tile = world->tilemap.GetTileA(hover_tile_x, hover_tile_y);
						ImGui::Text("Tile ID: %d", tile.index);
						ImGui::Text("Tile HFlip: %d", tile.hflip);
						ImGui::Text("Tile VFlip: %d", tile.vflip);
						ImGui::Text("Tile Top Solid: %d", tile.top_solid);
						ImGui::Text("Tile Left/Right/Bottom Solid: %d", tile.left_right_bottom_solid);
						ImGui::NewLine();
						ImGui::Text("Hold \"3\" to show tile heights");
						ImGui::Text("Hold \"4\" to show tile widths");
						ImGui::Text("Hold \"5\" to show flagges tiles");
						ImGui::Text("Hold \"6\" to show top solid tiles");
						ImGui::Text("Hold \"7\" to show left/right/bottom solid tiles");
					}
					ImGui::End();
					break;
				}

				case MODE_HEIGHTS: {

					break;
				}

				case MODE_WIDTHS: {

					break;
				}

				case MODE_CHUNKS: {

					break;
				}
			}

			if (show_demo_window) {
				ImGui::ShowDemoWindow(&show_demo_window);
			}

			if (s1_import_window.show) {
				ImGui::SetNextWindowPos(ImVec2(window_w / 2, window_h / 2), ImGuiCond_Appearing, ImVec2(0.5, 0.5));
				ImGui::SetNextWindowSize(ImVec2(500, 0));
				if (ImGui::Begin("Import S1 Level",
								 &s1_import_window.show,
								 ImGuiWindowFlags_NoResize
								 | ImGuiWindowFlags_NoCollapse
								 | ImGuiWindowFlags_NoDocking
								 | ImGuiWindowFlags_NoSavedSettings)) {
					ImGui::InputText("level data", s1_import_window.level_data_path, sizeof(s1_import_window.level_data_path));
					ImGui::InputText("chunk data", s1_import_window.chunk_data_path, sizeof(s1_import_window.chunk_data_path));
					ImGui::InputText("tile height data", s1_import_window.tile_height_data_path, sizeof(s1_import_window.tile_height_data_path));
					ImGui::InputText("tile width data", s1_import_window.tile_width_data_path, sizeof(s1_import_window.tile_width_data_path));
					ImGui::InputText("tile angle data", s1_import_window.tile_angle_data_path, sizeof(s1_import_window.tile_angle_data_path));
					ImGui::InputText("tile indicies data", s1_import_window.tile_indicies_path, sizeof(s1_import_window.tile_indicies_path));
					ImGui::InputText("startpos file", s1_import_window.startpos_file_path, sizeof(s1_import_window.startpos_file_path));
					ImGui::InputText("tileset texture", s1_import_window.tileset_texture, sizeof(s1_import_window.tileset_texture));

					if (ButtonCentered("Import")) {
						import_s1_level();
						s1_import_window.show = false;
					}
				}
				ImGui::End();
			}

			if (export_window.show) {
				ImGui::SetNextWindowPos(ImVec2(window_w / 2, window_h / 2), ImGuiCond_Appearing, ImVec2(0.5, 0.5));
				ImGui::SetNextWindowSize(ImVec2(500, 0));
				if (ImGui::Begin("Export",
								 &export_window.show,
								 ImGuiWindowFlags_NoResize
								 | ImGuiWindowFlags_NoCollapse
								 | ImGuiWindowFlags_NoDocking
								 | ImGuiWindowFlags_NoSavedSettings)) {
					ImGui::InputText("tileset path", export_window.tileset_path, sizeof(export_window.tileset_path));
					ImGui::InputText("tilemap path", export_window.tilemap_path, sizeof(export_window.tilemap_path));

					if (ButtonCentered("Export")) {
						export_level();
						export_window.show = false;
					}
				}
				ImGui::End();
			}

			if (import_window.show) {
				ImGui::SetNextWindowPos(ImVec2(window_w / 2, window_h / 2), ImGuiCond_Appearing, ImVec2(0.5, 0.5));
				ImGui::SetNextWindowSize(ImVec2(500, 0));
				if (ImGui::Begin("Import",
								 &import_window.show,
								 ImGuiWindowFlags_NoResize
								 | ImGuiWindowFlags_NoCollapse
								 | ImGuiWindowFlags_NoDocking
								 | ImGuiWindowFlags_NoSavedSettings)) {
					ImGui::InputText("tileset path", import_window.tileset_path, sizeof(import_window.tileset_path));
					ImGui::InputText("tilemap path", import_window.tilemap_path, sizeof(import_window.tilemap_path));
					ImGui::InputText("tileset texture path", import_window.tileset_texture_path, sizeof(import_window.tileset_texture_path));

					if (ButtonCentered("Import")) {
						import_level();
						import_window.show = false;
					}
				}
				ImGui::End();
			}

			{
				// also focus windows with middle and right click
				if (mouse_button_pressed == SDL_BUTTON_MIDDLE || mouse_button_pressed == SDL_BUTTON_RIGHT) {
					ImGui::FocusWindow(ImGui::GetCurrentContext()->HoveredWindow);
				}
			}
		}

		// Draw
		{
			ImGui::Render();

			SDL_SetRenderTarget(game->renderer, game_texture);
			{
				SDL_SetRenderDrawColor(game->renderer, 32, 32, 32, 255);
				SDL_RenderClear(game->renderer);

				{
					int w;
					int h;
					SDL_QueryTexture(game_texture, nullptr, nullptr, &w, &h);
					world->target_w = (float)w / tilemap_zoom;
					world->target_h = (float)h / tilemap_zoom;
					// printf("%d x %d\n", w, h);
				}
				SDL_RenderSetScale(game->renderer, tilemap_zoom, tilemap_zoom);

				world->Draw(delta);

				// draw hovered tile
				{
					SDL_Rect src = {
						(selected_tile % 16) * 16,
						(selected_tile / 16) * 16,
						16,
						16
					};
					SDL_Rect dest = {
						hover_tile_x * 16 - (int)world->camera_x,
						hover_tile_y * 16 - (int)world->camera_y,
						16,
						16
					};
					if (selected_tile == 0) {
						SDL_SetRenderDrawColor(game->renderer, 255, 255, 255, 255);
						SDL_RenderDrawRect(game->renderer, &dest);
					} else {
						SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
						SDL_RenderFillRect(game->renderer, &dest);
						SDL_RenderCopy(game->renderer, world->tileset.texture, &src, &dest);
					}
				}

				SDL_RenderSetScale(game->renderer, 1, 1);
			}
			SDL_SetRenderTarget(game->renderer, nullptr);

			SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
			SDL_RenderClear(game->renderer);

			ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());

			SDL_RenderPresent(game->renderer);
		}
	}

	// Cleanup
	ImGui_ImplSDLRenderer2_Shutdown();
	ImGui_ImplSDL2_Shutdown();
	ImGui::DestroyContext();

	SDL_DestroyTexture(game_texture);

	game->Quit();

	return 0;
}

#else

int editor_main(int argc, char* argv[]) {
	return 0;
}

#endif
