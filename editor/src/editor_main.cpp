#include <SDL.h>
#include <SDL_image.h>
#include <SDL_ttf.h>
#include <SDL_mixer.h>

#include "../../CppSonic/src/World.h"
#include "../../CppSonic/src/Game.h"

#include "../../CppSonic/src/stb_sprintf.h"
#include <math.h>
#include "../../CppSonic/src/mathh.h"
#include "../../CppSonic/src/misc.h"

#include "imgui/imgui.h"
#include "imgui/imgui_impl_sdl2.h"
#include "imgui/imgui_impl_sdlrenderer2.h"
#include "imgui/imgui_internal.h"
#include "IconsFontAwesome4.h"

#include <vector>
#include <direct.h>

#ifdef EDITOR

int selected_tile = 0;
float tilemap_zoom = 1;

int window_w = 1600;
int window_h = 900;
int hover_tile_x = 0;
int hover_tile_y = 0;
int mouse_dx = 0;
int mouse_dy = 0;
bool playing = false;

enum {
	MODE_TILEMAP,
	MODE_HEIGHTS,
	MODE_WIDTHS,
	MODE_CHUNKS,
	MODE_OBJECTS
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
	char tileset_path[256] = "../CppSonic/levels/export/tileset.bin";
	char tilemap_path[256] = "../CppSonic/levels/export/tilemap.bin";
	char objects_path[256] = "../CppSonic/levels/export/objects.bin";
} export_window;

struct {
	bool show;
	char tileset_path[256] = "../CppSonic/levels/export/tileset.bin";
	char tilemap_path[256] = "../CppSonic/levels/export/tilemap.bin";
	char tileset_texture_path[256] = "../CppSonic/levels/export/tileset.png";
	char objects_path[256] = "../CppSonic/levels/export/objects.bin";
} import_window;

#define X(name) #name,
static const char* object_type_name[] = {
	LIST_OF_ALL_OBJ_TYPES
};
#undef X

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
	world->tileset.Destroy();
	world->tilemap.Destroy();

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
	world->tileset.tiles_in_row = 16;

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
	world->tilemap.tile_count = world->tilemap.width * world->tilemap.height;
	world->tilemap.tiles_a = (Tile*) calloc(world->tilemap.tile_count, sizeof(*world->tilemap.tiles_a));
	world->tilemap.tiles_b = (Tile*) calloc(world->tilemap.tile_count, sizeof(*world->tilemap.tiles_b));

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

	world->tileset.tile_count = (texture_w / 16) * (texture_h / 16);
	world->tileset.tile_heights = (uint8_t*) calloc(world->tileset.tile_count, 16 * sizeof(*world->tileset.tile_heights));
	world->tileset.tile_widths = (uint8_t*) calloc(world->tileset.tile_count, 16 * sizeof(*world->tileset.tile_widths));
	world->tileset.tile_angles = (float*) calloc(world->tileset.tile_count, sizeof(*world->tileset.tile_angles));

	for (int y = 0; y < world->tilemap.height; y++) {
		for (int x = 0; x < world->tilemap.width; x++) {
			int chunk_x = x / 16;
			int chunk_y = y / 16;

			uint8_t chunk_index = level_data[2 + chunk_x + chunk_y * level_width_in_chunks] & 0b0111'1111;
			bool loop = level_data[2 + chunk_x + chunk_y * level_width_in_chunks] & 0b1000'0000;

			if (chunk_index == 0) {
				continue;
			}

			uint16_t* tiles = &level_chunk_data[(chunk_index - 1) * 256];

			int tile_in_chunk_x = x % 16;
			int tile_in_chunk_y = y % 16;

			uint16_t tile = tiles[tile_in_chunk_x + tile_in_chunk_y * 16];

			world->tilemap.tiles_a[x + y * world->tilemap.width].index = tile & 0b0000'0011'1111'1111;
			world->tilemap.tiles_a[x + y * world->tilemap.width].hflip = tile & 0b0000'1000'0000'0000;
			world->tilemap.tiles_a[x + y * world->tilemap.width].vflip = tile & 0b0001'0000'0000'0000;
			world->tilemap.tiles_a[x + y * world->tilemap.width].top_solid = tile & 0b0010'0000'0000'0000;
			world->tilemap.tiles_a[x + y * world->tilemap.width].left_right_bottom_solid = tile & 0b0100'0000'0000'0000;

			if (loop) {
				chunk_index++;
				tiles = &level_chunk_data[(chunk_index - 1) * 256];
				tile = tiles[tile_in_chunk_x + tile_in_chunk_y * 16];

				world->tilemap.tiles_b[x + y * world->tilemap.width].index = tile & 0b0000'0011'1111'1111;
				world->tilemap.tiles_b[x + y * world->tilemap.width].hflip = tile & 0b0000'1000'0000'0000;
				world->tilemap.tiles_b[x + y * world->tilemap.width].vflip = tile & 0b0001'0000'0000'0000;
				world->tilemap.tiles_b[x + y * world->tilemap.width].top_solid = tile & 0b0010'0000'0000'0000;
				world->tilemap.tiles_b[x + y * world->tilemap.width].left_right_bottom_solid = tile & 0b0100'0000'0000'0000;
			}
		}
	}

	for (size_t i = 0; i < level_col_indicies.size(); i++) {
		uint8_t index = level_col_indicies[i];
		uint8_t* height = &collision_array[index * 16];
		uint8_t* width = &collision_array_rot[index * 16];

		for (size_t j = 0; j < 16; j++) {
			world->tileset.tile_heights[i * 16 + j] = height[j];
			world->tileset.tile_widths[i * 16 + j] = width[j];
		}

		uint8_t angle = angle_map[index];
		if (angle == 0xFF) { // flagged
			world->tileset.tile_angles[i] = -1.0f;
		} else {
			world->tileset.tile_angles[i] = (float(256 - int(angle)) / 256.0f) * 360.0f;
		}
	}

	world->tileset.GenCollisionTextures();
}

static void export_level() {
	{
		if (SDL_RWops* f = SDL_RWFromFile(export_window.tileset_path, "wb")) {
			int tile_count = world->tileset.tile_count;
			SDL_RWwrite(f, &tile_count, sizeof(tile_count), 1);

			SDL_RWwrite(f, world->tileset.tile_heights, 16 * sizeof(*world->tileset.tile_heights), tile_count);
			SDL_RWwrite(f, world->tileset.tile_widths, 16 * sizeof(*world->tileset.tile_widths), tile_count);
			SDL_RWwrite(f, world->tileset.tile_angles, sizeof(*world->tileset.tile_angles), tile_count);

			SDL_RWclose(f);
		} else {
			char buf[256];
			stb_snprintf(buf, sizeof(buf), "%s", SDL_GetError());
			SDL_ShowSimpleMessageBox(0, "ERROR", buf, nullptr);
		}
	}

	{
		if (SDL_RWops* f = SDL_RWFromFile(export_window.tilemap_path, "wb")) {
			int width  = world->tilemap.width;
			int height = world->tilemap.height;
			SDL_RWwrite(f, &width,  sizeof(width),  1);
			SDL_RWwrite(f, &height, sizeof(height), 1);

			float start_x = world->tilemap.start_x;
			float start_y = world->tilemap.start_y;
			SDL_RWwrite(f, &start_x, sizeof(start_x), 1);
			SDL_RWwrite(f, &start_y, sizeof(start_y), 1);

			int tile_count = world->tilemap.tile_count;
			SDL_RWwrite(f, world->tilemap.tiles_a, sizeof(*world->tilemap.tiles_a), tile_count);
			SDL_RWwrite(f, world->tilemap.tiles_b, sizeof(*world->tilemap.tiles_b), tile_count);

			SDL_RWclose(f);
		} else {
			char buf[256];
			stb_snprintf(buf, sizeof(buf), "%s", SDL_GetError());
			SDL_ShowSimpleMessageBox(0, "ERROR", buf, nullptr);
		}
	}

	{
		if (SDL_RWops* f = SDL_RWFromFile(export_window.objects_path, "wb")) {
			int object_count = world->object_count;
			SDL_RWwrite(f, &object_count, sizeof(object_count), 1);

			for (int i = 0; i < world->object_count; i++) {
				Object o = world->objects[i];
				o.id = -1;
				SDL_RWwrite(f, &o, sizeof(o), 1);
			}

			SDL_RWclose(f);
		} else {
			char buf[256];
			stb_snprintf(buf, sizeof(buf), "%s", SDL_GetError());
			SDL_ShowSimpleMessageBox(0, "ERROR", buf, nullptr);
		}
	}
}

static void import_level() {
	world->tileset.Destroy();
	world->tilemap.Destroy();

	world->tileset.LoadFromFile(import_window.tileset_path,
								import_window.tileset_texture_path);
	world->tilemap.LoadFromFile(import_window.tilemap_path);
	world->load_objects(import_window.objects_path);

	world->player.x = world->tilemap.start_x;
	world->player.y = world->tilemap.start_y;

	world->camera_x = world->player.x - float(GAME_W) / 2.0f;
	world->camera_y = world->player.y - float(GAME_H) / 2.0f;
}

#if 0
#include <sstream>
#include <fstream>

static void import_gms() {
	world->tileset.Clear();
	world->tilemap.Clear();

	int w;
	int h;
	world->tileset.texture = IMG_LoadTexture(game->renderer, "gms.png");
	SDL_QueryTexture(world->tileset.texture, nullptr, nullptr, &w, &h);
	world->tileset.tiles.resize((w / 16) * (h / 16));
	world->tileset.tiles_in_row = w / 16;

	std::ifstream f("gms.txt");

	int width;
	int height;
	f >> width;
	f >> height;

	world->tilemap.width = width;
	world->tilemap.height = height;
	world->tilemap.tiles_a.resize(width * height);
	world->tilemap.tiles_b.resize(width * height);

	for (int y = 0; y < height; y++) {
		for (int x = 0; x < width; x++) {
			int index;
			int mirror;
			int flip;
			f >> index;
			f >> mirror;
			f >> flip;

			world->tilemap.tiles_a[x + y * width].index = index;
			world->tilemap.tiles_a[x + y * width].hflip = mirror;
			world->tilemap.tiles_a[x + y * width].vflip = flip;
		}
	}

	world->tileset.GenCollisionTextures();
}
#else
static void import_gms() {}
#endif

static SDL_Texture* game_texture;
static ImFont* fnt_droid_sans;
static ImFont* fnt_CP437;
static int mouse_wheel_y;
static Uint8 mouse_button_pressed;

static void show_tilemap_window() {
	if (ImGui::Begin("Tilemap")) {
		ImVec2 size = ImGui::GetContentRegionAvail();

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

			game->mouse_x = mouse_x;
			game->mouse_y = mouse_y;

			// draw tiles
			if (mode == MODE_TILEMAP) {
				Uint32 mouse = SDL_GetMouseState(nullptr, nullptr);
				if (mouse & SDL_BUTTON(SDL_BUTTON_LEFT)) {
					world->tilemap.tiles_a[hover_tile_x + hover_tile_y * world->tilemap.width] = {};
					world->tilemap.tiles_a[hover_tile_x + hover_tile_y * world->tilemap.width].index = selected_tile;
					world->tilemap.tiles_a[hover_tile_x + hover_tile_y * world->tilemap.width].top_solid = true;
					world->tilemap.tiles_a[hover_tile_x + hover_tile_y * world->tilemap.width].left_right_bottom_solid = true;
				}
			}
		}

		{
			ImGui::PushStyleColor(ImGuiCol_WindowBg, ImVec4(0, 0, 0, 0.5));
			ImGui::SetNextWindowPos(ImVec2(cursor.x + size.x - 10, cursor.y + size.y - 10), 0, ImVec2(1, 1));
			if (ImGui::Begin("tilemap info overlay",
							 nullptr,
							 ImGuiWindowFlags_NoDecoration
							 | ImGuiWindowFlags_NoSavedSettings
							 | ImGuiWindowFlags_NoInputs
							 | ImGuiWindowFlags_AlwaysAutoResize)) {
				ImGui::PushFont(fnt_CP437);

				// if (mode == MODE_TILEMAP) {
					ImGui::Text("Tile X: %d", hover_tile_x);
					ImGui::Text("Tile Y: %d", hover_tile_y);
					Tile tile = world->tilemap.GetTileA(hover_tile_x, hover_tile_y);
					ImGui::Text("Tile ID: %d", tile.index);
					ImGui::Text("Tile HFlip: %d", tile.hflip);
					ImGui::Text("Tile VFlip: %d", tile.vflip);
					ImGui::Text("Tile Top Solid: %d", tile.top_solid);
					ImGui::Text("Tile Left/Right/Bottom Solid: %d", tile.left_right_bottom_solid);
				// } else if (mode == MODE_OBJECTS) {
				// 	ImGui::Text("Mouse X: %f", game->mouse_x);
				// 	ImGui::Text("Mouse Y: %f", game->mouse_y);
				// }

				ImGui::PopFont();
			}
			ImGui::End();
			ImGui::PopStyleColor();
		}
	}
	ImGui::End();
}

static void show_tileset_window() {
	if (ImGui::Begin("Tileset")) {
		ImGui::PushStyleVar(ImGuiStyleVar_ItemSpacing, ImVec2(0, 0));
		ImGui::PushStyleVar(ImGuiStyleVar_FramePadding, ImVec2(2, 2));
		ImGui::PushStyleColor(ImGuiCol_ButtonHovered, ImVec4(1, 1, 1, 1));

		ImVec2 cursor = ImGui::GetCursorScreenPos();

		for (int tile_index = 0; tile_index < world->tileset.tile_count; tile_index++) {
			SDL_Rect src = world->tileset.GetTextureSrcRect(tile_index);

			ImVec2 uv0(src.x, src.y);
			ImVec2 uv1(src.x + src.w, src.y + src.h);

			int w;
			int h;
			SDL_QueryTexture(world->tileset.texture, nullptr, nullptr, &w, &h);

			uv0.x /= (float) w;
			uv0.y /= (float) h;

			uv1.x /= (float) w;
			uv1.y /= (float) h;

			if (tile_index == selected_tile) {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(1, 1, 1, 1));
			} else {
				ImGui::PushStyleColor(ImGuiCol_Button, ImVec4(0, 0, 0, 1));
			}

			char buf[50];
			stb_snprintf(buf, sizeof(buf), "tile button %d", tile_index);
			if (ImGui::ImageButton(buf, world->tileset.texture, ImVec2(32, 32), uv0, uv1, ImVec4(0, 0, 0, 1))) {
				selected_tile = tile_index;
			}

			ImGui::PopStyleColor();

			if (ImGui::GetItemRectMin().x - cursor.x + 32 * 2 < ImGui::GetContentRegionAvail().x) {
				ImGui::SameLine();
			}
		}

		ImGui::PopStyleColor();
		ImGui::PopStyleVar(2);
	}
	ImGui::End();
}

static void show_objects_window() {
	if (ImGui::Begin("Objects")) {
		if (ImGui::Button(ICON_FA_PLUS)) {
			world->CreateObject({});
		}
		ImGui::SameLine();
		if (ImGui::Button(ICON_FA_MINUS)) {
			world->object_count--;
			if (world->object_count < 0) world->object_count = 0;
		}
		ImGui::SameLine();
		ImGui::Text("%d/%d", world->object_count, MAX_OBJECTS);

		for (int i = 0; i < world->object_count; i++) {
			ImGui::PushID(i);

			Object* o = world->objects + i;

			ImGui::Text("Index: %d", i);
			ImGui::Text("ID: %d", o->id);
			ImGui::Text("Type: %d (%s)", o->type, object_type_name[(int)o->type]);
			ImGui::Text("Flags: %u", o->flags);
			{
				int x = o->x / 16.0f;
				int y = o->y / 16.0f;
				bool touched = false;
				touched |= ImGui::DragInt("x", &x, 0.05f, 0, world->tilemap.width  - 1);
				touched |= ImGui::DragInt("y", &y, 0.05f, 0, world->tilemap.height - 1);
				if (touched) {
					o->x = x * 16;
					o->y = y * 16;
				}
			}

			switch (o->type) {
				case ObjType::VERTICAL_LAYER_SWITCHER: {
					ImGui::InputFloat("Radius", &o->radius);
					ImGui::InputInt("Layer 1", &o->layer_1);
					ImGui::InputInt("Layer 2", &o->layer_2);
					{
						bool on = (o->flags & FLAG_LAYER_SWITCHER_GROUNDED_ONLY) != 0;
						ImGui::Checkbox("Grounded Only", &on);
						if (on) o->flags |= FLAG_LAYER_SWITCHER_GROUNDED_ONLY;
						else o->flags &= ~FLAG_LAYER_SWITCHER_GROUNDED_ONLY;
					}
					break;
				}
			}

			ImGui::Separator();

			ImGui::PopID();
		}
	}
	ImGui::End();
}

int editor_main(int argc, char* argv[]) {
	_chdir("../CppSonic/");

	Game game_instance{};
	game = &game_instance;

	game->Init();

	_chdir("../editor/");

	SDL_SetWindowSize(game->window, window_w, window_h);
	SDL_SetWindowPosition(game->window, SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED);

	SDL_MaximizeWindow(game->window);

	game_texture = SDL_CreateTexture(game->renderer, SDL_PIXELFORMAT_ARGB8888, SDL_TEXTUREACCESS_TARGET, window_w, window_h);

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
	fnt_droid_sans = io.Fonts->AddFontFromFileTTF("DroidSans.ttf", 20);

	{
		ImFontConfig config;
		config.MergeMode = true;
		config.GlyphMinAdvanceX = 14; // Use if you want to make the icon monospaced
		static const ImWchar icon_ranges[] = { ICON_MIN_FA, ICON_MAX_FA, 0 };
		io.Fonts->AddFontFromFileTTF(FONT_ICON_FILE_NAME_FA, 14, &config, icon_ranges);
	}

	fnt_CP437 = io.Fonts->AddFontFromFileTTF("PerfectDOSVGA437Win.ttf", 16);

	bool show_demo_window = false;

	bool quit = false;
	while (!quit) {
		double t = GetTime();
		double frame_end_time = t + (1.0 / (double)GAME_FPS);
		memset(game->key_pressed, 0, sizeof(game->key_pressed));
		game->skip_frame = game->frame_advance;

		mouse_wheel_y = 0;
		mouse_button_pressed = 0;

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

					case SDL_KEYDOWN: {
						SDL_Scancode scancode = ev.key.keysym.scancode;
						if (0 <= scancode && scancode < ArrayLength(game->key_pressed)) {
							game->key_pressed[scancode] = true;
						}
						switch (scancode) {
							case SDL_SCANCODE_F5: {
								game->frame_advance = true;
								game->skip_frame = false;
								break;
							}
							case SDL_SCANCODE_F6: {
								game->frame_advance = false;
								break;
							}
						}
						break;
					}
				}
			}
		}

		float delta = 1.0f;

		// Update
		{
			SDL_GetWindowSize(game->window, &window_w, &window_h);

			SDL_GetRelativeMouseState(&mouse_dx, &mouse_dy);

			if (playing) {
				if (!game->skip_frame) {
					world->Update(delta);
				}
				if (game->key_pressed[SDL_SCANCODE_ESCAPE]) {
					world->debug ^= true;
				}
			}
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

					if (ImGui::MenuItem("Import GMS")) {
						import_gms();
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
				if (ImGui::MenuItem("Objects", nullptr, mode == MODE_OBJECTS)) mode = MODE_OBJECTS;

				ImGui::Separator();
				ImGui::Separator();
				ImGui::Separator();

				if (playing) {
					if (ImGui::Button(ICON_FA_PAUSE)) playing = false;
				} else {
					if (ImGui::Button(ICON_FA_PLAY)) playing = true;
				}

				ImGui::EndMainMenuBar();
			}

			ImGui::DockSpaceOverViewport();

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
					ImGui::InputText("objects path", export_window.objects_path, sizeof(export_window.objects_path));

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
					ImGui::InputText("objects path", import_window.objects_path, sizeof(import_window.objects_path));

					if (ButtonCentered("Import")) {
						import_level();
						import_window.show = false;
					}
				}
				ImGui::End();
			}

			switch (mode) {
				case MODE_TILEMAP: {
					show_tilemap_window();

					show_tileset_window();

					// if (ImGui::Begin("Info")) {
					// 	ImGui::Text("Hold \"3\" to show tile heights");
					// 	ImGui::Text("Hold \"4\" to show tile widths");
					// 	ImGui::Text("Hold \"5\" to show flagges tiles");
					// 	ImGui::Text("Hold \"6\" to show top solid tiles");
					// 	ImGui::Text("Hold \"7\" to show left/right/bottom solid tiles");
					// }
					// ImGui::End();
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

				case MODE_OBJECTS: {
					show_tilemap_window();

					show_objects_window();
					break;
				}
			}

			if (show_demo_window) {
				ImGui::ShowDemoWindow(&show_demo_window);
			}

			// focus windows with middle and right click
			if (mouse_button_pressed == SDL_BUTTON_MIDDLE || mouse_button_pressed == SDL_BUTTON_RIGHT) {
				ImGui::FocusWindow(ImGui::GetCurrentContext()->HoveredWindow);
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

				if (world->tileset.texture) {
					world->Draw(delta);

					// draw hovered tile
					if (mode == MODE_TILEMAP) {
						SDL_Rect src = world->tileset.GetTextureSrcRect(selected_tile);
						SDL_Rect dest = {
							hover_tile_x * 16 - int(world->camera_x),
							hover_tile_y * 16 - int(world->camera_y),
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
				}

				SDL_RenderSetScale(game->renderer, 1, 1);
			}
			SDL_SetRenderTarget(game->renderer, nullptr);

			SDL_SetRenderDrawColor(game->renderer, 0, 0, 0, 255);
			SDL_RenderClear(game->renderer);

			ImGui_ImplSDLRenderer2_RenderDrawData(ImGui::GetDrawData());

			SDL_RenderPresent(game->renderer);
		}

#ifndef __EMSCRIPTEN__
		t = GetTime();
		double time_left = frame_end_time - t;
		if (time_left > 0.0) {
			SDL_Delay((Uint32) (time_left * (0.95 * 1000.0)));
			while (GetTime() < frame_end_time) {}
		}
#endif
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
