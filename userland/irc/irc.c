#include "sb.h"
#include "sv.h"
#include <arpa/inet.h>
#include <assert.h>
#include <ctype.h>
#include <errno.h>
#include <fcntl.h>
#include <libgui.h>
#include <math.h>
#include <poll.h>
#include <pty.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/socket.h>
#include <syscall.h>
#include <unistd.h>

#define TERMINAL_WIDTH 82
#define TERMINAL_HEIGHT 40
GUI_Window *w;

struct message {
  char name[9];
  size_t name_len;
  char text[510];
  size_t text_len;
};

struct irc_channel {
  struct sv name;
  struct message *messages;
  size_t messages_num;
  size_t messages_cap;
  int can_send_message;
  struct irc_channel *prev;
  struct irc_channel *next;
};

struct irc_server {
  u32 socket;
  struct irc_channel *channels;
};
struct irc_channel *selected_channel = NULL;
void refresh_screen(void);

void irc_add_message(struct irc_server *server, struct sv channel_name,
                     struct sv sender, struct sv msg_text);
void irc_add_message_to_channel(struct irc_channel *chan, struct sv sender,
                                struct sv msg_text);

u32 gen_ipv4(u8 i1, u8 i2, u8 i3, u8 i4) {
  return i4 << (32 - 8) | i3 << (32 - 16) | i2 << (32 - 24) | i1 << (32 - 32);
}

struct event {
  u8 type; // File descriptor | Socket
  u32 internal_id;
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

int tcp_fmt(int socket, const char *fmt, ...) {
  va_list ap;
  char cmd_str[512];
  va_start(ap, fmt);
  int rc = vsnprintf(cmd_str, 512, fmt, ap);
  va_end(ap);
  return write(socket, cmd_str, rc);
}

int message_pos_x = 0;
int message_pos_y = 0;

void clear_area(int x, int y, int sx, int sy) {
  GUI_DrawRectangle(w, x, y, sx, sy, 0x0);
}

int prompt_x = 0;
int prompt_y = TERMINAL_HEIGHT - 1;

void msg_sv_print(struct sv s);
void msg_sv_println(struct sv s) {
  msg_sv_print(s);
  if (0 != message_pos_x && 0 < s.length) {
    message_pos_x = 0;
    message_pos_y++;
  }
}

void msg_sv_print(struct sv s) {
  for (;;) {
    struct sv line = sv_split_delim(s, &s, '\n');

    for (size_t i = 0; i < line.length; i++) {
      GUI_DrawFont(w, message_pos_x * 8, message_pos_y * 8, line.s[i]);
      message_pos_x++;
    }

    if (sv_isempty(s) && '\n' != line.s[line.length]) {
      break;
    }
    message_pos_x = 0;
    message_pos_y++;
    if ('\n' == line.s[line.length]) {
      break;
    }
  }
  GUI_UpdateWindow(w);
}

#define RPL_WELCOME C_TO_SV("001")
#define RPL_YOURHOST C_TO_SV("002")
#define RPL_CREATED C_TO_SV("003")
#define RPL_MYINFO C_TO_SV("004")
#define RPL_BOUNCE C_TO_SV("005")
#define RPL_LUSERCLIENT C_TO_SV("251")
#define RPL_NOTOPIC C_TO_SV("331")
#define RPL_TOPIC C_TO_SV("332")
#define RPL_NAMREPLY C_TO_SV("353")
#define RPL_ENDOFNAMES C_TO_SV("366")

#define ERR_NOMOTD C_TO_SV("422")
#define RPL_MOTDSTART C_TO_SV("375")
#define RPL_MOTD C_TO_SV("372")
#define RPL_ENDOFMOTD C_TO_SV("376")

struct sv server_nick = C_TO_SV("testuser");

void handle_msg(struct irc_server *server, struct sv msg) {
  struct sv message_origin = C_TO_SV("");
  int has_prefix = (':' == sv_peek(msg));
  if (has_prefix) {
    message_origin = sv_split_delim(msg, &msg, ' ');
  }

  struct sv command = sv_split_delim(msg, &msg, ' ');
  struct sv command_parameters = msg;

#define START_HANDLE_CMD                                                       \
  if (0) {                                                                     \
  }
#define HANDLE_CMD(_cmd) else if (sv_eq(_cmd, command))
#define PASSTHROUGH_CMD(_cmd)                                                  \
  HANDLE_CMD(_cmd) {                                                           \
    struct sv intended_recipient =                                             \
        sv_split_delim(command_parameters, &command_parameters, ' ');          \
    if (sv_eq(intended_recipient, server_nick)) {                              \
      irc_add_message_to_channel(&server->channels[0], C_TO_SV("*"),           \
                                 command_parameters);                          \
    }                                                                          \
  }
#define PASSTHROUGH_REPLY_CMD(_cmd)                                            \
  HANDLE_CMD(_cmd) {                                                           \
    struct sv intended_recipient =                                             \
        sv_split_delim(command_parameters, &command_parameters, ' ');          \
    /* Remove the ':' */                                                       \
    command_parameters = sv_trim_left(command_parameters, 1);                  \
    if (sv_eq(intended_recipient, server_nick)) {                              \
      irc_add_message_to_channel(&server->channels[0], C_TO_SV("*"),           \
                                 command_parameters);                          \
    }                                                                          \
  }
#define PASSTHROUGH_CHANNEL_CMD(_cmd)                                          \
  HANDLE_CMD(_cmd) {                                                           \
    struct sv intended_recipient =                                             \
        sv_split_delim(command_parameters, &command_parameters, ' ');          \
    struct sv channel =                                                        \
        sv_split_delim(command_parameters, &command_parameters, ' ');          \
    /* Remove the ':' */                                                       \
    command_parameters = sv_trim_left(command_parameters, 1);                  \
    if (sv_eq(intended_recipient, server_nick)) {                              \
      irc_add_message(server, channel, C_TO_SV("*"), command_parameters);      \
    }                                                                          \
  }
#define END_HANDLE_CMD                                                         \
  else {                                                                       \
    msg_sv_print(C_TO_SV("Unknown command recieved from server: "));           \
    msg_sv_print(command);                                                     \
    msg_sv_print(C_TO_SV("\n"));                                               \
  }

  START_HANDLE_CMD
  HANDLE_CMD(C_TO_SV("JOIN")) {
    struct sv channel = sv_split_space(command_parameters, NULL);

    struct sb join_message;
    sb_init(&join_message);
    sb_append_sv(&join_message, message_origin);
    sb_append(&join_message, " has joined the channel.");
    irc_add_message(server, channel, C_TO_SV("*"), SB_TO_SV(join_message));
    sb_free(&join_message);
  }
  HANDLE_CMD(C_TO_SV("PING")) {
    write(server->socket, "PONG", 4);
  }
  HANDLE_CMD(C_TO_SV("PRIVMSG")) {
    struct sv channel = sv_split_delim(command_parameters, &msg, ' ');
    struct sv nick = sv_trim_left(message_origin, 1);
    nick = sv_split_delim(nick, NULL, '!');
    /* Remove the ':' */
    msg = sv_trim_left(msg, 1);
    irc_add_message(server, channel, nick, msg);
  }
  HANDLE_CMD(RPL_NAMREPLY) {
    struct sv intended_recipient =
        sv_split_delim(command_parameters, &command_parameters, ' ');
    struct sv channel_status =
        sv_split_delim(command_parameters, &command_parameters, ' ');
    (void)channel_status;
    struct sv channel =
        sv_split_delim(command_parameters, &command_parameters, ' ');
    // Remove the ':'
    command_parameters = sv_trim_left(command_parameters, 1);
    if (sv_eq(intended_recipient, server_nick)) {
      struct sb user_list_message;
      sb_init(&user_list_message);
      sb_append(&user_list_message, "Users on ");
      sb_append_sv(&user_list_message, channel);
      sb_append(&user_list_message, ": ");
      sb_append_sv(&user_list_message, command_parameters);
      irc_add_message(server, channel, C_TO_SV("*"),
                      SB_TO_SV(user_list_message));
      sb_free(&user_list_message);
    }
  }
  PASSTHROUGH_CHANNEL_CMD(RPL_ENDOFNAMES)

  PASSTHROUGH_CHANNEL_CMD(RPL_NOTOPIC)
  PASSTHROUGH_CHANNEL_CMD(RPL_TOPIC)

  PASSTHROUGH_REPLY_CMD(RPL_MOTDSTART)
  PASSTHROUGH_REPLY_CMD(RPL_MOTD)
  PASSTHROUGH_REPLY_CMD(RPL_ENDOFMOTD)

  PASSTHROUGH_REPLY_CMD(RPL_WELCOME)
  PASSTHROUGH_REPLY_CMD(RPL_YOURHOST)
  PASSTHROUGH_REPLY_CMD(RPL_CREATED)
  PASSTHROUGH_REPLY_CMD(RPL_MYINFO)
  PASSTHROUGH_REPLY_CMD(RPL_BOUNCE)
  PASSTHROUGH_REPLY_CMD(RPL_LUSERCLIENT)
  PASSTHROUGH_REPLY_CMD(ERR_NOMOTD)
  END_HANDLE_CMD
}

void send_message(struct irc_server *server, struct irc_channel *chan,
                  struct sv msg) {
  if (!chan->can_send_message) {
    return;
  }
  struct sb s;
  sb_init(&s);
  sb_append(&s, "PRIVMSG ");
  sb_append_sv(&s, chan->name);
  sb_append(&s, " :");
  sb_append_sv(&s, msg);
  sb_append(&s, "\n");

  if (msg.length > 512) {
    // TODO I am too lazy to add functionality to split msg right now
    assert(0);
  }
  write(server->socket, s.string, s.length);
  irc_add_message(server, chan->name, server_nick, msg);
  sb_free(&s);
}

void irc_add_channel(struct irc_server *server, struct sv name) {
  struct irc_channel *prev = server->channels;
  struct irc_channel *chan;
  if (!server->channels) {
    server->channels = malloc(sizeof(struct irc_channel));
    chan = server->channels;
  } else {
    struct irc_channel *t = server->channels;
    for (; t->next;) {
      t = t->next;
    }
    prev = t;
    t->next = malloc(sizeof(struct irc_channel));
    chan = t->next;
  }

  chan->name = sv_clone(name);
  chan->messages = malloc(256 * sizeof(struct message));
  chan->messages_cap = 256;
  chan->messages_num = 0;
  chan->prev = prev;
  chan->next = NULL;
  chan->can_send_message = 1;
}

void irc_join_channel(struct irc_server *server, const char *name) {
  assert(tcp_fmt(server->socket, "JOIN %s\n", name));
  irc_add_channel(server, C_TO_SV(name));
}

void irc_show_channel(struct irc_channel *channel) {
  clear_area(0, 0, TERMINAL_WIDTH * 8, (prompt_y - 1) * 8);
  message_pos_x = 0;
  message_pos_y = 0;

  char buffer[4096];
  sprintf(buffer, "Number of messages in channe: %d",
          (int)channel->messages_num);
  struct sb b;
  sb_init(&b);
  sb_append(&b, buffer);
  msg_sv_println(SB_TO_SV(b));
  sb_free(&b);

  for (size_t i = 0; i < channel->messages_num; i++) {
    struct message *m = &channel->messages[i];
    struct sv nick = {
        .s = m->name,
        .length = m->name_len,
    };
    struct sv msg = {
        .s = m->text,
        .length = m->text_len,
    };
    msg_sv_print(nick);
    msg_sv_print(C_TO_SV(": "));
    msg_sv_println(msg);
  }
}

void irc_add_message_to_channel(struct irc_channel *chan, struct sv sender,
                                struct sv msg_text) {
  if (chan->messages_num >= chan->messages_cap - 1) {
    chan->messages_cap += 256;
    chan->messages =
        realloc(chan->messages, chan->messages_cap * sizeof(struct message));
  }
  struct message *msg = &chan->messages[chan->messages_num];
  chan->messages_num++;

  assert(sender.length <= 9);
  assert(msg_text.length <= 510);

  msg->name_len = sender.length;
  msg->text_len = msg_text.length;
  memcpy(msg->name, sender.s, sender.length);
  memcpy(msg->text, msg_text.s, msg_text.length);
  if (selected_channel == chan) {
    refresh_screen();
  }
}

struct irc_channel *irc_get_channel(struct irc_server *server,
                                    struct sv channel_name) {
  struct irc_channel *chan = server->channels;
  for (; chan; chan = chan->next) {
    if (sv_eq(chan->name, channel_name)) {
      break;
    }
  }
  if (!chan) {
    // If a message was recieved but we have not joined the channel it
    // is probably a PRIVMSG that this application turns into a channel
    // message where the "channel_name" is the sender. Or the server is
    // confused and thinks we already have joined a channel. Either way
    // it should be added to the list of joined channels.
    irc_add_channel(server, channel_name);
    // Recursion as the function should now no longer fail
    return irc_get_channel(server, channel_name);
  }
  return chan;
}

void irc_add_message(struct irc_server *server, struct sv channel_name,
                     struct sv sender, struct sv msg_text) {
  struct irc_channel *chan = irc_get_channel(server, channel_name);
  irc_add_message_to_channel(chan, sender, msg_text);
}

int irc_connect_server(struct irc_server *server, u32 ip, u16 port) {
  (void)ip;
  struct sockaddr_in servaddr;
  int fd = socket(AF_INET, SOCK_STREAM, 0);
  if (fd < 0) {
    perror("socket");
    return 0;
  }

  memset(&servaddr, 0, sizeof(servaddr));
  servaddr.sin_family = AF_INET;
  servaddr.sin_addr.a[0] = 10;
  servaddr.sin_addr.a[1] = 0;
  servaddr.sin_addr.a[2] = 2;
  servaddr.sin_addr.a[3] = 2;
  servaddr.sin_port = htons(port);

  if (connect(fd, (struct sockaddr *)&servaddr, sizeof(servaddr)) < 0) {
    perror("connect");
    return 0;
  }

  if (!set_fd_nonblocking(fd)) {
    return 0;
  }

  int flag = 1;
  assert(0 ==
         setsockopt(fd, IPPROTO_TCP, TCP_NODELAY, (char *)&flag, sizeof(int)));

  server->socket = fd;
  irc_add_channel(server, C_TO_SV("*"));
  server->channels[0].can_send_message = 0;
  return 1;
}

void refresh_screen(void) {
  irc_show_channel(selected_channel);
}

void ws_handle_events(struct irc_server *ctx, struct sb *your_msg) {
  WS_EVENT ev;
  int rc = read(w->ws_socket, &ev, sizeof(ev));
  if (rc <= 0) {
    return;
  }
  if (WINDOWSERVER_EVENT_KEYPRESS != ev.type) {
    return;
  }
  const struct KEY_EVENT key = ev.ev;
  if (key.release) {
    return;
  }
  if ('n' == key.c && EV_CTRL(key.mode)) {
    struct irc_channel *next_channel = selected_channel->next;
    if (next_channel) {
      selected_channel = next_channel;
      refresh_screen();
    }
    return;
  }
  if ('p' == key.c && EV_CTRL(key.mode)) {
    struct irc_channel *prev_channel = selected_channel->prev;
    if (prev_channel) {
      selected_channel = prev_channel;
      refresh_screen();
    }
    return;
  }

  if ('\b' == key.c) {
    int n = sb_delete_right(your_msg, 1);
    if (0 == n) {
      return;
    }
    prompt_x -= n;
    GUI_OverwriteFont(w, prompt_x * 8, prompt_y * 8, 0x0);
    GUI_UpdateWindow(w);
    return;
  }

  if ('\n' == key.c) {
    send_message(ctx, selected_channel, SB_TO_SV(*your_msg));
    prompt_x = 0;
    clear_area(0, prompt_y * 8, 50 * 8, 1 * 8);
    GUI_UpdateWindow(w);
    sb_reset(your_msg);
    return;
  }

  GUI_DrawFont(w, prompt_x * 8, prompt_y * 8, key.c);
  GUI_UpdateWindow(w);
  prompt_x++;

  sb_append_char(your_msg, key.c);
}

int main(void) {
  w = GUI_CreateWindow(0, 0, TERMINAL_WIDTH * 8, TERMINAL_HEIGHT * 8 * 2);
  if (!w) {
    return 1;
  }
  struct pollfd fds[2];

  fds[0].fd = w->ws_socket;
  fds[0].events = POLLIN;
  fds[0].revents = 0;

  u32 ip = gen_ipv4(10, 0, 2, 2);
  struct irc_server server_ctx;
  if (!irc_connect_server(&server_ctx, ip, 6667)) {
    printf("Failed to connect to the irc server\n");
    return 1;
  }

  selected_channel = &server_ctx.channels[0];

  const char *nick = SV_TO_C(server_nick);
  const char *realname = "John Doe";
  assert(tcp_fmt(server_ctx.socket, "NICK %s\n", nick));
  assert(tcp_fmt(server_ctx.socket, "USER %s * * :%s\n", nick, realname));
  irc_join_channel(&server_ctx, "#secretclub");

  fds[1].fd = server_ctx.socket;
  fds[1].events = POLLIN;
  fds[1].revents = 0;

  struct sb your_msg;
  sb_init(&your_msg);

  char current_msg[512];
  u32 msg_usage = 0;

  assert(set_fd_nonblocking(server_ctx.socket));

  for (;;) {
    poll(fds, 2, 0);
    ws_handle_events(&server_ctx, &your_msg);
    char buffer[4096];
    int rc = read(server_ctx.socket, buffer, 4096);
    if (rc < 0) {
      continue;
    }

    for (int i = 0; i < rc; i++) {
      assert(msg_usage < 512);
      if ('\n' == buffer[i] && i > 1 && '\r' == buffer[i - 1]) {
        handle_msg(&server_ctx, (struct sv){
                                    .s = current_msg,
                                    .length = msg_usage,
                                });
        refresh_screen();
        msg_usage = 0;
        continue;
      }
      current_msg[msg_usage] = buffer[i];
      msg_usage++;
    }
  }
  return 0;
}
