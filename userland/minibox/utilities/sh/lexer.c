#include <assert.h>
#include <ctype.h>
#include "lexer.h"
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

int is_nonspecial_char(char c) {
  if (!isprint(c))
    return 0;
  if (isspace(c))
    return 0;
  if (isalnum(c))
    return 1;
  return ('>' != c && '|' != c && '&' != c);
}

int parse_chars(const char **code_ptr, struct TOKEN *cur) {
  const char *code = *code_ptr;
  if (!is_nonspecial_char(*code))
    return 0;
  cur->type = TOKEN_CHARS;
  int i = 0;
  for (; *code; code++, i++) {
    if (!is_nonspecial_char(*code)) {
      break;
    }
    assert(i < 256);
    cur->string_rep[i] = *code;
  }
  cur->string_rep[i] = '\0';
  *code_ptr = code;
  return 1;
}

// Operands such as: &, &&, |, || etc
// Is operands the right word?
int parse_operand(const char **code_ptr, struct TOKEN *cur) {
  const char *code = *code_ptr;
#define TRY_PARSE_STRING(_s, _token)                                           \
  if (0 == strncmp(code, _s, strlen(_s))) {                                    \
    cur->type = _token;                                                        \
    strcpy(cur->string_rep, _s);                                               \
    code += strlen(_s);                                                        \
    goto complete_return;                                                      \
  }
  TRY_PARSE_STRING("&&", TOKEN_AND);
  TRY_PARSE_STRING("||", TOKEN_NOT);
  TRY_PARSE_STRING(">>", TOKEN_STREAM_APPEND);
  TRY_PARSE_STRING(">", TOKEN_STREAM);
  TRY_PARSE_STRING("|", TOKEN_PIPE);
  // TODO: &

  // Failed to parse
  return 0;

complete_return:
  *code_ptr = code;
  return 1;
}

void skip_whitespace(const char **code_ptr) {
  const char *code = *code_ptr;
  for (; isspace(*code); code++)
    ;
  *code_ptr = code;
}

struct TOKEN *lex(const char *code) {
  struct TOKEN *head = NULL;
  struct TOKEN *prev = NULL;
  for (; *code;) {
    skip_whitespace(&code);
    if (!*code)
      break;
    struct TOKEN *cur = malloc(sizeof(struct TOKEN));
    cur->next = NULL;
    if (prev)
      prev->next = cur;
    if (parse_chars(&code, cur)) {
    } else if (parse_operand(&code, cur)) {
    } else {
      free(cur);
      printf("at: %s\n", code);
      assert(0 && "Unknown token");
    }
    if (!head)
      head = cur;
    prev = cur;
  }
  return head;
}
