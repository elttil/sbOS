// Very unfinished application that can view images and set the
// wallpaper. It only supports ppm3 and ppm6 files.
#include <assert.h>
#include <fcntl.h>
#include <libgui.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

GUI_Window *global_w;

struct PPM_IMAGE {
  uint8_t version;
  uint32_t width;
  uint32_t height;
  uint32_t maxval;
  long file_location;
};

int parse_ppm_header(FILE *fp, struct PPM_IMAGE *img) {
  char magic[3];
  int width;
  int height;
  int maxval;

  int v = 0;

  char c1;
  char c2;
  for (; v < 3;) {
    char c = fgetc(fp);
    if ('#' != c) {
      ungetc(c, fp);
      if (0 == v) {
        // TODO: Use %2s when scanf supports that
        fscanf(fp, "%c%c", &c1, &c2);
      }
      if (1 == v) {
        fscanf(fp, "%d %d", &width, &height);
      }
      if (2 == v) {
        fscanf(fp, "%d", &maxval);
      }

      v++;
    }
    for (; '\n' != fgetc(fp);)
      ;
  }

  if ('P' != c1) {
    printf("c1: %c\n", c1);
    return 0;
  }
  if (!isdigit(c2)) {
    printf("c2: %c\n", c2);
    return 0;
  }
  img->version = c2 - '0';
  if (3 != img->version && 6 != img->version) {
    printf("Not handeld version\n");
    return 0;
  }
  if (maxval > 255) {
    printf("maxval is: %d\n", maxval);
    return 0;
  }
  img->width = width;
  img->height = height;
  img->maxval = maxval;
  img->file_location = ftell(fp);
  return 1;
}

inline uint32_t low_32_reverse(uint32_t _value) {
  return (((_value & 0x000000FF) << 24) | ((_value & 0x0000FF00) << 8) |
          ((_value & 0x00FF0000) >> 8));
}

int load_ppm6_file(FILE *fp, const struct PPM_IMAGE *img, uint32_t buf_width,
                   uint32_t buf_height, uint32_t *buffer) {
  fseek(fp, img->file_location, SEEK_SET);
  const uint8_t modifier = 255 / img->maxval;
  int cx = 0;
  int cy = 0;
  const int n_pixels = img->width * img->height;
  uint8_t *rgb = malloc(3 * n_pixels);

  const int rc = fread(rgb, 3, n_pixels, fp);
  if (0 == rc) {
    return 0;
  }

  uint32_t *p = rgb;
  u32 buf_size = buf_height * buf_width;
  if (1 == modifier) {
    int c = 0;
    for (int i = 0; i < rc && cy * buf_width + cx < buf_size; i++, c += 3) {
      if (cx > buf_width) {
        i--;
      } else {
        u8 red = rgb[c];
        u8 green = rgb[c + 1];
        u8 blue = rgb[c + 2];
        buffer[cy * buf_width + cx] =
            (red << (8 * 2)) | (green << (8 * 1)) | (blue << (8 * 0));
      }
      cx++;
      if (cx == img->width) {
        cx = 0;
        cy++;
        i--;
        continue;
      }
    }
  } else {
    for (int i = 0; i < rc && cy * buf_width + cx < buf_size;
         i++, p = ((uint8_t *)p) + 3) {
      ((uint8_t *)p)[0] *= modifier;
      ((uint8_t *)p)[1] *= modifier;
      ((uint8_t *)p)[2] *= modifier;
      if (cx > buf_width) {
        i--;
      } else {
        uint32_t v = *p;
        buffer[cy * buf_width + cx] =
            ((v & 0xFF) << 16) | ((v & 0xFF00)) | ((v & 0xFF0000) >> 16);
      }
      cx++;
      if (cx == img->width) {
        cx = 0;
        cy++;
        i--;
        continue;
      }
    }
  }
  return 1;
}

int load_ppm3_file(FILE *fp, const struct PPM_IMAGE *img, uint32_t buf_width,
                   uint32_t buf_height, uint32_t *buffer) {
  fseek(fp, img->file_location, SEEK_SET);
  uint8_t modifier = 255 / img->maxval;
  int cx = 0;
  int cy = 0;
  for (int i = 0; i < img->width * img->height; i++) {
    int red;
    int green;
    int blue;
    int rc = fscanf(fp, "%d %d %d", &red, &green, &blue);
    if (0 == rc) {
      break;
    }
    red &= 0xFF;
    green &= 0xFF;
    blue &= 0xFF;
    red *= modifier;
    green *= modifier;
    blue *= modifier;
    if (3 != rc) {
      printf("not 3: %d\n", rc);
      return 0;
    }
    buffer[cy * buf_width + cx] =
        (red << 8 * 2) | (green << 8 * 1) | (blue << 8 * 0);
    cx++;
    if (cx == img->width) {
      cx = 0;
      cy++;
      continue;
    }
  }
  return 1;
}

int load_ppm_file(FILE *fp, const struct PPM_IMAGE *img, uint32_t buf_width,
                  uint32_t buf_height, uint32_t *buffer) {
  if (3 == img->version) {
    return load_ppm3_file(fp, img, buf_width, buf_height, buffer);
  } else if (6 == img->version) {
    return load_ppm6_file(fp, img, buf_width, buf_height, buffer);
  }
  return 0;
}

struct DISPLAY_INFO {
  uint32_t width;
  uint32_t height;
  uint8_t bpp;
};

int main(int argc, char **argv) {
  if (argc < 2) {
    printf("provide command line arguments dumbass\n");
    return 1;
  }
  argv++;

  int set_wallpaper = 0;
  // Parse commandline arguments(shitty)
  if (0 == strcmp(*argv, "-w")) {
    set_wallpaper = 1;
    argv++;
  }

  const char *file = *argv;

  struct PPM_IMAGE img;
  FILE *fp = fopen(file, "rb");
  assert(fp);
  assert(parse_ppm_header(fp, &img));

  if (!set_wallpaper) {
    global_w = GUI_CreateWindow(80, 80, img.width, img.height);
    assert(global_w);
    assert(
        load_ppm_file(fp, &img, img.width, img.height, global_w->bitmap_ptr));
    GUI_UpdateWindow(global_w);
    fclose(fp);
    GUI_EventLoop(global_w, NULL);
  } else {
    int wallpaper_fd;
    do {
      wallpaper_fd = shm_open("wallpaper", O_RDWR | O_CREAT, 0);
    } while (-1 == wallpaper_fd);

    struct DISPLAY_INFO inf;
    int fd = open("/dev/display_info", O_READ, 0);
    assert(fd >= 0);
    assert(sizeof(inf) == read(fd, &inf, sizeof(inf)));

    ftruncate(wallpaper_fd, inf.width * inf.height * sizeof(uint32_t));
    void *rc = mmap(NULL, inf.width * inf.height * sizeof(uint32_t), 0, 0,
                    wallpaper_fd, 0);
    assert(rc);
    assert(load_ppm_file(fp, &img, inf.width, inf.height, rc));
    close(fd);
  }
  return 0;
}
