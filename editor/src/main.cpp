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

int editor_main(int argc, char* argv[]) {
	Game game_instance;
	game->Init();

	SDL_SetWindowSize(game->window, window_w, window_h);
	SDL_SetWindowPosition(game->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

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
				if (ImGui::MenuItem("Tilemap")) mode = MODE_TILEMAP;
				if (ImGui::MenuItem("Heights")) mode = MODE_HEIGHTS;
				if (ImGui::MenuItem("Widths"))  mode = MODE_WIDTHS;
				if (ImGui::MenuItem("Chunks"))  mode = MODE_CHUNKS;

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

							hover_tile_x = clamp(hover_tile_x, 0, world->level_width  - 1);
							hover_tile_y = clamp(hover_tile_y, 0, world->level_height - 1);
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
						SDL_QueryTexture(world->tileset_texture, nullptr, nullptr, &w, &h);

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
								if (ImGui::ImageButton(buf, world->tileset_texture, ImVec2(32, 32), uv0, uv1, ImVec4(0, 0, 0, 1))) {
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
						Tile tile = world->get_tile(hover_tile_x, hover_tile_y);
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
						SDL_RenderCopy(game->renderer, world->tileset_texture, &src, &dest);
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

#if 0
int main(int argc, char* argv[]) {
	return editor_main(argc, argv);
}
#endif
