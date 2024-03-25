#ifndef LIBGUI_H
#define LIBGUI_H
#include <stddef.h>
#include <stdint.h>

#define WINDOWSERVER_EVENT_KEYPRESS 0
#define WINDOWSERVER_EVENT_WINDOW_EXIT 1
#define WINDOWSERVER_EVENT_WINDOW_RESIZE 2

typedef struct {
  int ws_socket;
  int bitmap_fd;
  uint32_t *bitmap_ptr;
  int x;
  int y;
  int sx;
  int sy;
} GUI_Window;

// Taken from drivers/keyboard.c
struct KEY_EVENT {
  char c;
  uint8_t mode;    // (shift (0 bit)) (alt (1 bit)) (ctrl (2 bit))
  uint8_t release; // 0 pressed, 1 released
};

typedef struct {
  int type;
  union {
    struct KEY_EVENT ev;
    int vector[2];
  };
} WS_EVENT;

GUI_Window *GUI_CreateWindow(uint32_t x, uint32_t y, uint32_t sx, uint32_t sy);
void GUI_DrawFont(GUI_Window *w, uint32_t px, uint32_t py, const uint32_t c);
void GUI_UpdateWindow(GUI_Window *w);
void GUI_OverwriteFont(GUI_Window *w, uint32_t px, uint32_t py,
                       const uint32_t color);
void GUI_ClearScreen(GUI_Window *w, uint32_t color);
void GUI_EventLoop(GUI_Window *w, void (*event_handler)(WS_EVENT ev));
void GUI_Resize(GUI_Window *w, uint32_t sx, uint32_t sy);
#endif
