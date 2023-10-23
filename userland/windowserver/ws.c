#include <fcntl.h>
#include <poll.h>
#include <stddef.h>
#include <stdint.h>
//#include <sys/mman.h>
#include "draw.h"
#include "font.h"
#include "ws.h"
#include <assert.h>
#include <socket.h>
#include <stdlib.h>
#include <string.h>
#include <sys/mman.h>
#include <unistd.h>

#define WINDOW_SERVER_SOCKET "/windowserver"

CLIENT *clients[100];
CLIENT *active_client;

int mouse_x = 0;
int mouse_y = 0;

// void *true_display;
//  void *buffer_display;
//  uint64_t display_size;
//  int border_size;
//  uint8_t border_color;
DISPLAY main_display;

struct pollfd *fds;
uint64_t num_fds;

uint64_t socket_fd_poll;
uint64_t keyboard_fd_poll;
uint64_t mouse_fd_poll;

// Taken from drivers/keyboard.c
struct KEY_EVENT {
  char c;
  uint8_t mode;    // (shift (0 bit)) (alt (1 bit))
  uint8_t release; // 0 pressed, 1 released
};

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
  disp->clients = clients;

  disp->wallpaper_fd = shm_open("wallpaper", O_RDWR, 0);
  assert(disp->wallpaper_fd >= 0);
  ftruncate(disp->wallpaper_fd, size);
  void *rc = mmap(NULL, size, 0, 0, disp->wallpaper_fd, 0);
  assert(rc);
  disp->wallpaper_buffer = rc;
  for (int i = 0; i < disp->size / BPP; i++) {
    uint32_t *p = disp->wallpaper_buffer + i * sizeof(uint32_t);
    *p = 0xFFFFFF;
  }
}

void setup(void) {
  setup_display(&main_display, "/dev/vbe", 0xBB8000);
  draw_wallpaper(&main_display);
  update_display(&main_display);

  main_display.border_size = 1;
  main_display.border_color = 0xF;
  active_client = NULL;
  for (int i = 0; i < 100; i++) {
    clients[i] = NULL;
  }

  num_fds = 100;
  fds = malloc(sizeof(struct pollfd[num_fds]));
  memset(fds, 0, sizeof(struct pollfd[num_fds]));
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

void add_client(int fd) {
  int client_socket = accept(fd, NULL, NULL);
  add_fd(client_socket);
  int i;
  for (i = 0; i < 100; i++)
    if (!clients[i])
      break;
  printf("adding client: %d\n", i);
  CLIENT *c = clients[i] = malloc(sizeof(CLIENT));
  c->fd = client_socket;
  active_client = c;
  printf("clients[0]: %x\n", clients[0]);
}

#define CLIENT_EVENT_CREATESCREEN 0
#define CLIENT_EVENT_UPDATESCREEN 1

void add_window(CLIENT *c, int fd, int x, int y, int sx, int sy) {
  WINDOW *w = malloc(sizeof(WINDOW));
  w->bitmap_fd = fd;
  w->bitmap_ptr = mmap(NULL, sx * sy * sizeof(uint32_t), 0, 0, fd, 0);
  w->x = x;
  w->y = y;
  w->sx = sx;
  w->sy = sy;
  c->w = w;
}

typedef struct {
  uint16_t px;
  uint16_t py;
  uint16_t sx;
  uint16_t sy;
  uint8_t name_len;
} WS_EVENT_CREATE;

void parse_client_event(CLIENT *c) {
  uint8_t event_type;
  if (0 == read(c->fd, &event_type, sizeof(uint8_t))) {
    printf("empty\n");
    return;
  }
  if (0 == event_type) {
    update_full_display(&main_display, mouse_x, mouse_y);
    return;
  }
  if (1 == event_type) {
    WS_EVENT_CREATE e;
    for (; 0 == read(c->fd, &e, sizeof(e));)
      ;
    uint8_t bitmap_name[e.name_len + 1];
    read(c->fd, bitmap_name, e.name_len);
    bitmap_name[e.name_len] = '\0';
    int bitmap_fd = shm_open(bitmap_name, O_RDWR, O_CREAT);
    add_window(c, bitmap_fd, e.px, e.py, e.sx, e.sy);
    update_full_display(&main_display, mouse_x, mouse_y);
  }
}

typedef struct {
  int type;
  struct KEY_EVENT ev;
} WS_EVENT;

void send_to_client(struct KEY_EVENT ev) {
  WS_EVENT e = {
      .type = 0,
      .ev = ev,
  };
  write(active_client->fd, &e, sizeof(e));
}

void clamp_screen_position(int *x, int *y) {
  if (0 > *x) {
    *x = 0;
  }
  if (0 > *y) {
    *y = 0;
  }
  if (WIDTH < *x) {
    *x = WIDTH;
  }
  if (HEIGHT < *y) {
    *y = HEIGHT;
  }
}

int windowserver_key_events(struct KEY_EVENT ev, int *redraw) {
  if (ev.release)
    return 0;
  if (!(ev.mode & (1 << 1)))
    return 0;
  if ('n' == ev.c) {
    // Create a new terminal
    int pid = fork();
    if (0 == pid) {
      // TODO: Close (almost) all file descriptors from parent
      char *argv[] = {"/term", NULL};
      execv("/term", argv);
      assert(0);
    }
  }
  if (!active_client)
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
    active_client->w->x += x;
    active_client->w->y += y;
    clamp_screen_position(&active_client->w->x, &active_client->w->y);
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

void update_mouse(void) {
  draw_mouse(&main_display, mouse_x, mouse_y);
  return;
}

void focus_window(int x, int y) {
  for (int i = 0; i < 100; i++) {
    if (!clients[i])
      continue;
    WINDOW *w = clients[i]->w;
    if (w->x < x && x < w->x + w->sx) {
      if (w->y < y && y < w->y + w->sy) {
        active_client = clients[i];
      }
    }
  }
}

void parse_mouse_event(int fd) {
  int16_t xc = 0;
  int16_t yc = 0;
  int middle_button = 0;
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
      left_button = e[i].buttons & (1 << 0);
      int16_t x = e[i].x;
      int16_t y = e[i].y;
      if (xs)
        x |= 0xFF00;
      if (ys)
        y |= 0xFF00;
      xc += *(int16_t *)&x;
      yc += *(int16_t *)&y;
    }
  }
  mouse_x += xc;
  mouse_y -= yc;
  if (mouse_x < 0)
    mouse_x = 0;
  if (mouse_y < 0)
    mouse_y = 0;
  if (left_button) {
    focus_window(mouse_x, mouse_y);
  }
  if (middle_button) {
    if (active_client) {
      active_client->w->x += xc;
      active_client->w->y -= yc;
      clamp_screen_position(&active_client->w->x, &active_client->w->y);
    }
  }
  update_mouse();
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
      if (windowserver_key_events(ev[i], &redraw))
        continue;
      if (!active_client)
        continue;
      send_to_client(ev[i]);
    }
  }
  if (redraw)
    update_full_display(&main_display, mouse_x, mouse_y);
}

CLIENT *get_client(int fd) {
  for (int i = 0; i < 100; i++) {
    if (!clients[i])
      continue;
    if (clients[i]->fd == fd)
      return clients[i];
  }
  return NULL;
}

void kill_client(CLIENT *c) {
  c->w = NULL;
  update_full_display(&main_display, mouse_x, mouse_y);
  active_client = clients[0];
}

void parse_revents(struct pollfd *fds, size_t s) {
  for (size_t i = 0; i < s - 1; i++) {
    if (!fds[i].revents)
      continue;
    if (-1 == fds[i].fd)
      continue;
    if (socket_fd_poll == i && fds[i].revents & POLLIN) {
      add_client(fds[i].fd);
      continue;
    } else if (keyboard_fd_poll == i) {
      parse_keyboard_event(fds[i].fd);
      continue;
    } else if (mouse_fd_poll == i) {
      parse_mouse_event(fds[i].fd);
      continue;
    }
    CLIENT *c = get_client(fds[i].fd);
    assert(c);
    if (fds[i].revents & POLLHUP) {
      kill_client(c);
      fds[i].fd = -1;
    } else {
      parse_client_event(c);
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
