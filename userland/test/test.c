#include <assert.h>
#include <fcntl.h>
//#include <json.h>
#include <ctype.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#if 1
void dbgln(const char *fmt) { printf("%s\n", fmt); }
#else
void dbgln(const char *fmt, ...) {
  printf("| ");
  va_list ap;
  va_start(ap, fmt);
  int rc = vdprintf(1, fmt, ap);
  va_end(ap);
  printf("\n");
}
#endif

void isx_test() {
  dbgln("isx TEST");
  {
    assert(isspace(' '));
    assert(isspace('\t'));
    assert(!isspace('A'));

    assert(isalpha('A'));
    assert(!isalpha('9'));
    assert(!isalpha(' '));
    assert(!isalpha('z' + 1));

    assert(isascii('A'));
    assert(isascii('9'));
    assert(isascii(' '));
    assert(!isascii(0xFF));

    assert(ispunct('.'));
    assert(!ispunct('A'));

    assert(isdigit('9'));
    assert(!isdigit('A'));
  }
  dbgln("isx TEST PASSED");
}

void malloc_test(void) {
  dbgln("malloc TEST");
  for (int j = 0; j < 100; j++) {
    //    printf("j : %x\n", j);
    uint8_t *t = malloc(400 + j);
    memset(t, 0x43 + (j % 10), 400 + j);
    uint8_t *p = malloc(900 + j);
    memset(p, 0x13 + (j % 10), 900 + j);
    assert(p > t && p > (t + 400 + j));

    for (int i = 0; i < 400 + j; i++)
      if (t[i] != 0x43 + (j % 10)) {
        printf("t addy: %x, val: %x, should be: %x\n", t + i, t[i],
               0x43 + (j % 10));
        assert(0);
      }

    for (int i = 0; i < 900 + j; i++)
      if (p[i] != 0x13 + (j % 10)) {
        printf("p addy: %x, val: %x, should be: %x\n", p + i, t[i],
               0x13 + (j % 10));
        assert(0);
      }
    free(t);
  }
  dbgln("malloc TEST PASSED");
}

void memcmp_test(void) {
  dbgln("memcmp TEST");
  {
    assert(0 == memcmp("\xAA\xBB\xCC", "\xAA\xBB\xCC", 3));
    assert(1 == memcmp("\xAF\xBB\xCC", "\xAA\xBB\xCC", 3));
  }
  dbgln("memcmp TEST PASSED");
}

void memcpy_test(void) {
  dbgln("memcpy TEST");
  {
    char buf[100];
    assert(buf == memcpy(buf, "\xAA\xBB\xCC", 3));
    assert(0 == memcmp(buf, "\xAA\xBB\xCC", 3));
    assert(buf == memcpy(buf, "\xCC\xBB\xCC", 3));
    assert(0 == memcmp(buf, "\xCC\xBB\xCC", 3));

    assert(buf == memcpy(buf, "\xAA\xBB\xCC\xDD", 4));
    assert(0 == memcmp(buf, "\xAA\xBB\xCC\xDD", 4));
    assert(buf == memcpy(buf, "\xAA\xBB\xCC\xEE", 3));
    assert(0 == memcmp(buf, "\xAA\xBB\xCC\xDD", 4));
  }
  dbgln("memcpy TEST PASSED");
}

void memmove_test(void) {
  dbgln("memmove TEST");
  {
    char buf[100];
    memcpy(buf, "foobar", 6);
    const char *p = buf + 2; // obar

    assert(buf == memmove(buf, p, 4));
    assert(0 == memcmp(buf, "obarar", 6));
  }
  dbgln("memmove TEST PASSED");
}

void strlen_test(void) {
  dbgln("strlen TEST");
  {
    assert(0 == strlen(""));
    assert(3 == strlen("%is"));
    assert(3 == strlen("foo"));
    assert(3 == strlen("bar"));
    assert(6 == strlen("foobar"));
  }
  dbgln("strlen TEST PASSED");
}

void strnlen_test(void) {
  dbgln("strnlen TEST");
  {
    assert(0 == strnlen("", 0));
    assert(0 == strnlen("", 3));
    assert(3 == strnlen("foo", 3));
    assert(2 == strnlen("bar", 2));
    assert(3 == strnlen("foobar", 3));
    assert(6 == strnlen("foobar", 6));
    assert(6 == strnlen("foobar", 8));
  }
  dbgln("strnlen TEST PASSED");
}

/*
void json_test(void) {
  dbgln("JSON TEST");
  {
    const char *s = "{\"string\": \"hello\", \"object\": {\"string\": \"ff\"}}";
    JSON_CTX ctx;
    JSON_Init(&ctx);
    JSON_Parse(&ctx, s);
    JSON_ENTRY *main_entry = JSON_at(&ctx.global_object, 0);
    assert(main_entry != NULL);
    JSON_ENTRY *index_entry = JSON_at(main_entry, 0);
    JSON_ENTRY *string_entry = JSON_search_name(main_entry, "string");
    assert(index_entry != NULL);
    assert(index_entry == string_entry);
    char *str = index_entry->value;
    assert(0 == strcmp(str, "hello"));

    JSON_ENTRY *object = JSON_search_name(main_entry, "object");
    assert(object != NULL);
    JSON_ENTRY *object_string = JSON_search_name(object, "string");
    assert(object_string != NULL);
    assert(0 == strcmp(object_string->value, "ff"));
  }
  dbgln("JSON TEST PASSED");
}*/

void random_test(void) {
  dbgln("/dev/random TEST");
  {
    int fd = open("/dev/random", O_RDONLY, 0);
    uint8_t buffer_1[256];
    assert(256 == read(fd, buffer_1, 256));
    close(fd);
    fd = open("/dev/random", O_RDONLY, 0);
    uint8_t buffer_2[256];
    assert(256 == read(fd, buffer_2, 256));
    close(fd);
    assert(0 != memcmp(buffer_1, buffer_2, 256));
  }
  dbgln("/dev/random TEST PASSED");
}

void sscanf_test(void) {
  dbgln("sscanf TEST");
  {
    int d1;
    int d2;
    sscanf("1337:7331", "%d:%d", &d1, &d2);
    assert(1337 == d1);
    assert(7331 == d2);
    sscanf(" 1234 31415", "%d%d", &d1, &d2);
    assert(1234 == d1);
    assert(31415 == d2);
    sscanf(" 1234 441122", "%*d%d", &d1);
    assert(441122 == d1);

    char f;
    char b;
    sscanf("f:b", "%c:%c", &f, &b);
    assert('f' == f);
    assert('b' == b);
  }
  dbgln("sscanf TEST PASSED");
}

void strtoul_test(void) {
  dbgln("strtoul TEST");
  {
    long r;
    char *s = "1234";
    char *endptr;
    r = strtoul("1234", &endptr, 10);
    assert(1234 == r);
    assert(endptr == (s + 4));
  }
  dbgln("strtoul TEST PASSED");
}

void strcspn_test(void) {
  dbgln("strcspn TEST");
  {
    assert(4 == strcspn("1234", ""));
    assert(3 == strcspn("1234", "4"));
    assert(2 == strcspn("1234", "43"));
    assert(0 == strcspn("1234", "2143"));
  }
  dbgln("strcspn TEST PASSED");
}

void strpbrk_test(void) {
  dbgln("strpbrk TEST");
  {
    char *s = "01234";
    assert(s + 3 == strpbrk(s, "3"));
    assert(s + 4 == strpbrk(s, "4"));
    assert(s + 2 == strpbrk(s, "2"));
    assert(s + 2 == strpbrk(s, "23"));
    assert(NULL == strpbrk(s, "5"));
  }
  dbgln("strpbrk TEST PASSED");
}

void strcpy_test(void) {
  dbgln("strcpy TEST");
  {
    char buf[10];
    memset(buf, '\xBB', 10);
    strcpy(buf, "hello");
    assert(0 == memcmp(buf, "hello\0\xBB\xBB\xBB\xBB", 10));
  }
  dbgln("strcpy TEST PASSED");
}

void strncpy_test(void) {
  dbgln("strncpy TEST");
  {
    char buf[10];
    memset(buf, '\xBB', 10);
    strncpy(buf, "hello", 3);
    assert(0 == memcmp(buf, "hel\xBB\xBB\xBB\xBB\xBB\xBB\xBB", 10));
    strncpy(buf, "hello", 6);
    assert(0 == memcmp(buf, "hello\0\xBB\xBB\xBB\xBB", 10));
  }
  dbgln("strncpy TEST PASSED");
}

void strlcpy_test(void) {
  dbgln("strlcpy TEST");
  {
    char buf[10];
    memset(buf, '\xBB', 10);
    strlcpy(buf, "hello", 3);
    assert(0 == memcmp(buf, "hel\x00\xBB\xBB\xBB\xBB\xBB\xBB", 10));
  }
  dbgln("strlcpy TEST PASSED");
}

void strcat_test(void) {
  dbgln("strcat TEST");
  {
    char buf[10];
    memset(buf, '\xBB', 10);
    strcpy(buf, "hel");
    assert(0 == memcmp(buf, "hel\0\xBB\xBB\xBB\xBB\xBB\xBB", 10));
    strcat(buf, "lo");
    assert(0 == memcmp(buf, "hello\0\xBB\xBB\xBB\xBB", 10));
  }
  dbgln("strcat TEST PASSED");
}

void strchr_test(void) {
  dbgln("strchr TEST");
  {
    char *s = "0123412345";
    assert(s + 3 == strchr(s, '3'));
    assert(s + 4 + 5 == strchr(s, '5'));
    assert(s + 4 == strchr(s, '4'));
    assert(NULL == strchr(s, '9'));
  }
  dbgln("strchr TEST PASSED");
}

void strrchr_test(void) {
  dbgln("strrchr TEST");
  {
    char *s = "0123412345";
    assert(s + 3 + 4 == strrchr(s, '3'));
    assert(s == strrchr(s, '0'));
    assert(s + 4 + 5 == strrchr(s, '5'));
    assert(NULL == strrchr(s, '9'));
  }
  dbgln("strrchr TEST PASSED");
}

void strdup_test(void) {
  dbgln("strdup TEST");
  {
    char *s;
    s = strdup("hello");
    assert(s);
    assert(0 == strcmp(s, "hello"));
    free(s);

    s = strdup("abc");
    assert(s);
    assert(0 == strcmp(s, "abc"));
    free(s);
  }
  dbgln("strdup TEST PASSED");
}

void strndup_test(void) {
  dbgln("strdup TEST");
  {
    char *s;
    s = strndup("hello", 5);
    assert(s);
    assert(0 == strcmp(s, "hello"));
    free(s);

    s = strndup("hello", 3);
    assert(s);
    assert(0 == strcmp(s, "hel"));
    free(s);
  }
  dbgln("strndup TEST PASSED");
}

void abs_test(void) {
  dbgln("abs TEST");
  {
    assert(1 == abs(1));
    assert(1 == abs(-1));
    assert(0 == abs(0));
  }
  dbgln("abs TEST PASSED");
}

void strspn_test(void) {
  dbgln("strspn TEST");
  {
    assert(0 == strspn("abc", ""));
    assert(0 == strspn("abc", "xyz"));
    assert(1 == strspn("abc", "a"));
    assert(2 == strspn("abc", "ab"));
    assert(3 == strspn("abcd", "abc"));
    assert(3 == strspn("abcde", "abce"));
  }
  dbgln("strspn TEST PASSED");
}

void strtol_test(void) {
  dbgln("strtol TEST");
  {
    char *s = "1234";
    char *e;
    long r;
    assert(1234 == strtol(s, &e, 10));
    assert(e == (s + 4));
  }
  dbgln("strtol TEST PASSED");
}

void strcmp_test(void) {
#define EQ(_s1)                                                                \
  { assert(0 == strcmp(_s1, _s1)); }
  dbgln("strcmp TEST");
  {
    EQ("hello, world");
    EQ("");
    EQ("int: 100, hex: 64, octal: 144 a");
  }
  dbgln("strcmp TEST PASSED");
}

void strncmp_test(void) {
  dbgln("strncmp TEST");
  {
    assert(0 == strncmp("hello", "hello", 5));
    assert(0 < strncmp("foo", "bar", 3));
    assert(0 == strncmp("foobar", "foofoo", 3));
  }
  dbgln("strncmp TEST PASSED");
}

void strcasecmp_test(void) {
  dbgln("strcasecmp TEST");
  {
    assert(0 == strcasecmp("foobar", "FOObar"));
    assert(0 == strcasecmp("foobar", "foobar"));
    assert(6 == strcasecmp("foobar", "bar"));
  }
  dbgln("strcasecmp TEST PASSED");
}

void strncasecmp_test(void) {
  dbgln("strncasecmp TEST");
  {
    assert(0 == strncasecmp("foobar", "FOObar", 6));
    assert(0 == strncasecmp("foobar", "foobar", 6));
    assert(0 == strncasecmp("foo", "foobar", 3));
    assert(-1 == strncasecmp("foo", "foobar", 4));
  }
  dbgln("strncasecmp TEST PASSED");
}

void strstr_test(void) {
  dbgln("strstr TEST");
  {
    const char *a = "foobar123";
    const char *b = "bar";
    assert(a + 3 == strstr(a, b));
  }
  dbgln("strstr TEST PASSED");
}

void strtok_test(void) {
  dbgln("strtok TEST");
  {
    char *s = "hello,world,goodbye";
    assert(0 == strcmp(strtok(s, ","), "hello"));
    assert(0 == strcmp(strtok(NULL, ","), "world"));
    assert(0 == strcmp(strtok(NULL, ","), "goodbye"));
    assert(NULL == strtok(NULL, ","));
    char *s2 = "hello,,world,,goodbye,test";
    assert(0 == strcmp(strtok(s2, ",,"), "hello"));
    assert(0 == strcmp(strtok(NULL, ",,"), "world"));
    assert(0 == strcmp(strtok(NULL, ","), "goodbye"));
    assert(0 == strcmp(strtok(NULL, ","), "test"));
    assert(NULL == strtok(NULL, ","));
  }
  dbgln("strtok TEST PASSED");
}

void fseek_test(void) {
  dbgln("fseek TEST");
  {
#ifndef LINUX
    FILE *fp = fopen("/testfile", "r");
#else
    FILE *fp = fopen("testfile", "r");
#endif
    assert(NULL != fp);
    char c;
    assert(0 == ftell(fp));
    assert(1 == fread(&c, 1, 1, fp));
    assert('A' == c);

    assert(1 == ftell(fp));
    assert(1 == fread(&c, 1, 1, fp));
    assert('B' == c);

    assert(2 == ftell(fp));
    fseek(fp, 0, SEEK_SET);
    assert(0 == ftell(fp));
    assert(1 == fread(&c, 1, 1, fp));
    assert('A' == c);
    fseek(fp, 2, SEEK_SET);
    assert(2 == ftell(fp));

    assert(1 == fread(&c, 1, 1, fp));
    assert('C' == c);
    fseek(fp, 2, SEEK_CUR);
    assert(5 == ftell(fp));
    assert(1 == fread(&c, 1, 1, fp));
    assert('F' == c);

    char buffer[100];
    assert(6 == ftell(fp));
    fseek(fp, 0, SEEK_SET);
    assert(1 == fread(buffer, 10, 1, fp));
    assert(0 == memcmp(buffer, "ABCDEFGHIJK", 10));
    assert(10 == ftell(fp));
    fseek(fp, 1, SEEK_SET);
    assert(10 == fread(buffer, 1, 10, fp));
    assert(0 == memcmp(buffer, "BCDEFGHIJKL", 10));

    fseek(fp, 0, SEEK_END);
    assert(0 == fread(&c, 1, 1, fp));
    fseek(fp, -2, SEEK_CUR);
    assert(1 == fread(&c, 1, 1, fp));
    assert('z' == c);
    fseek(fp, -3, SEEK_END);
    assert(1 == fread(&c, 1, 1, fp));
    assert('y' == c);

    assert(0 == fclose(fp));
  }
  dbgln("fseek TEST PASSED");
}

void printf_test(void) {
#define EXP(exp)                                                               \
  {                                                                            \
    int exp_n = strlen(exp);                                                   \
    if (0 != strcmp(buf, exp) || buf_n != exp_n) {                             \
      printf("buf n : %d\n", buf_n);                                           \
      printf("exp n : %d\n", snprintf(NULL, 0, "%s", exp));                    \
      printf("Expected: %s", exp);                                             \
      printf("\n");                                                            \
      printf("Got:      %s", buf);                                             \
      printf("\n");                                                            \
      assert(0);                                                               \
    }                                                                          \
  }
  dbgln("printf TEST");
  {
    char buf[200];
    int buf_n;
    buf_n = sprintf(buf, "hello");
    EXP("hello");
    buf_n = sprintf(buf, "hello %s\n", "world");
    EXP("hello world\n");
    buf_n = sprintf(buf, "int: %d", 100);
    EXP("int: 100");
    buf_n = sprintf(buf, "int: %d, hex: %x, octal: %o a", 100, 100, 100);
    EXP("int: 100, hex: 64, octal: 144 a");

    buf_n = sprintf(buf, "int: %02d", 1);
    EXP("int: 01");
    buf_n = sprintf(buf, "int: %2d", 1);
    EXP("int:  1");
    buf_n = sprintf(buf, "int: %00d", 1);
    EXP("int: 1");

    buf_n = sprintf(buf, "int: %d", -1337);
    EXP("int: -1337");

    buf_n = sprintf(buf, "int: %u", 1337);
    EXP("int: 1337");

    buf_n = sprintf(buf, "hex: %02x", 1);
    EXP("hex: 01");
    buf_n = sprintf(buf, "hex: %2x", 1);
    EXP("hex:  1");
    buf_n = sprintf(buf, "hex: %00x", 1);
    EXP("hex: 1");
    buf_n = sprintf(buf, "hex: %x", 0x622933f2);
    EXP("hex: 622933f2");

    buf_n = sprintf(buf, "oct: %02o", 1);
    EXP("oct: 01");
    buf_n = sprintf(buf, "oct: %2o", 1);
    EXP("oct:  1");
    buf_n = sprintf(buf, "oct: %00o", 1);
    EXP("oct: 1");

    buf_n = sprintf(buf, "int: %-2dend", 1);
    EXP("int: 1 end");
    buf_n = sprintf(buf, "int: %-2dend", 12);
    EXP("int: 12end");
    buf_n = sprintf(buf, "int: %-0dend", 1);
    EXP("int: 1end");

    buf_n = sprintf(buf, "int: %-2xend", 1);
    EXP("int: 1 end");
    buf_n = sprintf(buf, "int: %-2xend", 12);
    EXP("int: c end");
    buf_n = sprintf(buf, "int: %-0xend", 1);
    EXP("int: 1end");

    buf_n = sprintf(buf, "int: %-2oend", 1);
    EXP("int: 1 end");
    buf_n = sprintf(buf, "int: %-2oend", 12);
    EXP("int: 14end");
    buf_n = sprintf(buf, "int: %-0oend", 1);
    EXP("int: 1end");

    buf_n = sprintf(buf, "string: %.3s", "foobar");
    EXP("string: foo");
    buf_n = sprintf(buf, "string: %.4s", "foobar");
    EXP("string: foob");

    buf_n = sprintf(buf, "string: %s\n", "ABCDEFGHIJKLMNOPQRSTUVXYZ");
    EXP("string: ABCDEFGHIJKLMNOPQRSTUVXYZ\n");

    buf_n = sprintf(buf, "string: %6send", "hello");
    EXP("string:  helloend");
    buf_n = sprintf(buf, "string: %-6send", "hello");
    EXP("string: hello end");
    const char *data = "hello";
    char *foo = "foo";
    buf_n = sprintf(buf, "       %-20s Legacy alias for \"%s\"\n", data, foo);
    EXP("       hello                Legacy alias for \"foo\"\n");
    buf_n = sprintf(buf, "       %-20s %s%s\n", data, foo, "bar");
    EXP("       hello                foobar\n");
    // Make sure that parameters have not changed
    assert(0 == strcmp(data, "hello"));
    assert(0 == strcmp(foo, "foo"));

    /* snprintf test */
    char out_buffer[100];
    memset(out_buffer, 'A', 100);
    assert(5 == snprintf(out_buffer, 5, "hello"));
    memcmp("helloAAAAA", out_buffer, 10);
    assert(10 == snprintf(out_buffer, 5, "hellofoooo"));
    memcmp("helloAAAAA", out_buffer, 10);
    assert(10 == snprintf(out_buffer, 10, "hellofoooo"));
    memcmp("hellofoookAA", out_buffer, 12);
  }
  dbgln("printf TEST PASSED");
}

int cmpfunc(const void *a, const void *b) { return (*(char *)a - *(char *)b); }

void qsort_test(void) {
  dbgln("qsort TEST");
  {
    char buf[] = {'B', 'D', 'C', 'A', '\0'};
    qsort(buf, 4, sizeof(char), cmpfunc);
    assert(0 == memcmp(buf, "ABCD", 4));
  }
  dbgln("qsort TEST PASSED");
}

void basename_test(void) {
  dbgln("basename TEST");
  {
    char path[100];
    const char *s;
    strcpy(path, "/foo/bar");
    s = basename(path);
    assert(0 == strcmp(s, "bar"));

    strcpy(path, "/foo/bar/");
    s = basename(path);
    assert(0 == strcmp(s, "bar"));

    strcpy(path, "/");
    s = basename(path);
    assert(0 == strcmp(s, "/"));

    strcpy(path, "//");
    s = basename(path);
    assert(0 == strcmp(s, "/"));

    strcpy(path, "");
    s = basename(path);
    assert(0 == strcmp(s, "."));

    s = basename(NULL);
    assert(0 == strcmp(s, "."));
  }
  dbgln("basename TEST PASSED");
}

void dirname_test(void) {
  dbgln("dirname TEST");
  {
    char path[100];
    const char *s;
    strcpy(path, "/foo/bar");
    s = dirname(path);
    assert(0 == strcmp(s, "foo"));

    strcpy(path, "/foo/bar/");
    s = dirname(path);
    assert(0 == strcmp(s, "foo"));

    strcpy(path, "/foo/bar/zoo");
    s = dirname(path);
    assert(0 == strcmp(s, "bar"));
  }
  dbgln("dirname TEST PASSED");
}

int main(void) {
  dbgln("START");
  malloc_test();
  // json_test();
  // random_test();
  isx_test();
  memcmp_test();
  memcpy_test();
  memmove_test();
  strlen_test();
  strnlen_test();
  sscanf_test();
  strtoul_test();
  strcspn_test();
  strpbrk_test();
  strcpy_test();
  strncpy_test();
  strlcpy_test();
  strcat_test();
  strchr_test();
  strrchr_test();
  strdup_test();
  strndup_test();
  strspn_test();
  strtol_test();
  strcmp_test();
  strncmp_test();
  strcasecmp_test();
  strncasecmp_test();
  strstr_test();
  strtok_test();
  abs_test();
  //  fseek_test();
  printf_test();
  qsort_test();
  basename_test();
  dirname_test();
  // TODO: Add mkstemp
  return 0;
}
