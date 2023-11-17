#ifndef LEXER_H
#define LEXER_H
#include <stddef.h>

typedef enum {
  TOKEN_CHARS,
  TOKEN_AND,
  TOKEN_NOT,
  TOKEN_NOOP,
  TOKEN_PIPE,
  TOKEN_STREAM,
  TOKEN_STREAM_APPEND,
} token_type_t;

struct TOKEN {
  token_type_t type;
  char string_rep[256];
  struct TOKEN *next;
};

struct TOKEN *lex(const char *code);
struct AST *generate_ast(struct TOKEN *token);
void free_tokens(struct TOKEN *token);
#endif // LEXER_H
