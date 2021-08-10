//---
// fxlink:sdl2 - SDL2 functions
//---

#ifndef FXLINK_SDL2_H
#define FXLINK_SDL2_H

#ifndef FXLINK_DISABLE_SDL2

#include <SDL2/SDL.h>

/* sdl2_stream(): Display a streaming image on the window
   This function opens or reuses an SDL2 window to display an image. */
void sdl2_stream(uint8_t **RGB888_rows, int width, int height);

/* sdl2_tick(): Handle SDL events
   This just needs to be called regularly from the main thread, to respond to
   events on the window. */
void sdl2_tick(void);

#endif /* FXLINK_DISABLE_SDL2 */

#endif /* FXLINK_SDL2_H */
