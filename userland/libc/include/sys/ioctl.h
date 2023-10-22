#ifndef IOCTL_H
#define IOCTL_H
#define TIOCGWINSZ 0
struct winsize {
  unsigned short ws_row;
  unsigned short ws_col;
  unsigned short ws_xpixel;
  unsigned short ws_ypixel;
};
#endif // IOCTL_H
