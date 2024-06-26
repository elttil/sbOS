#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <tb/sv.h>
#include <unistd.h>

#define PORT 53

u16 add_domain_label(u8 *buffer, struct sv label) {
  assert(label.length < 64);
  u16 size = 0;
  u8 ll = label.length;
  memcpy(buffer + size, &ll, sizeof(u8));
  size += sizeof(u8);
  memcpy(buffer + size, label.s, label.length);
  size += label.length;
  return size;
}

u16 send_domain_request(int sockfd, const char *domain, u16 *id) {
  *id = 0;
  u8 message[512];
  memset(message, 0, sizeof(message));

  u16 size = 0;
  size += 6 * sizeof(u16); // header

  u16 flags = htons(0x0100);
  u16 qdcount = htons(1);
  memcpy(message, id, sizeof(u16));
  memcpy(message + sizeof(u16), &flags, sizeof(u16));
  memcpy(message + sizeof(u16) * 2, &qdcount, sizeof(u16));

  struct sv labels = C_TO_SV(domain);
  for (; !sv_isempty(labels);) {
    struct sv label = sv_split_delim(labels, &labels, '.');
    size += add_domain_label(message + size, label);
  }

  size += add_domain_label(message + size, C_TO_SV(""));

  u16 query_type = htons(1); // A type
  memcpy(message + size, &query_type, sizeof(u16));
  size += sizeof(u16);

  u16 q_class = htons(1); // IN
  memcpy(message + size, &q_class, sizeof(u16));
  size += sizeof(u16);

  printf("sending: %d\n", size);

  write(sockfd, (char *)message, size);
  return size;
}

int connect_dns(void) {
  int sockfd;
  struct sockaddr_in servaddr;

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_addr.a[0] = 8;
  servaddr.sin_addr.a[1] = 8;
  servaddr.sin_addr.a[2] = 8;
  servaddr.sin_addr.a[3] = 8;
  servaddr.sin_port = htons(PORT);
  servaddr.sin_family = AF_INET;

  sockfd = socket(AF_INET, SOCK_DGRAM, 0);

  if (connect(sockfd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    return -1;
  }
  return sockfd;
}

/*
  hints.ai_family = AF_UNSPEC;
  hints.ai_socktype = type;
  hints.ai_flags = AI_PASSIVE;
  hints.ai_protocol = 0;
  hints.ai_canonname = NULL;
  hints.ai_addr = NULL;
  hints.ai_next = NULL;
*/

#define EAI_AGAIN 1
#define EAI_FAIL 2
#define EAI_NONAME 3
#define EAI_MEMORY 4

u16 service_to_port(const char *service) {
  int is_number = 0;
  for (int i = 0; service[i]; i++) {
    if (isdigit(service[i])) {
      is_number = 1;
    }
  }
  if (is_number) {
    return htons(atoi(service));
  }

#define SERVICE(_name, _port)                                                  \
  if (0 == strcmp(_name, service)) {                                           \
    return htons(_port);                                                       \
  }
  SERVICE("http", 80)
  SERVICE("https", 443)
#undef SERVICE
  return 0;
}

int getaddrinfo(const char *restrict node, const char *restrict service,
                const struct addrinfo *restrict hints,
                struct addrinfo **restrict res) {
  int sockfd = connect_dns();
  if (-1 == sockfd) {
    return EAI_AGAIN;
  }

  // TODO: Use the hints to make decisions about what data to request.
  (void)hints;

  u16 id;
  u16 size = send_domain_request(sockfd, node, &id);

  u8 buffer[512];
  memset(buffer, 0, sizeof(buffer));
  int rc = read(sockfd, buffer, sizeof(buffer));
  if (rc < size) {
    close(sockfd);
    return EAI_FAIL;
  }
  close(sockfd);

  if (0 != memcmp(buffer, &id, sizeof(u16))) {
    close(sockfd);
    return EAI_FAIL;
  }

  u16 flags;
  memcpy(&flags, buffer + sizeof(u16), sizeof(u16));
  flags = ntohs(flags);

  if (!(flags & (1 << 15))) { // QR
    close(sockfd);
    return EAI_FAIL;
  }
  if (0 != (flags & 0x1F)) { // Error field
    close(sockfd);
    return EAI_AGAIN;
  }

  u16 qcount = ntohs(*(u16 *)(buffer + sizeof(u16) * 2));
  if (1 != qcount) {
    close(sockfd);
    return EAI_FAIL;
  }

  u16 ancount = ntohs(*(u16 *)(buffer + sizeof(u16) * 3));
  if (0 == ancount) {
    close(sockfd);
    return EAI_NONAME; // TODO: Check if this is correct
  }

  u8 *answer = buffer + size;
  u16 answer_length = rc - size;
  if (0 == answer_length) {
    close(sockfd);
    return EAI_FAIL;
  }

  /*
  // get labels
  {
    u16 offset = ntohs(*(u16 *)answer);
    assert(3 == offset >> 14);
    offset &= ~(3 << 14);

    printf("offset: %d\n", offset);
    printf("name: ");
    u8 length = buffer[offset];
    for (int i = 0; i < length; i++) {
      printf("%c", buffer[offset + 1 + i]);
    }
    printf("\n");
  }
  */

  for (u16 i = 0; i < answer_length;) {
    // type
    u16 type = ntohs(*(u16 *)(answer + sizeof(u16)));
    u16 class = ntohs(*(u16 *)(answer + sizeof(u16) * 2));
    int ignore = 0;
    u16 inc = 0;
    if (1 != type) {
      ignore = 1;
    }
    if (1 != class) {
      ignore = 1;
    }
    inc += sizeof(u16) * 3;

    u16 rdlength = ntohs(*(u16 *)(answer + sizeof(u16) * 3 + sizeof(u32)));
    inc += sizeof(u16);
    inc += rdlength;
    inc += sizeof(u32);

    if (4 != rdlength) {
      ignore = 1;
    }
    if (!ignore) {
      u32 ip = *(u32 *)(answer + sizeof(u16) * 4 + sizeof(u32));

      if (res) {
        *res = calloc(1, sizeof(struct addrinfo));
        if (!(*res)) {
          close(sockfd);
          return EAI_MEMORY;
        }
        struct sockaddr_in *sa = calloc(1, sizeof(struct sockaddr_in));
        if (!sa) {
          free(*res);
          close(sockfd);
          return EAI_MEMORY;
        }
        (*res)->ai_addr = (struct sockaddr *)sa;
        sa->sin_addr.s_addr = ip;
        sa->sin_port = service_to_port(service);
        sa->sin_family = AF_INET;
      }
    }
    i += inc;
    answer += inc;
  }
  return 0;
}

void freeaddrinfo(struct addrinfo *res) {
  free(res->ai_addr);
  free(res);
}
