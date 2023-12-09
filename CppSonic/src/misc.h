#pragma once

#include <SDL.h>
#include <stdlib.h>
#include "stb_sprintf.h"

#define ArrayLength(a) (sizeof(a) / sizeof(*a))

static double GetTime() {
	return double(SDL_GetPerformanceCounter()) / double(SDL_GetPerformanceFrequency());
}

#define ErrorMessageBox(...) 									\
	do {														\
		char buf[1024];											\
		stb_snprintf(buf, sizeof(buf), __VA_ARGS__);			\
		SDL_ShowSimpleMessageBox(0, "Error.", buf, nullptr);	\
	} while (0)

static void* ecalloc(size_t count, size_t size) {
	void* result = calloc(count, size);

	if (!result) {
		ErrorMessageBox("Out of memory.");
		exit(1);
	}

	return result;
}
