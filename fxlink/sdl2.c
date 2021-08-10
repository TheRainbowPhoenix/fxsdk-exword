#include "config.h"
#ifndef FXLINK_DISABLE_SDL2

#include "sdl2.h"
#include "util.h"

static SDL_Window *window = NULL;

static int init(size_t width, size_t height)
{
	if(!SDL_WasInit(SDL_INIT_VIDEO)) {
		int rc = SDL_Init(SDL_INIT_VIDEO);
		if(rc < 0)
			return err("Cannot initialize SDL: %s\n", SDL_GetError());
	}

	window = SDL_CreateWindow("fxlink", SDL_WINDOWPOS_CENTERED,
		SDL_WINDOWPOS_CENTERED, width, height, 0);
	return 0;
}

__attribute__((destructor))
static void quit(void)
{
	if(!window)
		return;
	SDL_DestroyWindow(window);
	window = NULL;
}

/* Generate an RGB888 surface from image data. */
static SDL_Surface *surface_for_image(uint8_t **RGB888_rows, int width,
	int height)
{
	/* Little endian setup for RGB */
	SDL_Surface *s = SDL_CreateRGBSurface(0, width, height, 24,
		0x000000ff, 0x0000ff00, 0x0000ff00, 0);
	if(!s) {
		err("Cannot create surface for image");
		return NULL;
	}

	for(int i = 0; i < height; i++)
		memcpy(s->pixels + i * s->pitch, RGB888_rows[i], width * 3);

	return s;
}

void sdl2_stream(uint8_t **RGB888_rows, int width, int height)
{
	if(!window && init(width, height))
		return;

	int current_w, current_h;
	SDL_GetWindowSize(window, &current_w, &current_h);
	if(current_w != width || current_h != height)
		SDL_SetWindowSize(window, width, height);

	SDL_Surface *src = surface_for_image(RGB888_rows, width, height);
	if(!src)
		return;

	SDL_Surface *dst = SDL_GetWindowSurface(window);
	SDL_BlitSurface(src, NULL, dst, NULL);
	SDL_FreeSurface(src);

	SDL_UpdateWindowSurface(window);
}

void sdl2_tick(void)
{
	if(!window)
		return;

	SDL_Event e;
	while(SDL_PollEvent(&e)) {
		if(e.type == SDL_QUIT)
			quit();
	}
}

#endif /* FXLINK_DISABLE_SDL2 */
