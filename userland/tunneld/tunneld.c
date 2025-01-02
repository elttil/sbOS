#include <assert.h>
#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include <arpa/inet.h>
#include <math.h>
#include <netdb.h>
#include <netinet/in.h>
#include <queue.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <unistd.h>

#include <bearssl.h>

#define TYPE_LISTEN_SOCKET 1
#define TYPE_LISTEN_CLIENT 2
#define TYPE_LISTEN_SERVER 3

int serial_fd = -1;

br_ec_private_key EC;
br_x509_certificate CHAIN[1];
#define CHAIN_LEN 1

#define SKEY EC

int set_fd_nonblocking(int fd) {
  int flags = fcntl(fd, F_GETFL, 0);
  if (-1 == flags) {
    return 0;
  }
  flags |= O_NONBLOCK;
  if (0 != fcntl(fd, F_SETFL, flags)) {
    return 0;
  }
  return 1;
}

struct tunnel {
  int socket;
  int server_fd;
  unsigned char iobuf[BR_SSL_BUFSIZE_BIDI];
  br_ssl_server_context sc;
  struct queue_entry client_entry;
  struct queue_entry server_entry;
};

void update_socket(int q, struct tunnel *ctx, int type, int state) {
  if (TYPE_LISTEN_CLIENT == type) {
    if ((ctx->client_entry.listen & (QUEUE_WAIT_READ | QUEUE_WAIT_WRITE)) ==
        state) {
      return;
    }
    ctx->client_entry.listen = state | QUEUE_WAIT_CLOSE;
    queue_mod_entries(q, QUEUE_MOD_CHANGE, &ctx->client_entry, 1);
  } else if (TYPE_LISTEN_SERVER == type) {
    if ((ctx->server_entry.listen & (QUEUE_WAIT_READ | QUEUE_WAIT_WRITE)) ==
        state) {
      return;
    }
    ctx->server_entry.listen = state | QUEUE_WAIT_CLOSE;
    queue_mod_entries(q, QUEUE_MOD_CHANGE, &ctx->server_entry, 1);
  }
}

void handle_incoming(int sockfd, int q, int dst_port) {
  struct sockaddr_in cli;

  socklen_t len = sizeof(cli);
  int connfd = accept(sockfd, (struct sockaddr *)&cli, &len);
  if (-1 == connfd) {
    return;
  }

  int flag = 1;
  assert(0 == setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
                         sizeof(int)));

  set_fd_nonblocking(connfd);

  struct tunnel *ctx = malloc(sizeof(struct tunnel));
  if (!ctx) {
    close(connfd);
    return;
  }
  ctx->socket = connfd;

  br_ssl_server_init_full_ec(&ctx->sc, CHAIN, CHAIN_LEN, BR_KEYTYPE_EC, &SKEY);

  br_ssl_engine_set_buffer(&ctx->sc.eng, &ctx->iobuf, sizeof(ctx->iobuf), 1);
  br_ssl_server_reset(&ctx->sc);

  struct sockaddr_in servaddr;
  int server_fd = socket(AF_INET, SOCK_STREAM | SOCK_NONBLOCK, 0);
  if (server_fd < 0) {
    perror("socket");
    return;
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.a[0] = 127;
  servaddr.sin_addr.a[1] = 0;
  servaddr.sin_addr.a[2] = 0;
  servaddr.sin_addr.a[3] = 1;
  servaddr.sin_port = htons(dst_port);

  if (-1 ==
      connect(server_fd, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
    perror("connect");
    return;
  }

  flag = 1;
  assert(0 == setsockopt(server_fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
                         sizeof(int)));

  ctx->server_fd = server_fd;

  ctx->client_entry.fd = ctx->socket;
  ctx->client_entry.data = ctx;
  ctx->client_entry.data_type = TYPE_LISTEN_CLIENT;
  ctx->client_entry.listen =
      QUEUE_WAIT_WRITE | QUEUE_WAIT_READ | QUEUE_WAIT_CLOSE;
  queue_mod_entries(q, QUEUE_MOD_ADD, &ctx->client_entry, 1);

  ctx->server_entry.fd = ctx->server_fd;
  ctx->server_entry.data = ctx;
  ctx->server_entry.data_type = TYPE_LISTEN_SERVER;
  ctx->server_entry.listen = QUEUE_WAIT_READ | QUEUE_WAIT_CLOSE;
  queue_mod_entries(q, QUEUE_MOD_ADD, &ctx->server_entry, 1);
}

void close_connection(int q, struct tunnel *ctx) {
  close(ctx->socket);
  close(ctx->server_fd);

  queue_mod_entries(q, QUEUE_MOD_DELETE, &ctx->server_entry, 1);
  queue_mod_entries(q, QUEUE_MOD_DELETE, &ctx->client_entry, 1);

  free(ctx);
  return;
}

void update_state(int q, struct tunnel *ctx, unsigned state) {
  if (BR_SSL_RECVAPP & state) {
    update_socket(q, ctx, TYPE_LISTEN_SERVER,
                  QUEUE_WAIT_WRITE | QUEUE_WAIT_READ);
  }
  if (BR_SSL_SENDREC & state) {
    update_socket(q, ctx, TYPE_LISTEN_CLIENT,
                  QUEUE_WAIT_WRITE | QUEUE_WAIT_READ);
  }
  if (!(state & BR_SSL_RECVAPP)) {
    update_socket(q, ctx, TYPE_LISTEN_SERVER, QUEUE_WAIT_READ);
  }
  if (!(state & BR_SSL_SENDREC)) {
    update_socket(q, ctx, TYPE_LISTEN_CLIENT, QUEUE_WAIT_READ);
  }
}

void handle_server(int q, int fd, struct tunnel *ctx) {
  for (;;) {
    unsigned state = br_ssl_engine_current_state(&ctx->sc.eng);
    if (BR_SSL_CLOSED & state) {
      close_connection(q, ctx);
      return;
    }
    update_state(q, ctx, state);
    if (!(state & BR_SSL_SENDAPP) && !(state & BR_SSL_RECVAPP)) {
      break;
    }
    if (BR_SSL_SENDAPP & state) {
      size_t len;
      char *buffer;
      if (NULL == (buffer = br_ssl_engine_sendapp_buf(&ctx->sc.eng, &len))) {
        continue;
      }
      int rc = read(fd, buffer, len);
      if (-1 == rc) {
        if (EWOULDBLOCK != errno) {
          close_connection(q, ctx);
          continue;
        }
      }
      if (0 == rc) {
        close_connection(q, ctx);
        continue;
      }
      if (rc > 0) {
        br_ssl_engine_sendapp_ack(&ctx->sc.eng, rc);
        br_ssl_engine_flush(&ctx->sc.eng, 0);
      }
    }
    if (BR_SSL_RECVAPP & state) {
      size_t len;
      char *buffer;
      if (NULL == (buffer = br_ssl_engine_recvapp_buf(&ctx->sc.eng, &len))) {
        continue;
      }

      int rc = write(fd, buffer, len);
      if (-1 == rc) {
        if (EWOULDBLOCK != errno) {
          close_connection(q, ctx);
          return;
        }
      }
      if (0 == rc) {
        close_connection(q, ctx);
        return;
      }
      if (rc > 0) {
        br_ssl_engine_recvapp_ack(&ctx->sc.eng, rc);
      }
    }
  }
}

void handle_client(int q, int fd, struct tunnel *ctx) {
  for (;;) {
    unsigned state = br_ssl_engine_current_state(&ctx->sc.eng);
    if (BR_SSL_CLOSED & state) {
      close_connection(q, ctx);
      return;
    }
    update_state(q, ctx, state);
    if (!(state & BR_SSL_SENDREC) && !(state & BR_SSL_RECVREC)) {
      break;
    }
    if (BR_SSL_RECVREC & state) {
      size_t len;
      char *buffer;
      if (NULL == (buffer = br_ssl_engine_recvrec_buf(&ctx->sc.eng, &len))) {
        return;
      }
      int rc = read(fd, buffer, len);
      if (-1 == rc) {
        if (EWOULDBLOCK != errno) {
          close_connection(q, ctx);
          return;
        }
        if (!(state & BR_SSL_SENDREC)) {
          break;
        }
      }
      if (0 == rc) {
        close_connection(q, ctx);
        return;
      }
      if (rc > 0) {
        len = min(len, rc);
        br_ssl_engine_recvrec_ack(&ctx->sc.eng, len);
      }
    }

    if (BR_SSL_SENDREC & state) {
      size_t len;
      char *buffer;
      if (NULL == (buffer = br_ssl_engine_sendrec_buf(&ctx->sc.eng, &len))) {
        return;
      }
      int rc = write(fd, buffer, len);
      if (-1 == rc) {
        if (EWOULDBLOCK != errno) {
          close_connection(q, ctx);
          return;
        }
      }
      if (rc > 0) {
        len = min(len, rc);
        br_ssl_engine_sendrec_ack(&ctx->sc.eng, len);
      }
    }
  }
  return;
}

void tunneld_loop(int sockfd, int dst_port) {
  int q = queue_create();
  struct queue_entry e;
  e.fd = sockfd;
  e.data = NULL;
  e.data_type = TYPE_LISTEN_SOCKET;
  e.listen = QUEUE_WAIT_READ;
  queue_mod_entries(q, QUEUE_MOD_ADD, &e, 1);
  for (;;) {
    struct queue_entry events[512];
    int rc = queue_wait(q, events, 512);
    if (0 == rc) {
      continue;
    }
    for (int i = 0; i < rc; i++) {
      switch (events[i].data_type) {
      case TYPE_LISTEN_SOCKET:
        handle_incoming(sockfd, q, dst_port);
        break;
      case TYPE_LISTEN_CLIENT:
        handle_client(q, events[i].fd, events[i].data);
        break;
      case TYPE_LISTEN_SERVER:
        handle_server(q, events[i].fd, events[i].data);
        break;
      }
    }
  }
}

void certificate_append(br_x509_certificate *ctx, u8 *buffer, size_t len) {
  ctx->data = realloc(ctx->data, ctx->data_len + len);
  memcpy(ctx->data + ctx->data_len, buffer, len);
  ctx->data_len += len;
}

int decode_x509(br_x509_decoder_context *c1, const char *path) {
  int fd = open(path, O_RDONLY);
  if (-1 == fd) {
    return 0;
  }

  br_pem_decoder_context c3;
  br_pem_decoder_init(&c3);

  br_x509_decoder_init(c1, NULL, NULL);

  CHAIN[0].data = NULL;
  CHAIN[0].data_len = 0;
  br_pem_decoder_setdest(
      &c3, (void (*)(void *, const void *, size_t))certificate_append,
      &CHAIN[0]);
  for (int offset = 0;;) {
    char buffer[4096];
    int rc = pread(fd, buffer, sizeof(buffer), offset);
    assert(-1 != rc);
    if (0 == rc) {
      break;
    }
    offset += br_pem_decoder_push(&c3, buffer, rc);
    int err = br_pem_decoder_event(&c3);
    printf("err: %d\n", err);
  }

  close(fd);
  return 1;
}

int decode_ec(br_skey_decoder_context *c2, const char *path,
              br_ec_private_key *key) {
  int fd = open(path, O_RDONLY);
  if (-1 == fd) {
    return 0;
  }

  br_pem_decoder_context c3;
  br_pem_decoder_init(&c3);

  br_skey_decoder_init(c2);
  for (int offset = 0;;) {
    char buffer[4096];
    int rc = pread(fd, buffer, sizeof(buffer), offset);
    assert(-1 != rc);
    if (0 == rc) {
      break;
    }
    offset += br_pem_decoder_push(&c3, buffer, rc);

    if (0 == strcmp("EC PRIVATE KEY", br_pem_decoder_name(&c3))) {
      br_pem_decoder_setdest(
          &c3, (void (*)(void *, const void *, size_t))br_skey_decoder_push,
          c2);
    }

    int err = br_pem_decoder_event(&c3);
    printf("err: %d\n", err);
  }

  printf("error: %d\n", br_skey_decoder_last_error(c2));
  memcpy(key, br_skey_decoder_get_ec(c2), sizeof(br_ec_private_key));
  close(fd);
  return 1;
}

int main(int argc, char **argv) {
  serial_fd = open("/dev/serial", O_RDWR);

  int port = 443;
  int dst_port = 80;

  char *cert_file = "/cert.pem";
  char *key_file = "/ec_key.pem";

  int opt;
  for (; -1 != (opt = getopt(argc, argv, "p:d:c:k:"));) {
    switch (opt) {
    case 'p':
      port = atoi(optarg);
      break;
    case 'd':
      dst_port = atoi(optarg);
      break;
    case 'c':
      cert_file = optarg;
      break;
    case 'k':
      key_file = optarg;
      break;
    case '?':
      printf("Unknown option: %c\n", optopt);
      break;
    }
  }

  br_x509_decoder_context c1;
  br_skey_decoder_context c2;
  decode_x509(&c1, cert_file);
  decode_ec(&c2, key_file, &EC);

  int fd = socket(AF_INET, SOCK_NONBLOCK | SOCK_STREAM, 0);
  if (-1 == fd) {
    perror("socket");
    return 1;
  }

  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(port);

  if (-1 == bind(fd, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
    perror("bind");
    return 1;
  }

  tunneld_loop(fd, dst_port);
  return 0;
}
