#include "lexer.h"
#include <assert.h>
#include <ctype.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>

void free_tokens(struct TOKEN *token) {
  for (; token;) {
    struct TOKEN *old = token;
    token = token->next;
    free(old);
  }
}

int is_special_char(char c) {
  if (!isprint(c)) {
    return 1;
  }
  if (isspace(c)) {
    return 1;
  }
  if (isalnum(c)) {
    return 0;
  }
  return !(('>' != c && '|' != c && '&' != c));
}

int parse_chars(struct sv *code_ptr, struct TOKEN *cur) {
  struct sv code = *code_ptr;
  if (is_special_char(sv_peek(code))) {
    return 0;
  }
  cur->type = TOKEN_CHARS;
  cur->string_rep = sv_split_function(code, &code, is_special_char);
  *code_ptr = code;
  return 1;
}

// Operands such as: &, &&, |, || etc
// Is operands the right word?
int parse_operand(struct sv *code_ptr, struct TOKEN *cur) {
  struct sv code = *code_ptr;
#define TRY_PARSE_STRING(_s, _token)                                           \
  if (sv_partial_eq(code, C_TO_SV(_s))) {                                      \
    cur->type = _token;                                                        \
    cur->string_rep = sv_take(code, &code, strlen(_s));                        \
    goto complete_return;                                                      \
  }
  TRY_PARSE_STRING("&&", TOKEN_AND);
  TRY_PARSE_STRING("||", TOKEN_NOT);
  TRY_PARSE_STRING(">>", TOKEN_STREAM_APPEND);
  TRY_PARSE_STRING(">", TOKEN_STREAM);
  TRY_PARSE_STRING("|", TOKEN_PIPE);
  TRY_PARSE_STRING("&", TOKEN_BACKGROUND);
  TRY_PARSE_STRING("\n", TOKEN_NEWLINE);

  // Failed to parse
  return 0;

complete_return:
  *code_ptr = code;
  return 1;
}

void skip_whitespace_and_comment(struct sv *s) {
  struct sv start;
  do {
    start = *s;
    *s = sv_skip_chars(*s, " \t\r");
    if (!sv_partial_eq(*s, C_TO_SV("#"))) {
      return;
    }
    (void)sv_split_delim(*s, s, '\n');
  } while (start.length != s->length);
}

struct TOKEN *lex(struct sv code) {
  struct TOKEN *head = NULL;
  struct TOKEN *prev = NULL;
  for (; !sv_isempty(code);) {
    skip_whitespace_and_comment(&code);
    if (sv_isempty(code)) {
      break;
    }

    struct TOKEN *cur = malloc(sizeof(struct TOKEN));
    cur->next = NULL;
    if (prev) {
      prev->next = cur;
    }
    if (parse_chars(&code, cur)) {
    } else if (parse_operand(&code, cur)) {
    } else {
      free(cur);
      assert(0 && "Unknown token");
    }
    if (!head) {
      head = cur;
    }
    prev = cur;
  }
  return head;
}
