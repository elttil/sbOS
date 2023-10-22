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

#define TERM_BACKGROUND 0x000000

int cmdfd;
GUI_Window *global_w;
uint32_t screen_pos_x = 0;
uint32_t screen_pos_y = 0;
int serial_fd;

void execsh(void) {
  char *argv[] = {NULL};
  execv("/sh", argv);
}

uint32_t cursor_pos_x = 0;
uint32_t cursor_pos_y = 0;

void screen_remove_old_cursor() {
  GUI_OverwriteFont(global_w, cursor_pos_x, cursor_pos_y, TERM_BACKGROUND);
}

void screen_update_cursor() {
  GUI_OverwriteFont(global_w, screen_pos_x, screen_pos_y, 0xFFFFFF);
  cursor_pos_x = screen_pos_x;
  cursor_pos_y = screen_pos_y;
}

void screen_putchar(uint32_t c) {
  if (c == '\n') {
    screen_pos_x = 0;
    screen_pos_y += 8;
    screen_remove_old_cursor();
    screen_update_cursor();
    return;
  }
  if (screen_pos_y > global_w->sy - 8) {
    uint8_t line[global_w->sx * sizeof(uint32_t)];
    for (int i = 0; i < global_w->sy - 8; i++) {
      memcpy(line, global_w->bitmap_ptr + ((i + 8) * global_w->sx),
             global_w->sx * sizeof(uint32_t));
      memcpy(global_w->bitmap_ptr + (i * global_w->sx), line,
             global_w->sx * sizeof(uint32_t));
    }
    for (int i = 0; i < global_w->sx; i += 8) {
      GUI_OverwriteFont(global_w, i, screen_pos_y - 8, TERM_BACKGROUND);
    }
    screen_pos_x = 0;
    screen_pos_y -= 8;
  }
  GUI_DrawFont(global_w, screen_pos_x, screen_pos_y, c);
  screen_pos_x += 8;
  if (screen_pos_x >= global_w->sx - 8) {
    screen_pos_x = 0;
    screen_pos_y += 8;
  }
  screen_update_cursor();
}

void screen_delete_char(void) {
  GUI_OverwriteFont(global_w, screen_pos_x, screen_pos_y, TERM_BACKGROUND);
  screen_pos_x -= 8;
  GUI_OverwriteFont(global_w, screen_pos_x, screen_pos_y, TERM_BACKGROUND);
  screen_update_cursor();
}

void screen_print(const unsigned char *s, int l) {
  for (; l; l--, s++) {
    if ('\b' == *s) {
      screen_delete_char();
      continue;
    }
    screen_putchar((uint32_t)*s);
  }
}

void newtty(void) {
  int m;
  int s;
  openpty(&m, &s, NULL, NULL, NULL);
  int pid = fork();
  if (0 == pid) {
    dup2(s, 0);
    dup2(s, 1);
    dup2(s, 2);
    execsh();
    return;
  }
  close(s);
  cmdfd = m;
}

void writetty(const char *b, size_t len) {
  write(serial_fd, b, len);
  for (int rc; len; len -= rc) {
    rc = write(cmdfd, b, len);
    if (-1 == rc) {
      perror("write");
      assert(0);
    }
  }
}

void run() {
  char buffer[4096];
  struct pollfd fds[2];
  fds[0].fd = cmdfd;
  fds[0].events = POLLIN;
  fds[0].revents = 0;
  fds[1].fd = global_w->ws_socket;
  fds[1].events = POLLIN;
  fds[1].revents = 0;
  for (;; fds[0].revents = fds[1].revents = 0) {
    poll(fds, 2, 0);
    if (fds[0].revents & POLLIN) {
      int rc;
      if ((rc = read(cmdfd, buffer, 4096))) {
        screen_print(buffer, rc);
        GUI_UpdateWindow(global_w);
      }
    }
    if (fds[1].revents & POLLIN) {
      WS_EVENT e;
      int rc;
      if (0 >= (rc = read(global_w->ws_socket, &e, sizeof(e))))
        continue;
            if (1 == e.ev.release)
              continue;
      if (0 == e.ev.c)
        continue;
      write(cmdfd, &e.ev.c, 1);
    }
  }
}

int main(void) {
  open("/dev/serial", O_RDWR, 0);
  open("/dev/serial", O_RDWR, 0);
  printf("running the terminal\n");
  int pid = fork();
  if (0 == pid) {
    char *argv[] = {NULL};
    execv("/ws", argv);
  }

  global_w = GUI_CreateWindow(20, 20, 250 * 4, 150 * 4);
  assert(global_w);
  for (int i = 0; i < 250 * 150 * 4 * 4; i++)
    global_w->bitmap_ptr[i] = TERM_BACKGROUND;
  //  memset(global_w->bitmap_ptr, 0x0, 50 * 50);
  GUI_UpdateWindow(global_w);
  newtty();
  run();
  return 0;
}
