#include "ws.h"
#include "draw.h"
#include "font.h"
#include <assert.h>
#include <fcntl.h>
#include <math.h>
#include <poll.h>
#include <socket.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define WINDOW_SERVER_SOCKET "/windowserver"

#define RET_CLICKED(_x, _y, _sx, _sy, _id)                                     \
  if (contains(_x, _y, _sx, _sy, mouse_x, mouse_y)) {                          \
    if (mouse_left_down) {                                                     \
      active_id = _id;                                                         \
    } else if (mouse_left_up) {                                                \
      if (_id == active_id) {                                                  \
        return 1;                                                              \
      }                                                                        \
    }                                                                          \
  } else {                                                                     \
    if (_id == active_id && mouse_left_up) {                                   \
      active_id = -1;                                                          \
    }                                                                          \
  }                                                                            \
  return 0;

// Taken from drivers/keyboard.c
struct KEY_EVENT {
  char c;
  uint8_t mode;    // (shift (0 bit)) (alt (1 bit))
  uint8_t release; // 0 pressed, 1 released
};

typedef struct {
  int type;
  union {
    struct KEY_EVENT ev;
    int vector[2];
  };
} WS_EVENT;

WINDOW *window;

int mouse_x = 0;
int mouse_y = 0;
int mouse_left_down = 0;
int mouse_left_up = 0;
int mouse_right_down = 0;
int mouse_right_up = 0;

int active_id = -1;

int ui_id = 0;
int ui_state = 0;

DISPLAY main_display;

struct pollfd *fds;
uint64_t num_fds;

uint64_t socket_fd_poll;
uint64_t keyboard_fd_poll;
uint64_t mouse_fd_poll;

void draw(void);

int create_socket(void) {
  struct sockaddr_un address;
  int fd = socket(AF_UNIX, 0, 0);
  address.sun_family = AF_UNIX;
  size_t str_len = strlen(WINDOW_SERVER_SOCKET);
  address.sun_path = malloc(str_len + 1);
  memcpy(address.sun_path, WINDOW_SERVER_SOCKET, str_len);
  address.sun_path[str_len] = 0;

  bind(fd, (struct sockaddr *)(&address), sizeof(address));
  //  for(;;);
  /*free(address.sun_path);*/
  return fd;
}

int next_ui_id(void) {
  int r = ui_id;
  ui_id++;
  return r;
}

void setup_display(DISPLAY *disp, const char *path, uint64_t size) {
  disp->wallpaper_color = 0x3;
  disp->size = size;
  disp->vga_fd = open(path, O_RDWR, 0);
  if (-1 == disp->vga_fd) {
    perror("open");
    for (;;)
      ;
  }
  disp->true_buffer = mmap(NULL, size, 0, 0, disp->vga_fd, 0);
  disp->back_buffer = malloc(size + 0x1000);
  disp->window = window;

  disp->wallpaper_fd = shm_open("wallpaper", O_RDWR, 0);
  assert(disp->wallpaper_fd >= 0);
  ftruncate(disp->wallpaper_fd, size);
  void *rc = mmap(NULL, size, 0, 0, disp->wallpaper_fd, 0);
  assert(rc);
  disp->wallpaper_buffer = rc;
  for (int i = 0; i < disp->size / disp->bpp; i++) {
    uint32_t *p = disp->wallpaper_buffer + i * sizeof(uint32_t);
    *p = 0xFFFFFF;
  }
}

struct DISPLAY_INFO {
  uint32_t width;
  uint32_t height;
  uint8_t bpp;
};

void setup(void) {
  struct DISPLAY_INFO disp_info;
  {
    int info_fd = open("/dev/display_info", O_READ, 0);
    assert(sizeof(disp_info) == read(info_fd, &disp_info, sizeof(disp_info)));
    close(info_fd);
  }

  main_display.width = disp_info.width;
  main_display.height = disp_info.height;
  main_display.bpp = disp_info.bpp / 8;

  setup_display(&main_display, "/dev/vbe", 0xBB8000);
  draw_wallpaper(&main_display);
  update_display(&main_display);

  main_display.border_size = 1;
  main_display.border_color = 0xF;
  main_display.active_window = NULL;
  main_display.window = NULL;

  num_fds = 100;
  fds = calloc(num_fds, sizeof(struct pollfd));
  for (size_t i = 0; i < num_fds; i++)
    fds[i].fd = -1;
  // Create socket
  int socket_fd = create_socket();
  assert(socket_fd >= 0);
  socket_fd_poll = 0;
  fds[socket_fd_poll].fd = socket_fd;
  fds[socket_fd_poll].events = POLLIN;
  fds[socket_fd_poll].revents = 0;
  int keyboard_fd = open("/dev/keyboard", O_RDONLY | O_NONBLOCK, 0);
  assert(keyboard_fd >= 0);
  keyboard_fd_poll = 1;
  fds[keyboard_fd_poll].fd = keyboard_fd;
  fds[keyboard_fd_poll].events = POLLIN;
  fds[keyboard_fd_poll].revents = 0;
  int mouse_fd = open("/dev/mouse", O_RDONLY, 0);
  assert(mouse_fd >= 0);
  mouse_fd_poll = 2;
  fds[mouse_fd_poll].fd = mouse_fd;
  fds[mouse_fd_poll].events = POLLIN;
  fds[mouse_fd_poll].revents = 0;
}

void reset_revents(struct pollfd *fds, size_t s) {
  for (size_t i = 0; i < s - 1; i++)
    fds[i].revents = 0;
}

void add_fd(int fd) {
  int i;
  for (i = 0; i < num_fds; i++)
    if (-1 == fds[i].fd)
      break;

  fds[i].fd = fd;
  fds[i].events = POLLIN | POLLHUP;
  //  fds[i].events = POLLIN;
  fds[i].revents = 0;
}

void add_window(int fd) {
  int window_socket = accept(fd, NULL, NULL);
  add_fd(window_socket);
  WINDOW *w = calloc(sizeof(WINDOW), 1);
  w->fd = window_socket;
  main_display.active_window = w;
  if (NULL == main_display.window) {
    assert(NULL == main_display.window_tail);
    main_display.window = w;
    main_display.window_tail = w;
  } else {
    w->next = main_display.window;
    assert(NULL == main_display.window->prev);
    main_display.window->prev = w;
    main_display.window = w;
  }
}

#define CLIENT_EVENT_CREATESCREEN 0
#define CLIENT_EVENT_UPDATESCREEN 1

void create_window(WINDOW *w, int fd, int x, int y, int sx, int sy) {
  w->bitmap_fd = fd;
  w->bitmap_ptr = mmap(NULL, sx * sy * sizeof(uint32_t), 0, 0, fd, 0);
  w->x = x;
  w->y = y;
  w->buffer_sx = w->sx = sx;
  w->buffer_sy = w->sy = sy;
}

typedef struct {
  uint16_t px;
  uint16_t py;
  uint16_t sx;
  uint16_t sy;
  uint8_t name_len;
} WS_EVENT_CREATE;

void parse_window_event(WINDOW *w) {
  uint8_t event_type;
  if (0 == read(w->fd, &event_type, sizeof(uint8_t))) {
    return;
  }
  if (0 == event_type) {
    draw();
    return;
  }
  if (1 == event_type) {
    WS_EVENT_CREATE e;
    for (; 0 == read(w->fd, &e, sizeof(e));)
      ;
    uint8_t bitmap_name[e.name_len + 1];
    read(w->fd, bitmap_name, e.name_len);
    bitmap_name[e.name_len] = '\0';
    int bitmap_fd = shm_open(bitmap_name, O_RDWR, O_CREAT);
    create_window(w, bitmap_fd, e.px, e.py, e.sx, e.sy);
    draw();
    return;
  }
  if (2 == event_type) {
    uint32_t vec[2];
    for (; 0 == read(w->fd, vec, sizeof(vec));)
      ;
    void *rc =
        mmap(NULL, vec[0] * vec[1] * sizeof(uint32_t), 0, 0, w->bitmap_fd, 0);
    if ((void *)-1 == rc) {
      return;
    }
    w->sx = vec[0];
    w->sy = vec[1];
    w->buffer_sx = vec[0];
    w->buffer_sy = vec[1];
    munmap(w->bitmap_ptr, 0);
    w->bitmap_ptr = rc;
    return;
  }
}

void send_key_event_to_window(struct KEY_EVENT ev) {
  WS_EVENT e = {
      .type = 0,
      .ev = ev,
  };
  write(main_display.active_window->fd, &e, sizeof(e));
}

void clamp_screen_position(int *x, int *y) {
  if (0 > *x) {
    *x = 0;
  }
  if (0 > *y) {
    *y = 0;
  }
  if (main_display.width < *x) {
    *x = main_display.width;
  }
  if (main_display.height < *y) {
    *y = main_display.height;
  }
}

void send_kill_window(WINDOW *w) {
  WS_EVENT e = {
      .type = WINDOWSERVER_EVENT_WINDOW_EXIT,
  };
  write(w->fd, &e, sizeof(e));
}

int windowserver_key_events(struct KEY_EVENT ev, int *redraw) {
  if (ev.release)
    return 0;
  if (!(ev.mode & (1 << 1)))
    return 0;
  if ('q' == ev.c) {
    send_kill_window(main_display.active_window);
    return 1;
  }
  if ('n' == ev.c) {
    // Create a new terminal
    int pid = fork();
    if (0 == pid) {
      // TODO: Close (almost) all file descriptors from parent
      char *argv[] = {"/term", NULL};
      execv("/term", argv);
      assert(0);
    }
    return 1;
  }
  if (!main_display.active_window)
    return 0;
  int x = 0;
  int y = 0;
  switch (ev.c) {
  case 'l':
    x++;
    break;
  case 'h':
    x--;
    break;
  case 'k':
    y--;
    break;
  case 'j':
    y++;
    break;
  }
  if (x || y) {
    main_display.active_window->x += x;
    main_display.active_window->y += y;
    clamp_screen_position(&main_display.active_window->x,
                          &main_display.active_window->y);
    *redraw = 1;
    return 1;
  }
  return 0;
}

struct mouse_event {
  uint8_t buttons;
  uint8_t x;
  uint8_t y;
};

void send_resize(WINDOW *w, int x, int y) {
  WS_EVENT e = {
      .type = WINDOWSERVER_EVENT_WINDOW_RESIZE,
      .vector = {x, y},
  };
  write(w->fd, &e, sizeof(e));
}

void parse_mouse_event(int fd) {
  int16_t xc = 0;
  int16_t yc = 0;
  int middle_button = 0;
  int right_button = 0;
  int left_button = 0;
  for (;;) {
    struct mouse_event e[100];
    int rc = read(fd, e, sizeof(e));
    if (rc <= 0)
      break;

    int n = rc / sizeof(e[0]);
    for (int i = 0; i < n; i++) {
      uint8_t xs = e[i].buttons & (1 << 4);
      uint8_t ys = e[i].buttons & (1 << 5);
      middle_button = e[i].buttons & (1 << 2);
      right_button = e[i].buttons & (1 << 1);
      left_button = e[i].buttons & (1 << 0);
      int16_t x = e[i].x;
      int16_t y = e[i].y;
      if (xs)
        x |= 0xFF00;
      if (ys)
        y |= 0xFF00;
      xc += x;
      yc += y;
    }
  }
  mouse_x += xc;
  mouse_y -= yc;
  if (mouse_x < 0) {
    mouse_x = 0;
  }
  if (mouse_y < 0) {
    mouse_y = 0;
  }
  if (mouse_left_down && !left_button) {
    mouse_left_up = 1;
  } else {
    mouse_left_up = 0;
  }

  if (mouse_right_down && !right_button) {
    mouse_right_up = 1;
  } else {
    mouse_right_up = 0;
  }
  mouse_right_down = right_button;

  mouse_left_down = left_button;
  if (middle_button) {
    if (main_display.active_window) {
      main_display.active_window->x += xc;
      main_display.active_window->y -= yc;
      clamp_screen_position(&main_display.active_window->x,
                            &main_display.active_window->y);
    }
  }
  if (mouse_right_down) {
    if (main_display.active_window) {
      int new_sx = main_display.active_window->sx + xc;
      int new_sy = main_display.active_window->sy - yc;
      new_sx = max(0, new_sx);
      new_sy = max(0, new_sy);
      send_resize(main_display.active_window, new_sx, new_sy);
    }
  }
  draw();
}

void parse_keyboard_event(int fd) {
  struct KEY_EVENT ev[250];
  int redraw = 0;
  for (;;) {
    int rc;
    if (0 > (rc = read(fd, &ev[0], sizeof(ev))))
      break;
    int n = rc / sizeof(ev[0]);
    for (int i = 0; i < n; i++) {
      if (windowserver_key_events(ev[i], &redraw)) {
        continue;
      }
      if (!main_display.active_window) {
        continue;
      }
      send_key_event_to_window(ev[i]);
    }
  }
  if (redraw) {
    draw();
  }
}

WINDOW *get_window(int fd) {
  WINDOW *w = main_display.window;
  for (; w; w = w->next) {
    if (fd == w->fd) {
      return w;
    }
  }
  return NULL;
}

void kill_window(WINDOW *w) {
  WINDOW *prev = w->prev;
  if (prev) {
    assert(prev->next == w);
    WINDOW *next_window = w->next;
    prev->next = next_window;
    if (next_window) {
      next_window->prev = prev;
    }
  } else {
    assert(w = main_display.window);
    main_display.window = w->next;
    if (main_display.window) {
      main_display.window->prev = NULL;
    }
    main_display.active_window = NULL;
  }

  if (w == main_display.window_tail) {
    assert(NULL == w->next);
    main_display.window_tail = prev;
  }
  free(w);
}

void parse_revents(struct pollfd *fds, size_t s) {
  for (size_t i = 0; i < s - 1; i++) {
    if (!fds[i].revents)
      continue;
    if (-1 == fds[i].fd)
      continue;
    if (socket_fd_poll == i && fds[i].revents & POLLIN) {
      add_window(fds[i].fd);
      continue;
    } else if (keyboard_fd_poll == i) {
      parse_keyboard_event(fds[i].fd);
      continue;
    } else if (mouse_fd_poll == i) {
      parse_mouse_event(fds[i].fd);
      continue;
    }
    WINDOW *w = get_window(fds[i].fd);
    assert(w);
    if (fds[i].revents & POLLHUP) {
      kill_window(w);
      fds[i].fd = -1;
    } else {
      parse_window_event(w);
    }
  }
}

void run(void) {
  for (;;) {
    poll(fds, num_fds, 0);
    parse_revents(fds, num_fds);
    reset_revents(fds, num_fds);
  }
}

int contains(int x, int y, int sx, int sy, int px, int py) {
  if ((px >= x) && (px <= x + sx) && (py >= y) && (py <= y + sy)) {
    return 1;
  }
  return 0;
}

int draw_button(DISPLAY *disp, int x, int y, int sx, int sy, uint32_t border,
                uint32_t filled, int id) {
  draw_rectangle(disp, x + 1, y + 1, sx, sy, filled);
  draw_outline(disp, x, y, sx, sy, 1, border);

  RET_CLICKED(x, y, sx, sy, id)
}

int draw_window(DISPLAY *disp, WINDOW *w, int id) {
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
  int border_px = 1;
  if (draw_button(disp, x + sx - 40, y - 20, 20, 20, 0x000000, 0x999999,
                  next_ui_id())) {
    w->minimized = 1;
  }
  if (draw_button(disp, x + sx - 20, y - 20, 20, 20, 0x000000, 0x999999,
                  next_ui_id())) {
    send_kill_window(w);
  }

  uint32_t border_color = 0x999999;
  if (w == disp->active_window) {
    border_color = 0xFF00FF;
  }
  draw_outline(disp, x, y, sx, sy, border_px, border_color);
  x += border_px;
  y += border_px;

  for (int i = 0; i < sy; i++) {
    if ((i + y) * disp->width + x > disp->height * disp->width)
      break;
    uint32_t *ptr =
        disp->back_buffer + disp->bpp * ((i + y) * disp->width) + x * disp->bpp;
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

  RET_CLICKED(x, y, sx, sy, id)
}

void update_display(const DISPLAY *disp) {
  for (int i = 0; i < 20; i++) {
    uint32_t color = 0xFFFFFFFF;
    // Make every other pixel black to make the mouse more visible on all
    // backgrounds.
    if (i & 1) {
      color = 0x0;
    }
    place_pixel(color, mouse_x + i, mouse_y + i);
    if (i <= 10) {
      place_pixel(color, mouse_x, mouse_y + i);
      place_pixel(color, mouse_x + i, mouse_y);
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

void draw(void) {
  ui_id = 0;
  DISPLAY *disp = &main_display;
  draw_wallpaper(disp);

  WINDOW *w = disp->window_tail;
  for (int i = 100; w; w = w->prev, i++) {
    if (!w->bitmap_ptr) {
      continue;
    }
    if (w->minimized) {
      continue;
    }
    if (draw_window(disp, w, i)) {
      if (w == main_display.active_window) {
        continue;
      }
      ui_state = 1;
      main_display.active_window = w;
      if (w == main_display.window) {
        continue;
      }
      // Remove w from the list
      WINDOW *prev = w->prev;
      assert(prev);
      assert(prev->next == w);
      WINDOW *next_window = w->next;
      prev->next = next_window;
      if (next_window) {
        next_window->prev = prev;
      }

      if (w == main_display.window_tail) {
        assert(NULL == w->next);
        main_display.window_tail = prev;
        assert(NULL == prev->next);
      }
      // Place it in front
      WINDOW *m = main_display.window;
      assert(NULL == m->prev);
      m->prev = w;
      w->next = m;
      w->prev = NULL;
      main_display.window = w;
      assert(main_display.window ==
             ((WINDOW *)main_display.window->next)->prev);
      if (NULL == m->next) {
        assert(m == main_display.window_tail);
      }
    }
  }

  int disp_x = 0;
  w = disp->window_tail;
  for (; w; w = w->prev) {
    if (w->minimized) {
      if (draw_button(disp, disp_x, 0, 20, 20, 0x000000, 0x999999,
                      next_ui_id())) {
        ui_state = 1;
        w->minimized = 0;
      }
      disp_x += 20;
    }
  }
  if (ui_state) {
    mouse_left_down = 0;
    mouse_left_up = 0;
    ui_state = 0;
    draw();
  } else {
    update_display(disp);
  }
}

int main(void) {
  open("/dev/serial", O_WRITE, 0);
  open("/dev/serial", O_WRITE, 0);
  // Start a terminal by default. This is just to make it easier for me
  // to test the system.
  int pid = fork();
  if (0 == pid) {
    // TODO: Close (almost) all file descriptors from parent
    char *argv[] = {"/term", NULL};
    execv("/term", argv);
    assert(0);
  }
  setup();
  run();
  return 0;
}
