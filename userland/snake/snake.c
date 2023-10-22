#include <assert.h>
#include <fcntl.h>
#include <json.h>
#include <libgui.h>
#include <poll.h>
#include <pty.h>
#include <socket.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>

GUI_Window *global_w;

#define SNAKE_WIDTH 10

int get_key_event(char *c) {
  if (0 >= read(global_w->ws_socket, c, 1))
    return 0;
  if (0 == *c)
    return 0;
  return 1;
}

void draw_snake(int x, int y) {
  int sx = global_w->sx;
  uint32_t *b = &global_w->bitmap_ptr[y * sx + x];
  for (int i = 0; i < SNAKE_WIDTH; i++) {
    for (int j = 0; j < SNAKE_WIDTH; j++) {
      b[j] = 0xFF00FF;
    }
    b += sx;
  }
}

int clamp(int *x, int *y) {
  int clamped = 0;
  if (*x + SNAKE_WIDTH > global_w->sx) {
    *x = global_w->sx - SNAKE_WIDTH;
    clamped = 1;
  }
  if (*y + SNAKE_WIDTH > global_w->sy) {
    *y = global_w->sy - SNAKE_WIDTH;
    clamped = 1;
  }
  return clamped;
}

void loop() {
  int vel = SNAKE_WIDTH;
  static int y_dir = SNAKE_WIDTH;
  static int x_dir = 0;
  static int snake_x = 10;
  static int snake_y = 10;
  static int ticks = 0;
  ticks++;
  char c;
  if (get_key_event(&c)) {
    switch (c) {
    case 'w':
      y_dir = -1 * vel;
      x_dir = 0;
      break;
    case 's':
      y_dir = vel;
      x_dir = 0;
      break;
    case 'a':
      x_dir = -1 * vel;
      y_dir = 0;
      break;
    case 'd':
      x_dir = vel;
      y_dir = 0;
      break;
    case 'q':
      exit(0);
      break;
    default:
      break;
    }
  }

  if (!(ticks > 5))
    return;
  ticks = 0;

  snake_x += x_dir;
  snake_y += y_dir;
  if (clamp(&snake_x, &snake_y))
    return;
  draw_snake(snake_x, snake_y);
  if (y_dir || x_dir)
    GUI_UpdateWindow(global_w);
}

int main(void) {
  global_w = GUI_CreateWindow(10, 10, 150 * 4, 150 * 4);
  assert(global_w);
  for (int i = 0; i < 150 * 150 * 4 * 4; i++)
    global_w->bitmap_ptr[i] = 0xFFFFFF;
  GUI_UpdateWindow(global_w);
  for (;;) {
    loop();
    msleep(50);
  }
  return 0;
}
