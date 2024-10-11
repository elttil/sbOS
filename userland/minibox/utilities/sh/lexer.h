#ifndef LEXER_H
#define LEXER_H
#include <stddef.h>
#include <tb/sv.h>

typedef enum {
  TOKEN_CHARS,
  TOKEN_AND,
  TOKEN_NOT,
  TOKEN_NOOP,
  TOKEN_PIPE,
  TOKEN_STREAM,
  TOKEN_STREAM_APPEND,
  TOKEN_BACKGROUND,
  TOKEN_NEWLINE,
} token_type_t;

struct TOKEN {
  token_type_t type;
  struct sv string_rep;
  struct TOKEN *next;
};

struct TOKEN *lex(struct sv code);
struct AST *generate_ast(struct TOKEN *token);
void free_tokens(struct TOKEN *token);
#endif // LEXER_H
