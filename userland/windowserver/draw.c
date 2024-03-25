#include "draw.h"
#include <math.h>
#include <string.h>

void draw_wallpaper(const DISPLAY *disp) {
  uint32_t *dst = disp->back_buffer;
  uint32_t *src = disp->wallpaper_buffer;
  const uint32_t n = disp->size / disp->bpp;
  for (int i = 0; i < n; i++) {
    *dst = *src;
    dst++;
    src++;
  }
}

void draw_outline(DISPLAY *disp, int x, int y, int sx, int sy, int border_px,
                  uint32_t color) {
  // Top
  draw_rectangle(disp, x, y, sx + border_px, border_px, color);
  // Bottom
  draw_rectangle(disp, x, y + sy+border_px, sx + border_px, border_px, color);
  // Left
  draw_rectangle(disp, x, y, border_px, sy + border_px * 2, color);
  // Right
  draw_rectangle(disp, x + sx + border_px, y, border_px, sy + border_px * 2,
                 color);
}

void draw_line(DISPLAY *disp, int sx, int sy, int dx, int dy, uint32_t color) {
  int x = sx;
  int y = sy;
  for (;;) {
    // Bounds checking
    if (y * disp->width + x > disp->height * disp->width)
      break;

    if (x >= dx - 1 && y >= dy - 1) {
      break;
    }
    uint32_t *ptr =
        disp->back_buffer + (disp->bpp * y * disp->width) + (x * disp->bpp);
    *ptr = color;
    if (x < dx - 1) {
      x++;
    }
    if (y < dy - 1) {
      y++;
    }
  }
}

void draw_rectangle(DISPLAY *disp, int x, int y, int sx, int sy,
                    uint32_t color) {
  for (int i = y; i < y + sy; i++) {
    for (int j = x; j < x + sx; j++) {
      // Bounds checking
      if (i * disp->width + j > disp->height * disp->width) {
        break;
      }

      uint32_t *ptr =
          disp->back_buffer + (disp->bpp * i * disp->width) + (j * disp->bpp);
      *ptr = color;
    }
  }
}
