#pragma once

#ifdef _WIN32
#include <SDL.h>
#else
#include <SDL2/SDL.h>
#endif

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

SDL_Point DrawText(Font* font, const char* text,
				   int x, int y,
				   int halign = HALIGN_LEFT, int valign = VALIGN_TOP,
				   SDL_Color color = {255, 255, 255, 255});

SDL_Point DrawTextShadow(Font* font, const char* text,
						 int x, int y,
						 int halign = HALIGN_LEFT, int valign = VALIGN_TOP,
						 SDL_Color color = {255, 255, 255, 255});

SDL_Point MeasureText(Font* font, const char* text);

void LoadFont(Font* font, const char* fname, int ptsize);

void DestroyFont(Font* font);
