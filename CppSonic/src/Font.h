#pragma once

#include <SDL.h>

struct GlyphData {
	SDL_Rect src;
	int xoffset;
	int yoffset;
	int advance;
};

struct Font {
	SDL_Texture* texture;
	int ptsize;
	int height;
	int ascent;
	int descent;
	int lineskip;
	GlyphData* glyphs; // 32..126
};

enum {
	HALIGN_LEFT,
	HALIGN_CENTER,
	HALIGN_RIGHT
};

enum {
	VALIGN_TOP,
	VALIGN_MIDDLE,
	VALIGN_BOTTOM
};

// Expects SDL_ttf (and SDL) to be initialized.
void LoadFontFromFileTTF(Font* font, const char* fname, int ptsize, SDL_Renderer* renderer);
void DestroyFont(Font* font);

SDL_Point DrawText(SDL_Renderer* renderer, Font* font, const char* text,
				   int x, int y,
				   int halign = 0, int valign = 0,
				   SDL_Color color = {255, 255, 255, 255});

SDL_Point DrawTextShadow(SDL_Renderer* renderer, Font* font, const char* text,
						 int x, int y,
						 int halign = 0, int valign = 0,
						 SDL_Color color = {255, 255, 255, 255});

SDL_Point MeasureText(Font* font, const char* text);
