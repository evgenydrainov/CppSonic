#include "Font.h"

#include <SDL_ttf.h>
#include <stdlib.h> // for calloc

SDL_Point DrawText(SDL_Renderer* renderer, Font* font, const char* text,
				   int x, int y,
				   int halign, int valign,
				   SDL_Color color) {
	if (!font) return {};
	if (!font->texture) return {};
	if (!font->glyphs) return {};

	if (halign != HALIGN_LEFT || valign != VALIGN_TOP) {
		SDL_Point m = MeasureText(font, text);
		if (halign == HALIGN_CENTER) {
			x -= m.x / 2;
		} else if (halign == HALIGN_RIGHT) {
			x -= m.x;
		}

		if (valign == VALIGN_MIDDLE) {
			y -= m.y / 2;
		} else if (valign == VALIGN_BOTTOM) {
			y -= m.y;
		}
	}

	int text_x = x;
	int text_y = y;

	SDL_SetTextureColorMod(font->texture, color.r, color.g, color.b);

	for (const char* it = text; *it; it++) {
		char ch = *it;
		if (ch == '\n') {
			text_x = x;
			text_y += font->lineskip;
			continue;
		}

		if (ch < 32 || ch > 126) {
			ch = '?';
		}

		GlyphData* glyph = &font->glyphs[ch - 32];

		if (ch != ' ') {
			SDL_Rect src = glyph->src;

			SDL_Rect dest;
			dest.x = text_x + glyph->xoffset;
			dest.y = text_y + glyph->yoffset;
			dest.w = src.w;
			dest.h = src.h;

			SDL_RenderCopy(renderer, font->texture, &src, &dest);
		}
		
		text_x += glyph->advance;
	}

	return {text_x, text_y};
}

SDL_Point DrawTextShadow(SDL_Renderer* renderer, Font* font, const char* text,
						 int x, int y,
						 int halign, int valign,
						 SDL_Color color) {
	DrawText(renderer, font, text, x + 1, y + 1, halign, valign, {0, 0, 0, 255});
	return DrawText(renderer, font, text, x, y, halign, valign, color);
}

static int max(int a, int b) { return (a > b) ? a : b; }
static int min(int a, int b) { return (a < b) ? a : b; }

SDL_Point MeasureText(Font* font, const char* text) {
	if (!font) return {};
	if (!font->texture) return {};
	if (!font->glyphs) return {};

	int text_x = 0;
	int text_y = 0;

	int text_w = 0;
	int text_h = font->height;

	for (const char* it = text; *it; it++) {
		char ch = *it;
		if (ch == '\n') {
			text_x = 0;
			text_y += font->lineskip;
			text_h = max(text_h, text_y + font->height);
			continue;
		}

		if (ch < 32 || ch > 126) {
			ch = '?';
		}

		GlyphData* glyph = &font->glyphs[ch - 32];

		if (ch == ' ') {
			text_w = max(text_w, text_x + glyph->advance);
			text_h = max(text_h, text_y + font->height);
		} else {
			SDL_Rect src = glyph->src;

			SDL_Rect dest;
			dest.x = text_x + glyph->xoffset;
			dest.y = text_y + glyph->yoffset;
			dest.w = src.w;
			dest.h = src.h;

			text_w = max(text_w, dest.x + dest.w);
			text_h = max(text_h, text_y + font->height);
		}

		text_x += glyph->advance;
	}

	return {text_w, text_h};
}

void LoadFontFromFileTTF(Font* font, const char* fname, int ptsize, SDL_Renderer* renderer) {
	TTF_Font* ttf_font = nullptr;
	SDL_Surface* atlas_surf = nullptr;

	{
		ttf_font = TTF_OpenFont(fname, ptsize);

		if (!ttf_font) {
			goto out;
		}

		font->ptsize = ptsize;
		font->glyphs = (GlyphData*) calloc(95, sizeof(*font->glyphs));

		if (!font->glyphs) {
			goto out;
		}

		int atlas_width = 256;
		int atlas_height = 256;

		atlas_surf = SDL_CreateRGBSurfaceWithFormat(0, atlas_width, atlas_height, 32, SDL_PIXELFORMAT_ARGB8888);

		if (!atlas_surf) {
			goto out;
		}

		font->height = TTF_FontHeight(ttf_font);
		font->ascent = TTF_FontAscent(ttf_font);
		font->descent = TTF_FontDescent(ttf_font);
		font->lineskip = TTF_FontLineSkip(ttf_font);

		{
			int minx;
			int maxx;
			int miny;
			int maxy;
			int advance;
			TTF_GlyphMetrics(ttf_font, ' ', &minx, &maxx, &miny, &maxy, &advance);

			font->glyphs[0].advance = advance;
		}

		int x = 0;
		int y = 0;

		for (unsigned char ch = 33; ch <= 126; ch++) {
			SDL_Surface* glyph_surf = TTF_RenderGlyph_Blended(ttf_font, ch, {255, 255, 255, 255});

			//SDL_GetPixelFormatName(glyph_surf->format->format);

			int minx;
			int maxx;
			int miny;
			int maxy;
			int advance;
			TTF_GlyphMetrics(ttf_font, ch, &minx, &maxx, &miny, &maxy, &advance);

			if (x + maxx - minx > atlas_surf->w) {
				y += font->lineskip;
				x = 0;
			}

			SDL_Rect src{minx, font->ascent - maxy, maxx - minx, maxy - miny};
			SDL_Rect dest{x, y, maxx - minx, maxy - miny};
			SDL_BlitSurface(glyph_surf, &src, atlas_surf, &dest);

			int w = maxx - minx;
			int h = maxy - miny;
			int xoffset = minx;
			int yoffset = font->ascent - maxy;

			font->glyphs[ch - 32].src = {x, y, w, h};
			font->glyphs[ch - 32].xoffset = xoffset;
			font->glyphs[ch - 32].yoffset = yoffset;
			font->glyphs[ch - 32].advance = advance;

			x += maxx - minx;

			SDL_FreeSurface(glyph_surf);
		}

		font->texture = SDL_CreateTextureFromSurface(renderer, atlas_surf);

		if (!font->texture) {
			goto out;
		}
	}

out:
	if (atlas_surf) SDL_FreeSurface(atlas_surf);
	if (ttf_font) TTF_CloseFont(ttf_font);
}

void DestroyFont(Font* font) {
	if (font->texture) SDL_DestroyTexture(font->texture);
	font->texture = nullptr;

	if (font->glyphs) free(font->glyphs);
	font->glyphs = nullptr;
}
