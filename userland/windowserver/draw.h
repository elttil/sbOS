#ifndef DRAW_H
#define DRAW_H
#include "ws.h"

void draw_wallpaper(DISPLAY *disp);
void draw_window(DISPLAY *disp, const WINDOW *w);
void update_full_display(DISPLAY *disp, int mouse_x, int mouse_y);
void update_active_window(DISPLAY *disp);
void draw_mouse(DISPLAY *disp, int mouse_x, int mouse_y);
#endif
