#include "draw.h"
#include <string.h>

#define place_pixel(_p, _w, _h)                                                \
  {                                                                            \
    *(uint32_t *)(disp->back_buffer + disp->bpp * (_w) +                       \
                  (disp->width * disp->bpp * (_h))) = _p;                      \
  }

#define place_pixel_pos(_p, _pos)                                              \
  { *(uint32_t *)(disp->back_buffer + disp->bpp * (_pos)) = _p; }

int mx;
int my;

void update_display(const DISPLAY *disp) {
  for (int i = 0; i < 20; i++) {
    uint32_t color = 0xFFFFFFFF;
    // Make every other pixel black to make the mouse more visible on all
    // backgrounds.
    if (i & 1) {
      color = 0x0;
    }
    place_pixel(color, mx + i, my + i);
    if (i <= 10) {
      place_pixel(color, mx, my + i);
      place_pixel(color, mx + i, my);
    }
  }
  uint32_t *dst = disp->true_buffer;
  uint32_t *src = disp->back_buffer;
  const uint32_t n = disp->size / disp->bpp;
  for (int i = 0; i < n; i++) {
    *dst = *src;
    dst++;
    src++;
  }
}

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

void draw_mouse(DISPLAY *disp, int mouse_x, int mouse_y) {
  mx = mouse_x;
  my = mouse_y;
  update_full_display(disp, mouse_x, mouse_y);
}

void draw_line(DISPLAY *disp, int sx, int sy, int dx, int dy, uint32_t color) {
  int x = sx;
  int y = sy;
  for (;;) {
    // Bounds checking
    if (y * disp->width + x > disp->height * disp->width)
      break;

    if (x >= dx - 1 && y >= dy - 1)
      break;
    uint32_t *ptr =
        disp->back_buffer + (disp->bpp * y * disp->width) + (x * disp->bpp);
    *ptr = color;
    if (x < dx - 1)
      x++;
    if (y < dy - 1)
      y++;
  }
}

void draw_window(DISPLAY *disp, const WINDOW *w) {
  int x, y;
  uint8_t border_size = disp->border_size;
  const int px = w->x + border_size;
  const int py = w->y;
  const int sx = w->sx;
  const int sy = w->sy;
  const int b_sx = w->buffer_sx;
  const int b_sy = w->buffer_sy;
  x = px;
  y = py;
  // Draw a border around the current selected window
  if (w == disp->active_window) {
    // Top
    draw_line(disp, px - 1, py - 1, px + sx + 1, py - 1, 0xFF00FF);
    // Bottom
    draw_line(disp, px - 1, py + sy, px + sx + 1, py + sy, 0xFF00FF);
    // Left
    draw_line(disp, px - 1, py - 1, px - 1, py + sy + 1, 0xFF00FF);
    // Right
    draw_line(disp, px + sx, py - 1, px + sx, py + sy + 1, 0xFF00FF);
  }

  for (int i = 0; i < sy; i++) {
    if ((i + py) * disp->width + px > disp->height * disp->width)
      break;
    uint32_t *ptr = disp->back_buffer + disp->bpp * ((i + py) * disp->width) +
                    px * disp->bpp;
    if (i * sx > disp->height * disp->width)
      break;
    if (i < b_sy) {
      uint32_t *bm = &w->bitmap_ptr[i * b_sx];
      int j = 0;
      for (; j < b_sx && j < sx; j++) {
        ptr[j] = bm[j];
      }
      for (; j < sx; j++) {
        ptr[j] = 0;
      }
    } else {
      for (int j = 0; j < sx; j++) {
        ptr[j] = 0;
      }
    }
  }
}

void update_active_window(DISPLAY *disp) {
  for (int i = 0; i < 100; i++) {
    if (!disp->windows[i])
      continue;
    if (!disp->windows[i]->bitmap_ptr)
      continue;
    draw_window(disp, disp->windows[i]);
  }
}

void update_full_display(DISPLAY *disp, int mouse_x, int mouse_y) {
  draw_wallpaper(disp);
  update_active_window(disp);
  update_display(disp);
}
