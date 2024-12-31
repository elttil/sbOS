#include "handle.h"
#include <arpa/inet.h>
#include <assert.h>
#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <tb/sv.h>
#include <unistd.h>

// FreeBSD does have it
// #define HAS_FDCLOSEDIR

#define min(a, b) (((a) < (b)) ? (a) : (b))

#define MAX_PACKET_SIZE 34000
#define MAX_READ_LEN 32768

uint64_t htonll(uint64_t x) {
  int n = 1;
  // little endian if true
  if (*(char *)&n == 1) {
    return ((((uint64_t)htonl(x)) << 32) + htonl((x) >> 32));
  }
  return x;
}

#define ntohll(x) htonll(x)

int send_error(struct sb *response, uint32_t id, int type);

#define SSH_FX_OK 0
#define SSH_FX_EOF 1
#define SSH_FX_NO_SUCH_FILE 2
#define SSH_FX_PERMISSION_DENIED 3
#define SSH_FX_FAILURE 4
#define SSH_FX_BAD_MESSAGE 5
#define SSH_FX_NO_CONNECTION 6
#define SSH_FX_CONNECTION_LOST 7
#define SSH_FX_OP_UNSUPPORTED 8

#define SSH_FXF_READ 0x00000001
#define SSH_FXF_WRITE 0x00000002
#define SSH_FXF_APPEND 0x00000004
#define SSH_FXF_CREAT 0x00000008
#define SSH_FXF_TRUNC 0x00000010
#define SSH_FXF_EXCL 0x00000020

#define SSH_FXP_INIT 1
#define SSH_FXP_VERSION 2
#define SSH_FXP_OPEN 3
#define SSH_FXP_CLOSE 4
#define SSH_FXP_READ 5
#define SSH_FXP_WRITE 6
#define SSH_FXP_LSTAT 7
#define SSH_FXP_FSTAT 8
#define SSH_FXP_SETSTAT 9
#define SSH_FXP_FSETSTAT 10
#define SSH_FXP_OPENDIR 11
#define SSH_FXP_READDIR 12
#define SSH_FXP_REMOVE 13
#define SSH_FXP_MKDIR 14
#define SSH_FXP_RMDIR 15
#define SSH_FXP_REALPATH 16
#define SSH_FXP_STAT 17
#define SSH_FXP_RENAME 18
#define SSH_FXP_READLINK 19
#define SSH_FXP_SYMLINK 20
#define SSH_FXP_STATUS 101
#define SSH_FXP_HANDLE 102
#define SSH_FXP_DATA 103
#define SSH_FXP_NAME 104
#define SSH_FXP_ATTRS 105
#define SSH_FXP_EXTENDED 200
#define SSH_FXP_EXTENDED_REPLY 201

#define SSH_FILEXFER_ATTR_SIZE 0x00000001
#define SSH_FILEXFER_ATTR_UIDGID 0x00000002
#define SSH_FILEXFER_ATTR_PERMISSIONS 0x00000004
#define SSH_FILEXFER_ATTR_ACMODTIME 0x00000008
#define SSH_FILEXFER_ATTR_EXTENDED 0x80000000

#define TAKE(buf, n) sv_read(*buffer, buffer, buf, n)

int log_fd = -1;

int force_read(int fd, void *buf, size_t nbytes) {
  for (; nbytes > 0;) {
    int rc = read(fd, buf, nbytes);
    if (-1 == rc) {
      return 0;
    }
    nbytes -= rc;
    buf = (void *)((uintptr_t)buf + rc);
  }
  return 1;
}

int sv_stat(struct sv path, struct stat *restrict sb) {
  char *p = SV_TO_C(path);
  int rc = stat(p, sb);
  free(p);
  return rc;
}

int sv_lstat(struct sv path, struct stat *restrict sb) {
  char *p = SV_TO_C(path);
  // TODO: Fix this when the OS has lstat
  //  int rc = lstat(p, sb);
  int rc = stat(p, sb);
  free(p);
  return rc;
}

char *sv_realpath(struct sv pathname, char *restrict resolved_path) {
  char *p = SV_TO_C(pathname);
  char *rc = realpath(p, resolved_path);
  free(p);
  return rc;
}

int client_has_sent_init = 0;

const uint32_t server_version = 3;
uint32_t client_version;

#define TRY_STATUS(exp)                                                        \
  if (!(exp)) {                                                                \
    send_error(reply, id, SSH_FX_BAD_MESSAGE);                                 \
    return 1;                                                                  \
  }

#define TRY(exp)                                                               \
  if (!(exp)) {                                                                \
    return 0;                                                                  \
  }

#define ERRNO_TRY(exp, str)                                                    \
  if (!(exp)) {                                                                \
    dprintf(log_fd, "%s: %s", str, strerror(errno));                           \
    return 0;                                                                  \
  }

void add_type(struct sb *s, uint8_t type) {
  sb_append_buffer(s, &type, sizeof(type));
}

void add_lu32(struct sb *s, uint32_t v) {
  v = htonl(v);
  sb_append_buffer(s, &v, sizeof(v));
}

int send_packet(struct sb *response, uint8_t type, const void *buf,
                uint32_t nbytes) {
  sb_append_buffer(response, &type, sizeof(type));
  sb_append_buffer(response, buf, nbytes);
  return 1;
}

int send_packet_id(struct sb *response, uint8_t type, uint32_t id,
                   const void *buf, uint32_t nbytes) {
  uint32_t length = nbytes + sizeof(type) + sizeof(id);
  length = htonl(length);
  sb_append_buffer(response, &length, sizeof(length));
  sb_append_buffer(response, &type, sizeof(type));
  id = htonl(id);
  sb_append_buffer(response, &id, sizeof(id));
  sb_append_buffer(response, buf, nbytes);
  return 1;
}

struct sv parse_string(struct sv *buffer, int *err) {
  if (err) {
    *err = 0;
  }
  uint32_t str_length;
  if (!TAKE(&str_length, sizeof(str_length))) {
    if (err) {
      *err = 1;
      return C_TO_SV("");
    }
  }
  str_length = ntohl(str_length);
  return sv_take(*buffer, buffer, str_length);
}

char *read_string(struct sv *buffer) {
  uint32_t str_length;
  TRY(TAKE(&str_length, sizeof(str_length)));
  str_length = ntohl(str_length);
  char *r = malloc(str_length + 1);
  if (!r) {
    return NULL;
  }
  if (!TAKE(r, str_length)) {
    free(r);
    return NULL;
  }
  r[str_length] = '\0';
  return r;
}

#define PARSE_STRING(name)                                                     \
  int _err;                                                                    \
  struct sv name = parse_string(buffer, &_err);                                \
  if (_err) {                                                                  \
    send_error(reply, id, SSH_FX_BAD_MESSAGE);                                 \
    return 1;                                                                  \
  }

int fxp_init(struct sb *reply, struct sv *buffer) {
  uint32_t version;
  TRY(TAKE(&version, sizeof(version)));
  client_version = version;
  client_has_sent_init = 1;

  uint32_t e_version = htonl(server_version);
  add_type(reply, SSH_FXP_VERSION);
  sb_append_buffer(reply, &e_version, sizeof(e_version));
  return 1;
}

void add_sv(struct sb *s, struct sv str) {
  uint32_t net_len = htonl(sv_length(str));
  sb_append_buffer(s, &net_len, sizeof(uint32_t));
  sb_append_sv(s, str);
}

void add_string(struct sb *s, const char *str) {
  const uint32_t len = strlen(str);
  uint32_t net_len = htonl(len);
  sb_append_buffer(s, &net_len, sizeof(uint32_t));
  sb_append_buffer(s, str, len);
}

void gen_attrib(struct sb *s, struct stat sb) {
  uint32_t server_flags = SSH_FILEXFER_ATTR_SIZE | SSH_FILEXFER_ATTR_UIDGID |
                          SSH_FILEXFER_ATTR_PERMISSIONS |
                          SSH_FILEXFER_ATTR_ACMODTIME;
  server_flags = htonl(server_flags);
  sb_append_buffer(s, &server_flags, sizeof(server_flags));
  server_flags = ntohl(server_flags);

  if (SSH_FILEXFER_ATTR_SIZE & server_flags) {
    uint64_t size;
    size = htonll(sb.st_size);
    sb_append_buffer(s, &size, sizeof(size));
  }
  if (SSH_FILEXFER_ATTR_UIDGID & server_flags) {
    uint32_t uid;
    uid = htonl(sb.st_uid);
    sb_append_buffer(s, &uid, sizeof(uid));
    uint32_t gid;
    gid = htonl(sb.st_gid);
    sb_append_buffer(s, &gid, sizeof(gid));
  }
  if (SSH_FILEXFER_ATTR_PERMISSIONS & server_flags) {
    uint32_t permissions;
    permissions = htonl(sb.st_mode);
    sb_append_buffer(s, &permissions, sizeof(permissions));
  }
  if (SSH_FILEXFER_ATTR_ACMODTIME & server_flags) {
    uint32_t atime;
    atime = htonl(sb.st_atime);
    sb_append_buffer(s, &atime, sizeof(atime));
    uint32_t mtime;
    mtime = htonl(sb.st_mtime);
    sb_append_buffer(s, &mtime, sizeof(mtime));
  }
}

int gen_send_error(struct sb *response, uint32_t id, uint32_t status,
                   struct sv error_message, struct sv language_tag) {
  add_type(response, SSH_FXP_STATUS);
  add_lu32(response, id);

  status = htonl(status);
  sb_append_buffer(response, &status, sizeof(status));
  add_sv(response, error_message);
  add_sv(response, language_tag);
  return 1;
}

#define GET_ID                                                                 \
  uint32_t network_id;                                                         \
  uint32_t id;                                                                 \
  TRY(TAKE(&network_id, sizeof(network_id)));                                  \
  id = ntohl(network_id);

int fxp_unsupported(struct sb *reply, struct sv *buffer) {
  GET_ID
  gen_send_error(reply, id, SSH_FX_OP_UNSUPPORTED, C_TO_SV("UNSUPPORTED"),
                 C_TO_SV("en"));
  return 1;
}

int fxp_close(struct sb *reply, struct sv *buffer) {
  GET_ID

  PARSE_STRING(handle);
  handle_close(handle);

  send_error(reply, id, SSH_FX_OK);
  return 1;
}

int fxp_readdir(struct sb *reply, struct sv *buffer) {
  GET_ID

  PARSE_STRING(handle);

  int fd;
  char *pathname;
  if (!handle_get_fd(handle, &fd, &pathname)) {
    send_error(reply, id, SSH_FX_NO_SUCH_FILE);
    return 1;
  }

  struct stat sb;
  if (-1 == fstat(fd, &sb)) {
    send_error(reply, id, SSH_FX_FAILURE);
    return 1;
  }

  if (!S_ISDIR(sb.st_mode)) {
    send_error(reply, id, SSH_FX_FAILURE);
    return 1;
  }

#ifndef HAS_FDCLOSEDIR
  // closedir() call will close the file descriptor so duplication is
  // required.
  fd = dup(fd);
  if (-1 == fd) {
    send_error(reply, id, SSH_FX_FAILURE);
    return 1;
  }
#endif // HAS_FDCLOSEDIR

  DIR *dir = fdopendir(fd);
  if (!dir) {
    send_error(reply, id, SSH_FX_FAILURE);
    return 1;
  }

  struct dirent *entry = readdir(dir);
  if (!entry) {
    send_error(reply, id, SSH_FX_EOF);
    return 1;
  }

  add_type(reply, SSH_FXP_NAME);
  add_lu32(reply, id);

  uint32_t count = 0;

  size_t offset;
  sb_reserve_buffer(reply, &offset, sizeof(count));

  for (; entry; entry = readdir(dir)) {
    add_string(reply, entry->d_name);
    char tmp_thing[strlen(pathname) + 1 + strlen(entry->d_name) + 1];
    strcpy(tmp_thing, pathname);
    strcat(tmp_thing, "/");
    strcat(tmp_thing, entry->d_name);
    char resolved[PATH_MAX];
    if (realpath(tmp_thing, resolved) != resolved) {
#ifdef HAS_FDCLOSEDIR
      fdclosedir(dir);
#else // HAS_FDCLOSEDIR
      closedir(dir);
#endif
      return 1;
    }
    add_string(reply, resolved);

    struct stat tmp;
    if (-1 == stat(resolved, &tmp)) {
#ifdef HAS_FDCLOSEDIR
      fdclosedir(dir);
#else // HAS_FDCLOSEDIR
      closedir(dir);
#endif
      send_error(reply, id, SSH_FX_FAILURE);
      return 1;
    }
    gen_attrib(reply, tmp);
    count++;

    if (sv_length(SB_TO_SV(*reply)) >
        MAX_PACKET_SIZE - 2048 /* random large number */) {
      break;
    }
  }

  count = htonl(count);
  sb_modify_location(reply, &count, sizeof(count), offset);
#ifdef HAS_FDCLOSEDIR
  fdclosedir(dir);
#else // HAS_FDCLOSEDIR
  closedir(dir);
#endif

  return 1;
}

int send_error_from_errno(struct sb *response, uint32_t id) {
  int type;
  switch (errno) {
  case ENOENT:
    type = SSH_FX_NO_SUCH_FILE;
    break;
  default:
    type = SSH_FX_FAILURE;
    break;
  };
  return gen_send_error(response, id, type, C_TO_SV(strerror(errno)),
                        C_TO_SV("en"));
}

int send_error(struct sb *reply, uint32_t id, int type) {
  struct sv error_msg;
  switch (type) {
  case SSH_FX_OK:
    error_msg = C_TO_SV("OK");
    break;
  case SSH_FX_BAD_MESSAGE:
    error_msg = C_TO_SV("Bad message.");
    break;
  default:
    error_msg = C_TO_SV("No error message for this type.");
    break;
  }
  return gen_send_error(reply, id, type, error_msg, C_TO_SV("en"));
}

int fxp_write(struct sb *reply, struct sv *buffer) {
  GET_ID

  PARSE_STRING(key);
  int fd;
  if (!handle_get_fd(key, &fd, NULL)) {
    send_error(reply, id, SSH_FX_NO_SUCH_FILE);
    return 1;
  }

  uint64_t offset;
  TRY_STATUS(TAKE(&offset, sizeof(offset)));
  offset = ntohll(offset);

  uint32_t len;
  TRY_STATUS(TAKE(&len, sizeof(len)));
  len = ntohl(len);

  if (len != sv_length(*buffer)) {
    send_error(reply, id, SSH_FX_BAD_MESSAGE);
    return 1;
  }

  int rc = write(fd, sv_buffer(*buffer), sv_length(*buffer));
  if (-1 == rc) {
    send_error_from_errno(reply, id);
    return 1;
  }
  send_error(reply, id, SSH_FX_OK);
  return 1;
}

int fxp_read(struct sb *reply, struct sv *buffer) {
  GET_ID

  PARSE_STRING(key);
  int fd;
  if (!handle_get_fd(key, &fd, NULL)) {
    send_error(reply, id, SSH_FX_NO_SUCH_FILE);
    return 1;
  }

  uint64_t offset;
  TRY_STATUS(TAKE(&offset, sizeof(offset)));
  offset = ntohll(offset);
  uint32_t len;
  TRY_STATUS(TAKE(&len, sizeof(len)));
  len = ntohl(len);
  len = min(len, MAX_READ_LEN);

  char *tmp = malloc(len);
  if (!tmp) {
    send_error(reply, id, SSH_FX_FAILURE);
    return 1;
  }

  int rc = pread(fd, tmp, len, offset);
  if (-1 == rc) {
    free(tmp);
    send_error_from_errno(reply, id);
    return 1;
  }
  if (0 == rc) {
    send_error(reply, id, SSH_FX_EOF);
    return 1;
  }

  add_type(reply, SSH_FXP_DATA);
  add_lu32(reply, id);

  uint32_t buffer_len = htonl(rc);
  sb_append_buffer(reply, &buffer_len, sizeof(uint32_t));
  sb_append_buffer(reply, tmp, rc);
  free(tmp);
  return 1;
}

int fxp_open(struct sb *reply, struct sv *buffer) {
  GET_ID

  PARSE_STRING(filename);
  uint32_t pflags;
  TRY_STATUS(TAKE(&pflags, sizeof(pflags)));
  pflags = ntohl(pflags);

  int read = (SSH_FXF_READ & pflags) ? 1 : 0;
  int write = (SSH_FXF_WRITE & pflags) ? 1 : 0;
  int append = (SSH_FXF_APPEND & pflags) ? 1 : 0;
  int create = (SSH_FXF_CREAT & pflags) ? 1 : 0;
  int truncate = (SSH_FXF_TRUNC & pflags) ? 1 : 0;
  int exclusive = (SSH_FXF_EXCL & pflags) ? 1 : 0;

  int f = 0;
  if (read || write) {
    f |= O_RDWR;
  }
  if (append) {
    f |= O_APPEND;
  }
  if (create) {
    f |= O_CREAT;
  }
  if (exclusive || truncate) {
    if (!create) {
      send_error(reply, id, SSH_FX_BAD_MESSAGE);
      return 1;
    }
  }
  if (truncate) {
    f |= O_TRUNC;
  }

  if (exclusive) {
    struct stat junk;
    if (-1 != sv_stat(filename, &junk)) {
      send_error(reply, id, SSH_FX_FAILURE);
      return 0;
    }
  }

  uint32_t attribute_flags;
  TRY_STATUS(TAKE(&attribute_flags, sizeof(attribute_flags)));
  attribute_flags = ntohl(attribute_flags);

  uint64_t size;
  uint32_t uid;
  uint32_t gid;
  uint32_t permissions;
  if (SSH_FILEXFER_ATTR_SIZE & attribute_flags) {
    TRY_STATUS(TAKE(&size, sizeof(uint64_t)));
    size = ntohll(size);
  }
  if (SSH_FILEXFER_ATTR_UIDGID & attribute_flags) {
    TRY_STATUS(TAKE(&uid, sizeof(uid)));
    TRY_STATUS(TAKE(&gid, sizeof(gid)));
    uid = ntohl(uid);
    gid = ntohl(gid);
  }
  if (SSH_FILEXFER_ATTR_PERMISSIONS & attribute_flags) {
    TRY_STATUS(TAKE(&permissions, sizeof(permissions)));
    permissions = ntohl(permissions);
  }
  // SSH_FILEXFER_ATTR_ACMODTIME ignored

  char *key = handle_create(filename, 0, f, permissions);
  if (!key) {
    send_error(reply, id, SSH_FX_FAILURE);
    return 0;
  }
  int fd;
  assert(handle_get_fd(C_TO_SV(key), &fd, NULL));

  if (SSH_FILEXFER_ATTR_SIZE & attribute_flags) {
    ftruncate(fd, size);
  }
  if (SSH_FILEXFER_ATTR_UIDGID & attribute_flags) {
    //    fchown(fd, uid, gid);
  }

  add_type(reply, SSH_FXP_HANDLE);
  add_lu32(reply, id);

  add_string(reply, key);

  return 1;
}

int fxp_opendir(struct sb *reply, struct sv *buffer) {
  GET_ID

  PARSE_STRING(path);

  char *key = handle_create(path, 1, -1, 0);
  if (!key) {
    // FIXME: Error is not completely correct
    send_error(reply, id, SSH_FX_NO_SUCH_FILE);
    return 1;
  }

  add_type(reply, SSH_FXP_HANDLE);
  add_lu32(reply, id);

  add_string(reply, key);
  return 1;
}

int fxp_realpath(struct sb *reply, struct sv *buffer) {
  GET_ID

  PARSE_STRING(path);
  char resolved_path[_POSIX_PATH_MAX];
  if (NULL == sv_realpath(path, resolved_path)) {
    dprintf(log_fd, "realpath: %s\n", strerror(errno));
    send_error_from_errno(reply, id);
    return 1;
  }

  struct stat sb;
  if (-1 == stat(resolved_path, &sb)) {
    dprintf(log_fd, "lstat: %s\n", strerror(errno));
    dprintf(log_fd, "no such directory: \"%s\"\n", resolved_path);
    send_error_from_errno(reply, id);
    return 1;
  }

  add_type(reply, SSH_FXP_NAME);
  add_lu32(reply, id);

  uint32_t count = htonl(1);
  sb_append_buffer(reply, &count, sizeof(count));

  add_sv(reply, path);
  add_string(reply, resolved_path);

  gen_attrib(reply, sb);

  return 1;
}

int fxp_stat(struct sb *reply, struct sv *buffer, int do_lstat) {
  GET_ID

  PARSE_STRING(path);

  struct stat sb;
  int rc = (do_lstat) ? sv_lstat(path, &sb) : sv_stat(path, &sb);
  if (-1 == rc) {
    dprintf(log_fd, "%s: %s\n", (do_lstat) ? "lstat" : "stat", strerror(errno));
    send_error_from_errno(reply, id);
    return 1;
  }

  add_type(reply, SSH_FXP_ATTRS);
  add_lu32(reply, id);

  gen_attrib(reply, sb);
  return 1;
}

int fxp_fstat(struct sb *reply, struct sv *buffer) {
  GET_ID

  PARSE_STRING(handle);

  int fd;
  if (!handle_get_fd(handle, &fd, NULL)) {
    send_error(reply, id, SSH_FX_NO_SUCH_FILE);
    return 1;
  }

  struct stat sb;
  if (-1 == fstat(fd, &sb)) {
    dprintf(log_fd, "fstat: %s\n", strerror(errno));
    send_error_from_errno(reply, id);
    return 1;
  }

  add_type(reply, SSH_FXP_ATTRS);
  add_lu32(reply, id);

  gen_attrib(reply, sb);
  return 1;
}

int read_payload(void) {
  uint32_t length;
  ERRNO_TRY(force_read(0, &length, sizeof(uint32_t)), "read");
  if (0 == length) {
    dprintf(log_fd, "???????\n");
    // ???
    return 1;
  }
  length = ntohl(length);

  char *payload = malloc(length);
  ERRNO_TRY(force_read(0, payload, length), "read");

  struct sv tmp = sv_init(payload, length);
  struct sv *buffer = &tmp;

  uint8_t type;
  assert(TAKE(&type, sizeof(type)));

  if (SSH_FXP_INIT != type && !client_has_sent_init) {
    dprintf(log_fd, "problem?\n");
    // TODO: Problem?
    return 0;
  }

  struct sb reply;
  sb_init(&reply);
  size_t length_offset;
  sb_reserve_buffer(&reply, &length_offset, sizeof(uint32_t));

  switch (type) {
  case SSH_FXP_INIT:
    TRY(fxp_init(&reply, buffer));
    break;
  case SSH_FXP_OPEN:
    TRY(fxp_open(&reply, buffer));
    break;
  case SSH_FXP_READ:
    TRY(fxp_read(&reply, buffer));
    break;
  case SSH_FXP_WRITE:
    TRY(fxp_write(&reply, buffer));
    break;
  case SSH_FXP_STAT:
    TRY(fxp_stat(&reply, buffer, 0));
    break;
  case SSH_FXP_LSTAT:
    TRY(fxp_stat(&reply, buffer, 1));
    break;
  case SSH_FXP_FSTAT:
    TRY(fxp_fstat(&reply, buffer));
    break;
  case SSH_FXP_REALPATH:
    TRY(fxp_realpath(&reply, buffer));
    break;
  case SSH_FXP_OPENDIR:
    TRY(fxp_opendir(&reply, buffer));
    break;
  case SSH_FXP_READDIR:
    TRY(fxp_readdir(&reply, buffer));
    break;
  case SSH_FXP_CLOSE:
    TRY(fxp_close(&reply, buffer));
    break;
  default:
    dprintf(log_fd, "unknown type: %d\n", type);
    TRY(fxp_unsupported(&reply, buffer));
    break;
  }

  uint32_t network_length =
      htonl(sv_length(SB_TO_SV(reply)) - sizeof(uint32_t));

  assert(sb_modify_location(&reply, &network_length, sizeof(uint32_t),
                            length_offset));

  struct sv v = SB_TO_SV(reply);
  uint32_t l = sv_length(v);
  write(1, sv_buffer(v), l);

  sb_free(&reply);
  free(payload);
  return 1;
}

int main(void) {
  log_fd = open("/log.txt", O_CREAT | O_RDWR | O_TRUNC);
  assert(-1 != log_fd);
  dprintf(log_fd, "start of log file\n");
  for (;;) {
    if (!read_payload()) {
      return 1;
    }
  }
  return 0;
}
