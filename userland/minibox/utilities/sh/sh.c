#include "ast.h"
#include "lexer.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int execute_command(struct AST *ast, int input_fd);

int execute_binary(struct AST *ast, int input_fd) {
  char *program = ast->val.string;
  struct AST *child = ast->children;
  char *argv[100];
  argv[0] = program;
  int i = 1;
  for (; child; i++, child = child->next) {
    argv[i] = child->val.string;
  }
  argv[i] = NULL;

  int in = input_fd;
  int out = STDOUT_FILENO;
  int slave_input = -1;

  int file_out_fd;
  if (ast->file_out) {
    file_out_fd =
        open(ast->file_out,
             O_WRONLY | O_CREAT | ((ast->file_out_append) ? O_APPEND : O_TRUNC),
             0666);
  }

  if (ast->pipe_rhs) {
    int fds[2];
    pipe(fds);
    out = fds[1];
    slave_input = fds[0];
  }

  int pid = fork();
  if (0 == pid) {
    if (slave_input >= 0)
      close(slave_input);
    dup2(in, STDIN_FILENO);
    dup2(out, STDOUT_FILENO);
    if (ast->file_out)
      dup2(file_out_fd, ast->file_out_fd_to_use);

    execvp(program, argv);
    exit(1);
  }
  if (ast->file_out)
    close(file_out_fd);

  if (ast->pipe_rhs) {
    if (out >= 0)
      close(out);
    return execute_command(ast->pipe_rhs, slave_input);
  }
  int rc;
  // FIXME: Should use waitpid ... when my OS supports that
  wait(&rc);
  return rc;
}

int execute_command(struct AST *ast, int input_fd) {
  char *program = ast->val.string;
  if (0 == strcmp(program, "cd")) {
    struct AST *child = ast->children;
    char *directory;
    if (!child)
      directory = "~";
    else
      directory = child->val.string;
    int rc = chdir(directory);
    if (-1 == rc) {
      perror("cd");
      return 1;
    }
    return 0;
  }
  return execute_binary(ast, input_fd);
}

void execute_ast(struct AST *ast) {
  int rc = -1;
  for (; ast;) {
    if (AST_COMMAND == ast->type) {
      rc = execute_command(ast, STDIN_FILENO);
    } else if (AST_CONDITIONAL_AND == ast->type) {
      if (0 != rc) {
        ast = ast->next;
        if (!ast)
          break;
      }
    } else if (AST_CONDITIONAL_NOT == ast->type) {
      if (0 == rc) {
        ast = ast->next;
        if (!ast)
          break;
      }
    }
    ast = ast->next;
  }
}

char *get_line(void) {
  char *str = malloc(1024);
  char *p = str;
  int rc;
  for (;;) {
    if (0 == (rc = read(0, p, 1))) {
      continue;
    }
    if (0 > rc) {
      perror("read");
      continue;
    }
    if (8 == *p) {
      if (p == str)
        continue;
      putchar(*p);
      p--;
      continue;
    }
    putchar(*p);
    if ('\n' == *p) {
      break;
    }
    p++;
  }
  p++;
  *p = '\0';
  return str;
}

int sh_main(int argc, char **argv) {
  (void)argc;
  (void)argv;
  for (;;) {
    printf("/ : ");
    char *line = get_line();
    {
      struct TOKEN *h = lex(line);
      struct AST *ast_h = generate_ast(h);
      execute_ast(ast_h);
      free_tokens(h);
      free_ast(ast_h);
    }
    free(line);
  }
  return 0;
}
