#include <arpa/inet.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <unistd.h>

void usage(char *program_name) {
  fprintf(stderr, "Usage: %s host port program [arg...]\n", program_name);
}

int main(int argc, char **argv) {
  if (argc < 4) {
    usage(*argv);
    return 1;
  }
  char *host = *(argv + 1);
  if ('-' == *host) {
    fprintf(stderr, "Sorry, command line arguments are not supported\n");
    return 1;
  }
  char *port = *(argv + 2);
  char *program = *(argv + 3);
  // The arguments beign at argv + 4 but
  // argv[0] should be "program" anyways.
  char **program_arguments = argv + 3;

  int port_number = atoi(port);
  if (0 == port_number) {
    fprintf(stderr, "Invalid port: %s\n", port);
    return 1;
  }

  int sockfd = socket(AF_INET, SOCK_STREAM, 0);
  if (-1 == sockfd) {
    perror("socket");
    return 1;
  }

  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port_number);

  if (-1 == bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
    perror("bind");
    return 1;
  }

  for (;;) {
    struct sockaddr_in cli;
    socklen_t len = sizeof(cli);
    int connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
    if (-1 == connfd) {
      perror("accept");
      continue;
    }

    if (0 == fork()) {
      dup2(connfd, 0);
      dup2(connfd, 1);
      assert(-1 == execv(program, program_arguments));
      perror("execv");
      return 1;
    }
    close(connfd);
  }

  close(sockfd);
  return 0;
}
