#include <assert.h>
#include <fcntl.h>
#include <libgui.h>
#include <poll.h>
#include <pty.h>
#include <socket.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>

#define BACKGROUND_COLOR 0x000000

int file_fd;
char *file_buffer;
uint64_t file_buffer_size;
uint64_t file_size = 0;
uint64_t file_real_position = 0;
GUI_Window *global_w;

typedef enum {
  NORMAL,
  INSERT,
  COMMAND,
} editor_mode_t;

editor_mode_t current_mode = NORMAL;
uint32_t cursor_pos_x = 0;
uint64_t cursor_pos_y = 0;
void draw_file(void);
void write_file(void);

int clamp(int *x, int *y) {
  int clamped = 0;
  if (*x < 0) {
    *x = 0;
    clamped = 1;
  }
  if (*y < 0) {
    *y = 0;
    clamped = 1;
  }
  if (*x + 8 > global_w->sx) {
    *x = (global_w->sx - 8) / 8;
    clamped = 1;
  }
  if (*y + 8 > global_w->sy) {
    *y = (global_w->sy - 8) / 8;
    clamped = 1;
  }
  return clamped;
}

void insert_char(int pos, char c) {
  if (0 == file_size) {
    file_size++;
    file_buffer[0] = c;
    return;
  }

  // Shift the buffer at 'pos' to the right by one.
  memmove(file_buffer + pos + 1, file_buffer + pos, file_size - pos);

  file_buffer[pos] = c;
  file_size++;
}

void delete_char(int pos, int *deleted_newline) {
  if (0 == pos) {
    return;
  }
  // Delete the characther at 'pos'. This makes sense if 'pos' is
  // at the end of the file and as a result the shift operation done
  // later in the function is not overwritting the characther.
  if ('\n' == file_buffer[pos - 1])
    *deleted_newline = 1;
  file_buffer[pos - 1] = 0;

  // Shift the buffer at 'pos' to the left by one
  memmove(file_buffer + pos - 1, file_buffer + pos, file_size - pos);
  file_size--;
}

uint64_t cursor_to_real(int *px, int y, int *is_valid) {
  int x = *px;
  *is_valid = 0;
  if (clamp(&x, &y))
    return file_real_position;
  if (0 == file_size) {
    if (0 == x && 0 == y) {
      *is_valid = 1;
      return 0;
    }
    return 0;
  }
  uint64_t p = 0;
  int cx = 0;
  int cy = 0;
  for (; p < file_size; p++) {
    if (cy == y) {
      *is_valid = 1;
      *px = cx;
      if (cx == x) {
        break;
      }
    }
    if ('\n' == file_buffer[p]) {
      if (cy == y) {
        break;
      }
      cx = 0;
      cy++;
      continue;
    }
    cx++;
  }
  if (*is_valid)
    return p;
  else
    return file_real_position;
}

void key_event(char c) {
  int x = 0;
  int y = 0;
  if (COMMAND == current_mode) {
    if ('w' == c) {
      printf("wrote to file\n");
      write_file();
      current_mode = NORMAL;
      return;
    }
    if ('q' == c) {
      close(file_fd);
      close(global_w->ws_socket);
      current_mode = NORMAL;
      exit(0);
      return;
    }
    return;
  } else if (NORMAL == current_mode) {
    switch (c) {
    case 'j':
      y++;
      break;
    case 'k':
      y--;
      break;
    case 'l':
      x++;
      break;
    case 'h':
      x--;
      break;
    case 'c':
      printf("entering command mode\n");
      current_mode = COMMAND;
      return;
      break;
    case 'i':
      current_mode = INSERT;
      return;
      break;
    default:
      return;
    }
  } else {
    if ('\x1B' == c) {
      current_mode = NORMAL;
      return;
    }
    if ('\b' == c) {
      int deleted_newline = 0;
      delete_char(file_real_position, &deleted_newline);
      if (deleted_newline) {
        y--;
      } else {
        x--;
      }
      GUI_ClearScreen(global_w, BACKGROUND_COLOR);
    } else {
      insert_char(file_real_position, c);
      printf("inserting char: %c\n", c);
      if ('\n' == c) {
        cursor_pos_x = 0;
        y++;
      } else {
        x++;
      }
      GUI_ClearScreen(global_w, BACKGROUND_COLOR);
    }
  }

  int is_valid_position = 0;
  cursor_pos_x += x;
  file_real_position =
      cursor_to_real(&cursor_pos_x, cursor_pos_y + y, &is_valid_position);
  if (!is_valid_position) {
    return;
  }
  GUI_OverwriteFont(global_w, cursor_pos_x * 8, cursor_pos_y * 8,
                    BACKGROUND_COLOR);
  draw_file();
  cursor_pos_y += y;
  GUI_OverwriteFont(global_w, cursor_pos_x * 8, cursor_pos_y * 8, 0xFFFFFF);

  GUI_UpdateWindow(global_w);
}

void draw_file(void) {
  assert(file_buffer);
  uint32_t screen_pos_x = 0;
  uint64_t screen_pos_y = 0;
  for (uint64_t i = 0; i < file_size; i++) {
    char c = file_buffer[i];
    if ('\n' == c) {
      screen_pos_x = 0;
      screen_pos_y += 8;
      continue;
    }
    GUI_DrawFont(global_w, screen_pos_x, screen_pos_y, c);
    screen_pos_x += 8;
  }
}

void write_file(void) {
  assert(file_buffer);
  assert(0 == ftruncate(file_fd, file_size));
  pwrite(file_fd, file_buffer, file_size, 0);
}

int open_file(const char *file) {
  file_fd = open(file, O_RDWR, O_CREAT);
  printf("file_fd: %d\n", file_fd);
  if (0 > file_fd) {
    perror("open");
    return 0;
  }

  file_buffer_size = 0x1000;
  file_buffer = malloc(file_buffer_size);
  uint64_t index = 0;
  assert(file_buffer);
  for (;;) {
    char buffer[4096];
    int rc = read(file_fd, buffer, 4096);
    if (-1 == rc) {
      perror("read");
      assert(0);
    }

    if (0 == rc)
      break;

    file_size += rc;
    if (file_size > file_buffer_size) {
      file_buffer_size = file_size;
      file_buffer = realloc(file_buffer, file_buffer_size);
      assert(file_buffer);
    }

    for (int i = 0; i < rc; i++, index++) {
      file_buffer[index] = buffer[i];
    }
  }
  return 1;
}

void event_handler(WS_EVENT e) {
  if (WINDOWSERVER_EVENT_WINDOW_EXIT == e.type)
    exit(0);

  if (WINDOWSERVER_EVENT_KEYPRESS != e.type)
    return;

  if (e.ev.release)
    return;
  if (0 == e.ev.c)
    return;
  key_event(e.ev.c);
}

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("File has to be specified.\n");
    return 1;
  }
  global_w = GUI_CreateWindow(10, 10, 150 * 4, 150 * 4);
  assert(global_w);
  GUI_ClearScreen(global_w, BACKGROUND_COLOR);
  GUI_UpdateWindow(global_w);

  assert(open_file(argv[1]));
  draw_file();
  GUI_EventLoop(global_w, event_handler);
  return 0;
}
