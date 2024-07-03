#include <stdio.h>
/* General constants */
#define fd_stdin 0  /* standard in file descriptor */
#define fd_stdout 1 /* standard out file descriptor */
#define ASSERT_NOT_REACHED assert(0);
/* END General constants */

/* CAT SETTINGS */
#define CAT_BUFFER 4096 /* size of the read buffer */
/* END CAT SETTINGS */

/* HTTP SETTINGS */
#define HTTP_MAX_BUFFER 4096      /* size of the read buffer */
#define HTTP_DEFAULT_PORT 1337    /* default port should -p not be supplied */
#define HTTP_WEBSITE_ROOT "site/" /* default directory that it chroots into */

/* should xxx.html not exist these will be supplied instead */
#define HTTP_DEFAULT_404_SITE "404 - Not Found"
#define HTTP_DEFAULT_400_SITE "400 - Bad Request"
/* END HTTP SETTINGS */

#define COND_PERROR_EXP(condition, function_name, expression)                  \
  if (condition) {                                                             \
    perror(function_name);                                                     \
    expression;                                                                \
  }

int minibox_main(int argc, char **argv);
int ascii_main(int argc, char **argv);
int echo_main(int argc, char **argv);
int http_main(int argc, char **argv);
int lock_main(int argc, char **argv);
int yes_main(int argc, char **argv);
int cat_main(int argc, char **argv);
int pwd_main(int argc, char **argv);
int wc_main(int argc, char **argv);
int ls_main(int argc, char **argv);
int touch_main(int argc, char **argv);
int ed_main(int argc, char **argv);
int sh_main(int argc, char **argv);
int kill_main(int argc, char **argv);
int init_main(void);
int sha1sum_main(int argc, char **argv);
int rdate_main(int argc, char **argv);
