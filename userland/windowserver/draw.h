#ifndef DRAW_H
#define DRAW_H
#include "ws.h"

#define place_pixel(_p, _w, _h)                                                \
  {                                                                            \
    *(uint32_t *)(disp->back_buffer + disp->bpp * (_w) +                       \
                  (disp->width * disp->bpp * (_h))) = _p;                      \
  }

#define place_pixel_pos(_p, _pos)                                              \
  { *(uint32_t *)(disp->back_buffer + disp->bpp * (_pos)) = _p; }

void draw_line(DISPLAY *disp, int sx, int sy, int dx, int dy, uint32_t color);
void draw_wallpaper(const DISPLAY *disp);
void draw_rectangle(DISPLAY *disp, int x, int y, int sx, int sy,
                    uint32_t color);
void draw_outline(DISPLAY *disp, int x, int y, int sx, int sy, int border_px, uint32_t color);
#endif
