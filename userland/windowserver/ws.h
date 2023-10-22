#ifndef WS_H
#define WS_H
#include <stddef.h>
#include <stdint.h>

typedef struct {
  int bitmap_fd;
  uint32_t *bitmap_ptr;
  int x;
  int y;
  int sx;
  int sy;
} WINDOW;

typedef struct {
  int fd;
  WINDOW *w;
} CLIENT;

typedef struct {
  int vga_fd;
  uint8_t *back_buffer;
  uint8_t *true_buffer;
  size_t size;
  uint8_t border_size;
  uint8_t border_color;
  uint8_t wallpaper_color;
  CLIENT **clients;
} DISPLAY;
#endif
