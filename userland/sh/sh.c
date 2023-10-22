#include <assert.h>
#include <fcntl.h>
#include <limits.h>
#include <poll.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#define MAX_ARGS 20

#define IS_SPECIAL(_c) (';' == _c || '\n' == _c || '&' == _c || '|' == _c)

char *PATH = "/:/bin";

int can_open(char *f) {
  int rc;
  if (-1 == (rc = open(f, O_RDONLY, 0)))
    return 0;
  close(rc);
  return 1;
}

int find_path(char *file, char *buf) {
  if ('/' == *file || '.' == *file) { // Real path
    if (!can_open(file)) {
      return 0;
    }
    strcpy(buf, file);
    return 1;
  }

  char *p = PATH;
  for (;;) {
    char *b = p;
    for (; *p && ':' != *p; p++)
      ;
    size_t l = p - b;
    strlcpy(buf, b, l);
    strcat(buf, "/");
    strcat(buf, file);
    if (can_open(buf)) {
      return 1;
    }
    if (!*p)
      break;
    p++;
  }
  return 0;
}

int internal_exec(char *file, char **argv) {
  char r_path[PATH_MAX];
  if (!find_path(file, r_path))
    return 0;
  if (-1 == execv(r_path, argv)) {
    perror("exec");
    return 0;
  }
  return 1;
}

int execute_program(char *s, size_t l, int ignore_rc, int f_stdout,
                    int *new_out) {
  if (!l)
    return -1;

  char c[l + 1];
  memcpy(c, s, l);
  c[l] = 0;

  char *argv[MAX_ARGS];
  int args = 0;
  char *p = c;
  for (size_t i = 0; i <= l; i++) {
    if (' ' == c[i]) {
      if (p == c + i) {
        for (; ' ' == *p; p++, i++)
          ;
        i--;
        continue;
      }
      c[i] = '\0';

      if (strlen(p) == 0)
        break;

      argv[args] = p;
      args++;
      p = c + i + 1;
    } else if (i == l) {
      if (strlen(p) == 0)
        break;
      argv[args] = p;
      args++;
    }
  }
  argv[args] = NULL;

  int fd[2];
  pipe(fd);

  int pid = fork();
  if (0 == pid) {
    if (new_out)
      dup2(fd[1], 1);
    if (-1 != f_stdout)
      dup2(f_stdout, 0);
    close(fd[0]);
    if (!internal_exec(argv[0], argv)) {
      printf("exec failed\n");
      exit(1);
    }
  }

  close(fd[1]);
  if (new_out)
    *new_out = fd[0];

  if (ignore_rc)
    return 0;

  int rc;
  wait(&rc);
  return rc;
}

int compute_expression(char *exp) {
  char *n = exp;
  int ignore_next = 0;
  int rc;
  int f_stdout = -1;
  int new_out = f_stdout;
  for (; *exp; exp++) {
    if (!IS_SPECIAL(*exp)) {
      continue;
    }
    if ('\n' == *exp)
      break;
    if (';' == *exp) {
      if (!ignore_next) {
        execute_program(n, exp - n, 0, f_stdout, NULL);
      }
      n = exp + 1;
      continue;
    }
    if ('&' == *exp && '&' == *(exp + 1)) {
      if (!ignore_next) {
        rc = execute_program(n, exp - n, 0, f_stdout, NULL);
        if (0 != rc)
          ignore_next = 1;
      }
      n = exp + 2;
      exp++;
      continue;
    } else if ('&' == *exp) {
      if (!ignore_next) {
        execute_program(n, exp - n, 1, f_stdout, NULL);
      }
      n = exp + 1;
      continue;
    }
    if ('|' == *exp && '|' == *(exp + 1)) {
      if (!ignore_next) {
        rc = execute_program(n, exp - n, 0, f_stdout, NULL);
        if (0 == rc)
          ignore_next = 1;
      }
      n = exp + 2;
      exp++;
      continue;
    } else if ('|' == *exp) {
      if (!ignore_next) {
        execute_program(n, exp - n, 1, f_stdout, &new_out);
        f_stdout = new_out;
      }
      n = exp + 1;
      continue;
    }
  }
  if (!ignore_next)
    rc = execute_program(n, exp - n, 0, f_stdout, NULL);
  return rc;
}

char *get_line(void) {
  char *str = malloc(1024);
  char *p = str;
  int rc;
  for (;;) {
    if (0 == (rc = read(0, p, 1))) {
      continue;
    }
    if (0 > rc) {
      perror("read");
      continue;
    }
    if (8 == *p) {
      if (p == str)
        continue;
      putchar(*p);
      p--;
      continue;
    }
    putchar(*p);
    if ('\n' == *p) {
      break;
    }
    p++;
  }
  p++;
  *p = '\0';
  return str;
}

int main(void) {
  for (;;) {
    printf("/ : ");
    char *l = get_line();
    compute_expression(l);
    free(l);
  }
  return 0;
}
