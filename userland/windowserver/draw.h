#ifndef DRAW_H
#define DRAW_H
#include "ws.h"

#define WIDTH 0x500
#define HEIGHT 0x320
#define BPP 4

void draw_wallpaper(DISPLAY *disp);
void draw_window(DISPLAY *disp, const WINDOW *w);
void update_full_display(DISPLAY *disp, int mouse_x, int mouse_y);
void update_active_window(DISPLAY *disp);
void draw_mouse(DISPLAY *disp, int mouse_x, int mouse_y);
#endif
