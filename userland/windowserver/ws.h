#ifndef WS_H
#define WS_H
#include <stddef.h>
#include <stdint.h>

#define WINDOWSERVER_EVENT_KEYPRESS 0
#define WINDOWSERVER_EVENT_WINDOW_EXIT 1

typedef struct {
  int fd;
  int bitmap_fd;
  uint32_t *bitmap_ptr;
  int x;
  int y;
  int sx;
  int sy;
} WINDOW;

typedef struct {
  int vga_fd;
  int wallpaper_fd;
  uint8_t *wallpaper_buffer;
  uint8_t *back_buffer;
  uint8_t *true_buffer;
  uint32_t size;
  uint32_t width;
  uint32_t height;
  uint32_t bpp;
  uint8_t border_size;
  uint8_t border_color;
  uint8_t wallpaper_color;
  WINDOW *active_window;
  WINDOW **windows;
} DISPLAY;
#endif
