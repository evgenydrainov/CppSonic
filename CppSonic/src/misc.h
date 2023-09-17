#define ArrayLength(a) (sizeof(a) / sizeof(*a))

static double GetTime() {
	return (double)SDL_GetPerformanceCounter() / (double)SDL_GetPerformanceFrequency();
}
