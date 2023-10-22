// ed - edit text
// ed [-p string] [-s] [file]
#include <assert.h>
#include <stdio.h>
#include <string.h>
#define COMMAND_MODE 0
#define INPUT_MODE 1

int mode = COMMAND_MODE;

FILE *fp = NULL;
FILE *mem_fp = NULL;
int line_number = 1;

int getline(char *buffer, size_t s) {
  (void)s;
  int i = 0;
  char c;
  for (;; i++) {
    int rc = read(0, &c, 1);
    assert(rc > 0);
    printf("%c", c);
    buffer[i] = c;
    if ('\n' == c)
      break;
  }
  buffer[i] = '\0';
  return i;
}

void goto_line(void) {
  char c;
  fseek(mem_fp, 0, SEEK_SET);
  int line = 1;
  if (1 != line_number) {
    // Goto line
    for (; fread(&c, 1, 1, mem_fp);) {
      if ('\n' == c)
        line++;
      if (line == line_number)
        return;
    }
    printf("got to line: %d\n", line);
    assert(0);
  }
}

void read_line(void) {
  char c;
  goto_line();
  for (; fread(&c, 1, 1, mem_fp);) {
    if ('\n' == c)
      break;
    printf("%c", c);
  }
  printf("\n");
}

void goto_end_of_line(void) {
  char c;
  for (; fread(&c, 1, 1, mem_fp);) {
    if ('\n' == c)
      break;
  }
}

void delete_line(void) {
  long s = ftell(mem_fp);
  printf("s: %d\n", s);
  goto_end_of_line();
  long end = ftell(mem_fp);
  printf("end: %d\n", end);
  long offset = end - s;
  for (char buffer[4096];;) {
    int rc = fread(buffer, 1, 100, mem_fp);
    long reset = ftell(mem_fp);
    if (0 == rc)
      break;
    fseek(mem_fp, s, SEEK_SET);
    fwrite(buffer, 1, rc, mem_fp);
    s += rc;
    fseek(mem_fp, reset, SEEK_SET);
  }
}

void read_command(void) {
  char buffer[4096];
  char *s = buffer;
  getline(buffer, 4096);
  int a = -1;
  int rc = sscanf(buffer, "%d", &a);
  if (0 < rc) {
    line_number = a;
  }
  for (; isdigit(*s); s++)
    ;
  if (0 == strcmp(s, "i")) {
    mode = INPUT_MODE;
    return;
  }
  if (0 == strcmp(s, ".")) {
    read_line();
    return;
  }
  return;
}

void read_input(void) {
  char buffer[4096];
  int l = getline(buffer, 4096);
  if (0 == strcmp(buffer, ".")) {
    mode = COMMAND_MODE;
    return;
  }
  goto_line();
  delete_line();
  goto_line();
  assert(fwrite(buffer, l, 1, mem_fp));
  return;
}

int ed_main(char argc, char **argv) {
  if (argc < 2)
    return 1;
  char *buffer;
  size_t size;
  mem_fp = open_memstream(&buffer, &size);
  assert(mem_fp);
  fp = fopen(argv[1], "r");
  assert(fp);
  char r_buffer[4096];
  for (int rc; (rc = fread(buffer, 1, 4096, fp));) {
    fwrite(r_buffer, 1, rc, mem_fp);
  }

  for (;;) {
    if (COMMAND_MODE == mode)
      read_command();
    else
      read_input();
  }
  fclose(fp);
  fclose(mem_fp);
  return 0;
}
