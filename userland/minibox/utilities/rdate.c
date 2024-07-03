#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

int rdate_main(int argc, char **argv) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return 1;
  }

  struct addrinfo *result = NULL;
  struct addrinfo hints;
  memset(&hints, 0, sizeof(hints));
  hints.ai_socktype = SOCK_STREAM;
  hints.ai_protocol = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;

  int rc = getaddrinfo("time-d-g.nist.gov", "37", &hints, &result);
  if (-1 == rc) {
    fprintf(stderr, "Error in getaddrinfo()\n");
    return 1;
  }

  if (connect(fd, (struct sockaddr *)result->ai_addr, result->ai_addrlen) < 0) {
    perror("connect");
    return 1;
  }

  uint32_t t;
  read(fd, &t, sizeof(t));
  t = ntohl(t);

  close(fd);

  int64_t unix_time = (t - 2208988800) * 1000;
  int clock_fd = open("/dev/clock", O_RDWR);
  int64_t current;
  read(clock_fd, &current, sizeof(int64_t));
  write(clock_fd, &unix_time, sizeof(int64_t));
  int64_t delta = (current / 1000) - (unix_time / 1000);
  printf("delta: %d\n", delta);
  return 0;
}
