#include "../../drivers/serial.h"
#include "../include/assert.h"
#include "../include/stdio.h"
#include <stdarg.h>

#define TAB_SIZE 8

const char HEX_SET[0x10] = {'0', '1', '2', '3', '4', '5', '6', '7',
                            '8', '9', 'A', 'B', 'C', 'D', 'E', 'F'};

inline void putc(const char c) {
  write_serial(c);
}

int kprint_hex(u64 num) {
  int c = 2;

  if (num == 0) {
    putc('0');
    c++;
    return c;
  }

  char str[16] = {0};
  int i = 0;
  for (; num != 0 && i < 16; i++, num /= 16) {
    str[i] = HEX_SET[(num % 16)];
  }

  c += i;
  for (i--; i >= 0; i--) {
    putc(str[i]);
  }

  return c;
}

int kprint_int(int num) {
  int c = 0;
  if (0 == num) {
    putc('0');
    c++;
    return c;
  }
  char str[10];
  int i = 0;
  for (; num != 0 && i < 10; i++, num /= 10) {
    str[i] = (num % 10) + '0';
  }

  c += i;
  for (i--; i >= 0; i--) {
    putc(str[i]);
  }
  return c;
}

int kprintf(const char *format, ...) {
  int c = 0;
  va_list list;
  va_start(list, format);

  const char *s = format;
  for (; *s; s++) {
    if ('%' != *s) {
      putc(*s);
      c++;
      continue;
    }

    char flag = *(s + 1);
    if ('\0' == flag) {
      break;
    }

    switch (flag) {
    case 'c':
      putc((char)va_arg(list, int));
      c++;
      break;
    case 'd':
      c += kprint_int(va_arg(list, int));
      break;
    case 's':
      for (char *string = va_arg(list, char *); *string; putc(*string++), c++)
        ;
      break;
    case 'x':
      c += kprint_hex(va_arg(list, const u32));
      break;
    case '%':
      putc('%');
      c++;
      break;
    default:
      ASSERT_NOT_REACHED;
      break;
    }
    s++;
  }
  return c;
}

int puts(char *str) {
  return kprintf("%s\n", str);
}
