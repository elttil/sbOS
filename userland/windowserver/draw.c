#include "draw.h"
#include <string.h>

#define BPP 4

#define place_pixel(_p, _w, _h)                                                \
  { *(uint32_t *)(disp->back_buffer + BPP * (_w) + (WIDTH * BPP * (_h))) = _p; }

#define place_pixel_pos(_p, _pos)                                              \
  { *(uint32_t *)(disp->back_buffer + BPP * (_pos)) = _p; }

int mx;
int my;

void update_display(DISPLAY *disp) {
  for (int i = 0; i < 20; i++) {
    place_pixel(0xFFFFFFFF, mx + i, my + i);
    place_pixel(0xFFFFFFFF, mx, my + i / 2);
    place_pixel(0xFFFFFFFF, mx + i / 2, my);
  }
  for (int i = 0; i < disp->size; i++)
    disp->true_buffer[i] = disp->back_buffer[i];
  //  memcpy(disp->true_buffer, disp->back_buffer, disp->size);
}

void draw_wallpaper(DISPLAY *disp) {
  for (int i = 0; i < disp->size / BPP; i++) {
    place_pixel_pos(0xFF00FF, i);
  }
}

void draw_mouse(DISPLAY *disp, int mouse_x, int mouse_y) {
  mx = mouse_x;
  my = mouse_y;
  update_full_display(disp, mouse_x, mouse_y);
}

void draw_window(DISPLAY *disp, const WINDOW *w) {
  int x, y;
  uint8_t border_size = disp->border_size;
  const int px = w->x + border_size;
  const int py = w->y;
  const int sx = w->sx;
  const int sy = w->sy;
  x = px;
  y = py;
  for (int i = 0; i < sy; i++) {
    if((i+py)*WIDTH + px > HEIGHT*WIDTH)
	    break;
    uint32_t *ptr = disp->back_buffer + BPP * ((i + py) * WIDTH) + px * BPP;
    if(i*sx > HEIGHT*WIDTH)
	    break;
    uint32_t *bm = &w->bitmap_ptr[i * sx];
    //        ((uint8_t *)w->bitmap_ptr) + BPP * ((i + py) * WIDTH) + px * BPP;
    for (int j = 0; j < sx; j++) {
      ptr[j] = bm[j];
    }
  }
}

void update_active_window(DISPLAY *disp) {
  for (int i = 0; i < 100; i++) {
    if (!disp->clients[i])
      continue;
    if (!disp->clients[i]->w)
      continue;
    draw_window(disp, disp->clients[i]->w);
  }
}

void update_full_display(DISPLAY *disp, int mouse_x, int mouse_y) {
  draw_wallpaper(disp); /*
   for (int i = 0; i < 100; i++) {
     if (!disp->windows[i])
       continue;
     draw_window(disp, disp->windows[i]);
   }*/
  update_active_window(disp);
  update_display(disp);
}
