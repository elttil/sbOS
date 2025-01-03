#include <arpa/inet.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <queue.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/sendfile.h>
#include <sys/socket.h>
#include <sys/stat.h>
#include <tb/sb.h>
#include <tb/sv.h>
#include <unistd.h>

#define TYPE_LISTEN_SOCKET 1
#define TYPE_LISTEN_CLIENT 2

#define REQUEST_GET_DATA 0
#define REQUEST_SEND_HEADER 1
#define REQUEST_SEND_DATA 2

struct sb response;

struct http_request {
  int socket;
  char buffer[8192];
  struct sb client_buffer;
  int state;

  int file_fd;
  int is_directory;
  int status_code;
  int header_sent;
};

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

void request_reinit(struct http_request *request, int q) {
  request->state = REQUEST_GET_DATA;

  sb_reset(&request->client_buffer);
  struct queue_entry e;
  e.fd = request->socket;
  e.data = request;
  e.data_type = TYPE_LISTEN_CLIENT;
  e.listen = QUEUE_WAIT_READ | QUEUE_WAIT_CLOSE;
  queue_mod_entries(q, QUEUE_MOD_CHANGE, &e, 1);
}

void httpd_handle_incoming(int socket, int q) {
  for (;;) {
    struct sockaddr_in cli;

    socklen_t len = sizeof(cli);
    int connfd = accept(socket, (struct sockaddr *)&cli, &len);
    if (-1 == connfd) {
      break;
    }

    int flag = 1;
    assert(0 == setsockopt(connfd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag,
                           sizeof(int)));

    struct queue_entry e;
    e.fd = connfd;
    struct http_request *request = malloc(sizeof(struct http_request));
    assert(request); // TODO: Don't assume malloc succeeds
    request->state = REQUEST_GET_DATA;

    set_fd_nonblocking(connfd);
    request->socket = connfd;

    sb_init_buffer(&request->client_buffer, request->buffer,
                   sizeof(request->buffer));
    e.data = request;
    e.data_type = TYPE_LISTEN_CLIENT;
    e.listen = QUEUE_WAIT_READ | QUEUE_WAIT_CLOSE;
    queue_mod_entries(q, QUEUE_MOD_ADD, &e, 1);
  }
}

void request_try_file(struct http_request *request, const char *path) {
  int fd = open(path, O_RDONLY);
  if (-1 == fd) {
    request->status_code = 404;
    return;
  }
  if (-1 != request->file_fd) {
    close(request->file_fd);
  }
  request->file_fd = fd;
  struct stat statbuf;
  if (-1 == fstat(request->file_fd, &statbuf)) {
    close(request->file_fd);
    request->status_code = 500;
    // The only possible error(I believe)
    assert(ENOMEM == errno);
    return;
  }
  request->is_directory = S_ISDIR(statbuf.st_mode);
  request->status_code = 200;
}

void parse_incoming_request(struct http_request *request) {
  struct sv l = SB_TO_SV(request->client_buffer);
  struct sv method = sv_split_delim(l, &l, ' ');
  struct sv path = sv_split_space(l, &l);

  request->file_fd = -1;
  request->is_directory = 0;

  char path_buffer[256 + 11];
  if (sv_eq(method, C_TO_SV("GET"))) {
    // The difference in buffer size is intentional to be able to append
    // '/index.html' later
    sv_to_cstring_buffer(path, path_buffer, 256);
    request_try_file(request, path_buffer);

    if (request->is_directory) {
      strcat(path_buffer, "/index.html");
      request_try_file(request, path_buffer);
      request->status_code = 200;
      return;
    }
  } else {
    request->status_code = 400;
  }

  if (200 != request->status_code) {
    const int tmp = request->status_code;
    sprintf(path_buffer, "/%3d.html", request->status_code);
    request_try_file(request, path_buffer);
    request->status_code = tmp;
  }
  return;
}

void httpd_handle_client(int q, int fd, void *data) {
  struct queue_entry e;
  struct http_request *request = data;
  if (REQUEST_GET_DATA == request->state) {
    char buffer[4096];
    int rc;
    if (-1 == (rc = read(fd, buffer, 4096))) {
      if (EWOULDBLOCK != errno) {
        goto end_connection;
      }
      perror("read");
      return;
    }
    sb_append_buffer(&request->client_buffer, buffer, rc);
    struct sv string_view = SB_TO_SV(request->client_buffer);
    if (!sv_eq(C_TO_SV("\r\n\r\n"), sv_take_end(string_view, NULL, 4))) {
      return;
    }

    e.fd = fd;
    e.data = data;
    e.data_type = TYPE_LISTEN_CLIENT;
    e.listen = QUEUE_WAIT_WRITE | QUEUE_WAIT_CLOSE;
    queue_mod_entries(q, QUEUE_MOD_CHANGE, &e, 1);

    parse_incoming_request(request);

    request->header_sent = 0;
    request->state = REQUEST_SEND_HEADER;
  }
  if (REQUEST_SEND_HEADER == request->state) {
    char buffer[4096];
    if (request->is_directory) {
      sprintf(buffer, "HTTP/1.1 %d\r\nKeep-Alive: timeout=5, max=1000\r\n",
              request->status_code);
    } else if (-1 == request->file_fd) {
      // This sends more than the header...
      char error_message[4096];
      int rc = sprintf(error_message, "<!DOCTYPE html><html>Error %3d<br>\
The server administrator was too lazy to create a custom error page for this.</html>",
                       request->status_code);
      sprintf(buffer,
              "HTTP/1.1 %d\r\nKeep-Alive: timeout=5, "
              "max=1000\r\nContent-Length: %d\r\n\r\n%s",
              request->status_code, rc, error_message);
      request_reinit(request, q);
    } else {
      struct stat statbuf;
      if (-1 == fstat(request->file_fd, &statbuf)) { // TODO: Use the old
                                                     // fstat call
        perror("fstat");
        assert(0);
      }
      sprintf(buffer,
              "HTTP/1.1 %d\r\nKeep-Alive: timeout=5, "
              "max=1000\r\nContent-Length: %llu\r\n\r\n",
              request->status_code, statbuf.st_size);
    }
    size_t left = strlen(buffer) - request->header_sent;
    if (left > 0) {
      int rc;
      if (-1 == (rc = write(fd, buffer + request->header_sent, left))) {
        if (EWOULDBLOCK != errno) {
          goto end_connection;
        }
        return;
      }
      request->header_sent += rc;
    }
    request->state = REQUEST_SEND_DATA;
  }
  if (REQUEST_SEND_DATA == request->state) {
    if (request->is_directory) {
      // No attempt at buffering directory listing
      // since that is annoying and not as if a
      // directory listing is super big or
      // important.
      // TODO: Do this when I am less lazy
      DIR *directory = fdopendir(request->file_fd);
      char path[256];
      (void)getcwd(path, 256);

      sb_reset(&response);
      sb_append_sv(&response, C_TO_SV("<!DOCTYPE html><html>List of "));
      sb_append(&response, path);

      for (;;) {
        struct dirent entries[128];
        int n = readdir_multi(directory, entries, 128);
        if (-1 == n) {
          goto end_connection;
        }
        if (0 == n) {
          break;
        }
        for (int i = 0; i < n; i++) {
          sb_append_sv(&response, C_TO_SV("<br><a href=\""));
          sb_append(&response, path);
          sb_append(&response, entries[i].d_name);
          sb_append_sv(&response, C_TO_SV("\">"));
          sb_append(&response, entries[i].d_name);

          sb_append_sv(&response, C_TO_SV("</a>"));
        }
      }
      closedir(directory);
      request->file_fd = -1; // The fd gets closed by the closedir call.
      sb_append_sv(&response, C_TO_SV("</html>"));

      struct sv s = SB_TO_SV(response);
      char buffer[4096];
      int rc = sprintf(buffer, "Content-Length: %lu\r\n\r\n", s.length);

      request_reinit(request, q);

      sb_prepend_buffer(&response, buffer, rc);
      s = SB_TO_SV(response);

      if (-1 == write(request->socket, s.s, s.length)) {
        if (EWOULDBLOCK != errno) {
          goto end_connection;
        }
        return;
      }
      return;
    }

    int err;
    int rc = sendfile(request->socket, request->file_fd, NULL, 8192, &err);
    if (0 != err) {
      if (EWOULDBLOCK != errno) {
        goto end_connection;
      }
      return;
    }
    if (8192 == rc) {
      return;
    }
    close(request->file_fd);
    request->file_fd = -1;
    request_reinit(request, q);
  }
  return;
end_connection:
  free(request);
  close(fd);
  e.fd = fd;
  e.data = NULL;
  e.data_type = TYPE_LISTEN_CLIENT;
  e.listen = 0;
  queue_mod_entries(q, QUEUE_MOD_DELETE, &e, 1);
}

void httpd_loop(int socket) {
  int q = queue_create();
  struct queue_entry e;
  e.fd = socket;
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
        httpd_handle_incoming(socket, q);
        break;
      case TYPE_LISTEN_CLIENT:
        httpd_handle_client(q, events[i].fd, events[i].data);
        break;
      }
    }
  }
}

int main(void) {
  sb_init(&response);

  int sockfd = socket(AF_INET, SOCK_NONBLOCK | SOCK_STREAM, 0);
  if (-1 == sockfd) {
    perror("socket");
    return 1;
  }

  struct sockaddr_in servaddr;
  memset(&servaddr, 0, sizeof(servaddr));

  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.s_addr = htonl(INADDR_ANY);
  servaddr.sin_port = htons(80);

  if (-1 == bind(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr))) {
    perror("bind");
    return 1;
  }

  set_fd_nonblocking(sockfd);

  httpd_loop(sockfd);

  close(sockfd);
  return 0;
}
