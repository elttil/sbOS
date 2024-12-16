#include <arpa/inet.h>
#include <assert.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <sys/socket.h>

int rdate_main(int argc, char **argv) {
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == fd) {
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
  if (0 != rc) {
    fprintf(stderr, "getaddrinfo: %s\n", gai_strerror(rc));
    return 1;
  }

  if (-1 ==
      connect(fd, (struct sockaddr *)result->ai_addr, result->ai_addrlen)) {
    perror("connect");
    return 1;
  }

  uint32_t t;
  if (-1 == (rc = read(fd, &t, sizeof(t)))) {
    perror("read");
    return 1;
  }

  close(fd);

  if (sizeof(t) != rc) {
    fprintf(stderr, "Invalid message recieved from time server.\n");
    return 1;
  }

  t = ntohl(t);

  int64_t unix_time = (t - 2208988800) * 1000;
  int clock_fd = open("/dev/clock", O_RDWR);
  int64_t current;
  dprintf(clock_fd, "%lld", unix_time);
  close(clock_fd);
  freeaddrinfo(result);
  return 0;
}
