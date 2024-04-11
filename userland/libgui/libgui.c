#include "libgui.h"
#include "font.h"
#include <assert.h>
#include <fcntl.h>
#include <poll.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define place_pixel_pos(_p, _pos)                                              \
  { *(uint32_t *)(w->bitmap_ptr + _pos) = _p; }

static const unsigned char font8x8_basic[128][8] = {
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0000 (nul)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0001
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0002
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0003
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0004
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0005
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0006
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0007
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0008
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0009
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000A
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000B
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000C
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000D
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000E
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+000F
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0010
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0011
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0012
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0013
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0014
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0015
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0016
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0017
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0018
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0019
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001A
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001B
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001C
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001D
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001E
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+001F
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0020 (space)
    {0x18, 0x3C, 0x3C, 0x18, 0x18, 0x00, 0x18, 0x00}, // U+0021 (!)
    {0x36, 0x36, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0022 (")
    {0x36, 0x36, 0x7F, 0x36, 0x7F, 0x36, 0x36, 0x00}, // U+0023 (#)
    {0x0C, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x0C, 0x00}, // U+0024 ($)
    {0x00, 0x63, 0x33, 0x18, 0x0C, 0x66, 0x63, 0x00}, // U+0025 (%)
    {0x1C, 0x36, 0x1C, 0x6E, 0x3B, 0x33, 0x6E, 0x00}, // U+0026 (&)
    {0x06, 0x06, 0x03, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0027 (')
    {0x18, 0x0C, 0x06, 0x06, 0x06, 0x0C, 0x18, 0x00}, // U+0028 (()
    {0x06, 0x0C, 0x18, 0x18, 0x18, 0x0C, 0x06, 0x00}, // U+0029 ())
    {0x00, 0x66, 0x3C, 0xFF, 0x3C, 0x66, 0x00, 0x00}, // U+002A (*)
    {0x00, 0x0C, 0x0C, 0x3F, 0x0C, 0x0C, 0x00, 0x00}, // U+002B (+)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x06}, // U+002C (,)
    {0x00, 0x00, 0x00, 0x3F, 0x00, 0x00, 0x00, 0x00}, // U+002D (-)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // U+002E (.)
    {0x60, 0x30, 0x18, 0x0C, 0x06, 0x03, 0x01, 0x00}, // U+002F (/)
    {0x3E, 0x63, 0x73, 0x7B, 0x6F, 0x67, 0x3E, 0x00}, // U+0030 (0)
    {0x0C, 0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x3F, 0x00}, // U+0031 (1)
    {0x1E, 0x33, 0x30, 0x1C, 0x06, 0x33, 0x3F, 0x00}, // U+0032 (2)
    {0x1E, 0x33, 0x30, 0x1C, 0x30, 0x33, 0x1E, 0x00}, // U+0033 (3)
    {0x38, 0x3C, 0x36, 0x33, 0x7F, 0x30, 0x78, 0x00}, // U+0034 (4)
    {0x3F, 0x03, 0x1F, 0x30, 0x30, 0x33, 0x1E, 0x00}, // U+0035 (5)
    {0x1C, 0x06, 0x03, 0x1F, 0x33, 0x33, 0x1E, 0x00}, // U+0036 (6)
    {0x3F, 0x33, 0x30, 0x18, 0x0C, 0x0C, 0x0C, 0x00}, // U+0037 (7)
    {0x1E, 0x33, 0x33, 0x1E, 0x33, 0x33, 0x1E, 0x00}, // U+0038 (8)
    {0x1E, 0x33, 0x33, 0x3E, 0x30, 0x18, 0x0E, 0x00}, // U+0039 (9)
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x00}, // U+003A (:)
    {0x00, 0x0C, 0x0C, 0x00, 0x00, 0x0C, 0x0C, 0x06}, // U+003B (;)
    {0x18, 0x0C, 0x06, 0x03, 0x06, 0x0C, 0x18, 0x00}, // U+003C (<)
    {0x00, 0x00, 0x3F, 0x00, 0x00, 0x3F, 0x00, 0x00}, // U+003D (=)
    {0x06, 0x0C, 0x18, 0x30, 0x18, 0x0C, 0x06, 0x00}, // U+003E (>)
    {0x1E, 0x33, 0x30, 0x18, 0x0C, 0x00, 0x0C, 0x00}, // U+003F (?)
    {0x3E, 0x63, 0x7B, 0x7B, 0x7B, 0x03, 0x1E, 0x00}, // U+0040 (@)
    {0x0C, 0x1E, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x00}, // U+0041 (A)
    {0x3F, 0x66, 0x66, 0x3E, 0x66, 0x66, 0x3F, 0x00}, // U+0042 (B)
    {0x3C, 0x66, 0x03, 0x03, 0x03, 0x66, 0x3C, 0x00}, // U+0043 (C)
    {0x1F, 0x36, 0x66, 0x66, 0x66, 0x36, 0x1F, 0x00}, // U+0044 (D)
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x46, 0x7F, 0x00}, // U+0045 (E)
    {0x7F, 0x46, 0x16, 0x1E, 0x16, 0x06, 0x0F, 0x00}, // U+0046 (F)
    {0x3C, 0x66, 0x03, 0x03, 0x73, 0x66, 0x7C, 0x00}, // U+0047 (G)
    {0x33, 0x33, 0x33, 0x3F, 0x33, 0x33, 0x33, 0x00}, // U+0048 (H)
    {0x1E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0049 (I)
    {0x78, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E, 0x00}, // U+004A (J)
    {0x67, 0x66, 0x36, 0x1E, 0x36, 0x66, 0x67, 0x00}, // U+004B (K)
    {0x0F, 0x06, 0x06, 0x06, 0x46, 0x66, 0x7F, 0x00}, // U+004C (L)
    {0x63, 0x77, 0x7F, 0x7F, 0x6B, 0x63, 0x63, 0x00}, // U+004D (M)
    {0x63, 0x67, 0x6F, 0x7B, 0x73, 0x63, 0x63, 0x00}, // U+004E (N)
    {0x1C, 0x36, 0x63, 0x63, 0x63, 0x36, 0x1C, 0x00}, // U+004F (O)
    {0x3F, 0x66, 0x66, 0x3E, 0x06, 0x06, 0x0F, 0x00}, // U+0050 (P)
    {0x1E, 0x33, 0x33, 0x33, 0x3B, 0x1E, 0x38, 0x00}, // U+0051 (Q)
    {0x3F, 0x66, 0x66, 0x3E, 0x36, 0x66, 0x67, 0x00}, // U+0052 (R)
    {0x1E, 0x33, 0x07, 0x0E, 0x38, 0x33, 0x1E, 0x00}, // U+0053 (S)
    {0x3F, 0x2D, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0054 (T)
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x33, 0x3F, 0x00}, // U+0055 (U)
    {0x33, 0x33, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // U+0056 (V)
    {0x63, 0x63, 0x63, 0x6B, 0x7F, 0x77, 0x63, 0x00}, // U+0057 (W)
    {0x63, 0x63, 0x36, 0x1C, 0x1C, 0x36, 0x63, 0x00}, // U+0058 (X)
    {0x33, 0x33, 0x33, 0x1E, 0x0C, 0x0C, 0x1E, 0x00}, // U+0059 (Y)
    {0x7F, 0x63, 0x31, 0x18, 0x4C, 0x66, 0x7F, 0x00}, // U+005A (Z)
    {0x1E, 0x06, 0x06, 0x06, 0x06, 0x06, 0x1E, 0x00}, // U+005B ([)
    {0x03, 0x06, 0x0C, 0x18, 0x30, 0x60, 0x40, 0x00}, // U+005C (\)
    {0x1E, 0x18, 0x18, 0x18, 0x18, 0x18, 0x1E, 0x00}, // U+005D (])
    {0x08, 0x1C, 0x36, 0x63, 0x00, 0x00, 0x00, 0x00}, // U+005E (^)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0xFF}, // U+005F (_)
    {0x0C, 0x0C, 0x18, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+0060 (`)
    {0x00, 0x00, 0x1E, 0x30, 0x3E, 0x33, 0x6E, 0x00}, // U+0061 (a)
    {0x07, 0x06, 0x06, 0x3E, 0x66, 0x66, 0x3B, 0x00}, // U+0062 (b)
    {0x00, 0x00, 0x1E, 0x33, 0x03, 0x33, 0x1E, 0x00}, // U+0063 (c)
    {0x38, 0x30, 0x30, 0x3e, 0x33, 0x33, 0x6E, 0x00}, // U+0064 (d)
    {0x00, 0x00, 0x1E, 0x33, 0x3f, 0x03, 0x1E, 0x00}, // U+0065 (e)
    {0x1C, 0x36, 0x06, 0x0f, 0x06, 0x06, 0x0F, 0x00}, // U+0066 (f)
    {0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x1F}, // U+0067 (g)
    {0x07, 0x06, 0x36, 0x6E, 0x66, 0x66, 0x67, 0x00}, // U+0068 (h)
    {0x0C, 0x00, 0x0E, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+0069 (i)
    {0x30, 0x00, 0x30, 0x30, 0x30, 0x33, 0x33, 0x1E}, // U+006A (j)
    {0x07, 0x06, 0x66, 0x36, 0x1E, 0x36, 0x67, 0x00}, // U+006B (k)
    {0x0E, 0x0C, 0x0C, 0x0C, 0x0C, 0x0C, 0x1E, 0x00}, // U+006C (l)
    {0x00, 0x00, 0x33, 0x7F, 0x7F, 0x6B, 0x63, 0x00}, // U+006D (m)
    {0x00, 0x00, 0x1F, 0x33, 0x33, 0x33, 0x33, 0x00}, // U+006E (n)
    {0x00, 0x00, 0x1E, 0x33, 0x33, 0x33, 0x1E, 0x00}, // U+006F (o)
    {0x00, 0x00, 0x3B, 0x66, 0x66, 0x3E, 0x06, 0x0F}, // U+0070 (p)
    {0x00, 0x00, 0x6E, 0x33, 0x33, 0x3E, 0x30, 0x78}, // U+0071 (q)
    {0x00, 0x00, 0x3B, 0x6E, 0x66, 0x06, 0x0F, 0x00}, // U+0072 (r)
    {0x00, 0x00, 0x3E, 0x03, 0x1E, 0x30, 0x1F, 0x00}, // U+0073 (s)
    {0x08, 0x0C, 0x3E, 0x0C, 0x0C, 0x2C, 0x18, 0x00}, // U+0074 (t)
    {0x00, 0x00, 0x33, 0x33, 0x33, 0x33, 0x6E, 0x00}, // U+0075 (u)
    {0x00, 0x00, 0x33, 0x33, 0x33, 0x1E, 0x0C, 0x00}, // U+0076 (v)
    {0x00, 0x00, 0x63, 0x6B, 0x7F, 0x7F, 0x36, 0x00}, // U+0077 (w)
    {0x00, 0x00, 0x63, 0x36, 0x1C, 0x36, 0x63, 0x00}, // U+0078 (x)
    {0x00, 0x00, 0x33, 0x33, 0x33, 0x3E, 0x30, 0x1F}, // U+0079 (y)
    {0x00, 0x00, 0x3F, 0x19, 0x0C, 0x26, 0x3F, 0x00}, // U+007A (z)
    {0x38, 0x0C, 0x0C, 0x07, 0x0C, 0x0C, 0x38, 0x00}, // U+007B ({)
    {0x18, 0x18, 0x18, 0x00, 0x18, 0x18, 0x18, 0x00}, // U+007C (|)
    {0x07, 0x0C, 0x0C, 0x38, 0x0C, 0x0C, 0x07, 0x00}, // U+007D (})
    {0x6E, 0x3B, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}, // U+007E (~)
    {0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00}  // U+007F
};

#define pokeb(S, O, V) (*(unsigned char *)((S) + (O)) = (V))

// Very temporary
char *random_string(void) {
  int fd = open("/dev/urandom", O_RDONLY, 0);
  char *r = malloc(sizeof(char[10]));
  for (int i = 0; i < 10 - 1; i++) {
    char c;
    read(fd, &c, 1);
    r[i] = 'a' + (c & 0xF);
  }
  close(fd);
  r[9] = '\0';
  return r;
}

int get_bitmap_value(const unsigned char bitmap[], int i) {
  int array_index = i / 8;
  int byte_index = i % 8;
  int rc = (bitmap[array_index] >> byte_index) & 0x1;
  return rc;
}

void GUI_OverwriteFont(GUI_Window *w, uint32_t px, uint32_t py,
                       const uint32_t color) {
  int sx, sy;
  sx = 8;
  sy = 8;
  int x, y;
  x = px;
  y = py;
  if (px + sx > w->sx)
    return;
  if (py + sy > w->sy)
    return;
  for (int i = 0; i < sx * sy; i++) {
    int pos = x + y * w->sx;
    place_pixel_pos(color, pos);
    x++;
    if (x >= sx + px) {
      y++;
      x = px;
    }
    if (y > py + sy)
      break;
  }
}

void GUI_DrawFont(GUI_Window *w, uint32_t px, uint32_t py, const uint32_t c) {
  int sx, sy;
  sx = 8;
  sy = 8;
  int x, y;
  x = px;
  y = py;
  if (px + sx > w->sx)
    return;
  if (py + sy > w->sy)
    return;
  const unsigned char *bitmap = font8x8_basic[c];
  for (int i = 0; i < sx * sy; i++) {
    int pos = x + y * w->sx;
    if (get_bitmap_value(bitmap, i)) {
      place_pixel_pos(0xFFFFFF, pos);
    } else {
      place_pixel_pos(0x0, pos);
    }
    x++;
    if (x >= sx + px) {
      y++;
      x = px;
    }
    if (y > py + sy)
      break;
  }
}

typedef struct {
  uint16_t px;
  uint16_t py;
  uint16_t sx;
  uint16_t sy;
  uint8_t name_len;
} WS_EVENT_CREATE;

void GUI_ClearScreen(GUI_Window *w, uint32_t color) {
  for (int i = 0; i < w->sx * w->sy; i++)
    w->bitmap_ptr[i] = color;
}

void GUI_EventLoop(GUI_Window *w, void (*event_handler)(WS_EVENT ev)) {
  struct pollfd fds[1];
  fds[0].fd = w->ws_socket;
  fds[0].events = POLLIN;
  fds[0].revents = 0;
  for (;; fds[0].revents = 0) {
    poll(fds, 1, 0);
    if (!(fds[0].revents & POLLIN))
      continue;

    int rc;
    WS_EVENT e;
    if (0 >= (rc = read(w->ws_socket, &e, sizeof(e))))
      continue;
    if (event_handler) {
      event_handler(e);
      continue;
    }
    if (WINDOWSERVER_EVENT_WINDOW_EXIT == e.type)
      exit(0);
  }
}

// This should really not be a constant. It is merely here so that each
// window can be given a sufficently large buffer such that buffers
// don't have to be resized if the window is resized.
#define MAX_WINDOW_SIZE (1920 * 1080)
void GUI_Resize(GUI_Window *w, uint32_t sx, uint32_t sy) {
  if (sx * sy > MAX_WINDOW_SIZE) {
    return;
  }

  w->sx = sx;
  w->sy = sy;
  char buffer[sizeof(uint8_t) + sizeof(uint32_t) * 2];
  uint8_t l = 2;
  memcpy(buffer, &l, sizeof(l));
  memcpy(buffer + sizeof(uint8_t), &sx, sizeof(sx));
  memcpy(buffer + sizeof(uint8_t) + sizeof(uint32_t), &sy, sizeof(sy));
  int len = sizeof(uint8_t) + sizeof(uint32_t) * 2;
  write(w->ws_socket, buffer, len);
}

GUI_Window *GUI_CreateWindow(uint32_t x, uint32_t y, uint32_t sx, uint32_t sy) {
  GUI_Window *w = malloc(sizeof(GUI_Window));
  if (!w)
    return NULL;
  w->x = x;
  w->y = y;
  w->sx = sx;
  w->sy = sy;

  // Connect to the windowserver
  int ws_fd = -1;
  for (; - 1 == ws_fd;) {
    ws_fd = open("/dev/windowserver", O_RDWR | O_NONBLOCK, 0);
  }
  w->ws_socket = ws_fd;
  char *str = random_string();
  WS_EVENT_CREATE e;
  e.px = x;
  e.py = y;
  e.sx = sx;
  e.sy = sy;
  e.name_len = (uint8_t)strlen(str) + 1;

  // Create bitmap
  w->bitmap_fd = shm_open(str, O_RDWR, 0);
  if (!((int)w->bitmap_fd >= 0)) {
    printf("bitmap_fd: %x\n", w->bitmap_fd);
    assert(0);
  }
  ftruncate(w->bitmap_fd, MAX_WINDOW_SIZE * sizeof(uint32_t));
  void *rc =
      mmap(NULL, MAX_WINDOW_SIZE * sizeof(uint32_t), 0, 0, w->bitmap_fd, 0);
  if (!((int)rc >= 0)) {
    printf("rc: %x\n", rc);
    assert(0);
  }
  w->bitmap_ptr = rc;

  // Send the request to the windowserver
  uint8_t l = 1;
  uint8_t len = sizeof(l) + sizeof(e) + e.name_len;
  char *buffer = malloc(len);
  char *p = buffer;
  memcpy(p, &l, sizeof(l));
  p += sizeof(l);
  memcpy(p, &e, sizeof(e));
  p += sizeof(e);
  strcpy(p, str);
  write(ws_fd, buffer, len);
  return w;
}

void GUI_UpdateWindow(GUI_Window *w) {
  uint8_t l = 0;
  write(w->ws_socket, &l, sizeof(l));
}
