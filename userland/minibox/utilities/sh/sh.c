#include "ast.h"
#include "lexer.h"
#include <fcntl.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/wait.h>
#include <unistd.h>

int active_processes = 0;

int execute_command(struct AST *ast, int input_fd);

int execute_binary(struct AST *ast, int input_fd) {
  char *program = SV_TO_C(ast->val.string);
  struct AST *child = ast->children;
  char *argv[100];
  argv[0] = program;
  int i = 1;
  for (; child; i++, child = child->next) {
    argv[i] = SV_TO_C(child->val.string);
  }
  argv[i] = NULL;

  int in = input_fd;
  int out = STDOUT_FILENO;
  int slave_input = -1;

  int file_out_fd;
  if (!sv_isempty(ast->file_out)) {
    char *tmp = SV_TO_C(ast->file_out);
    file_out_fd = open(
        tmp, O_WRONLY | O_CREAT | ((ast->file_out_append) ? O_APPEND : O_TRUNC),
        0666);
    free(tmp);
  }

  if (ast->pipe_rhs) {
    int fds[2];
    pipe(fds);
    out = fds[1];
    slave_input = fds[0];
  }

  int pid = fork();
  if (0 == pid) {
    if (slave_input >= 0) {
      close(slave_input);
    }
    dup2(in, STDIN_FILENO);
    dup2(out, STDOUT_FILENO);
    if (!sv_isempty(ast->file_out)) {
      dup2(file_out_fd, ast->file_out_fd_to_use);
    }

    execvp(program, argv);
    perror("execvp");
    exit(1);
  }

  for (int j = 0; j < i; j++) {
    free(argv[j]);
  }

  if (!sv_isempty(ast->file_out)) {
    close(file_out_fd);
  }

  if (ast->pipe_rhs) {
    if (out >= 0)
      close(out);
    return execute_command(ast->pipe_rhs, slave_input);
  }

  active_processes++;
  if (ast->should_background) {
    return pid;
  }

  int rc;
  // FIXME: Should use waitpid ... when my OS supports that
  for (; active_processes > 0;) {
    if (-1 == wait(&rc)) {
      continue;
    }
    active_processes--;
  }
  return rc;
}

int execute_command(struct AST *ast, int input_fd) {
  struct sv program = ast->val.string;
  if (sv_eq(program, C_TO_SV("cd"))) {
    struct AST *child = ast->children;
    struct sv directory;
    if (!child) {
      directory = C_TO_SV("~");
    } else {
      directory = child->val.string;
    }
    char *dir = SV_TO_C(directory);
    int rc = chdir(dir);
    free(dir);
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

void get_line(struct sb *s) {
  int rc;
  for (;;) {
    char c;
    if (0 == (rc = read(0, &c, 1))) {
      continue;
    }
    if (0 > rc) {
      perror("read");
      continue;
    }
    if ('\b' == c) {
      if (sb_delete_right(s, 1) > 0) {
        putchar('\b');
      }
      continue;
    }
    sb_append_char(s, c);
    putchar(c);
    if ('\n' == c) {
      break;
    }
  }
}

int sh_main(int argc, char **argv) {
  if (argc > 1) {
    int fd = open(argv[1], O_RDONLY, 0);
    char buffer[8192];
    struct sb file;
    sb_init(&file, 256);

    for (;;) {
      int rc = read(fd, buffer, 8192);
      if (0 == rc) {
        break;
      }
      sb_append_buffer(&file, buffer, rc);
    }

    close(fd);
    struct TOKEN *h = lex(SB_TO_SV(file));
    struct AST *ast_h = generate_ast(h);
    execute_ast(ast_h);
    free_tokens(h);
    free_ast(ast_h);
    sb_free(&file);
    return 0;
  }

  for (;;) {
    char buffer[256];
    printf("%s : ", getcwd(buffer, 256));
    struct sb line;
    sb_init(&line, 256);
    get_line(&line);
    {
      struct TOKEN *h = lex(SB_TO_SV(line));
      struct AST *ast_h = generate_ast(h);
      execute_ast(ast_h);
      free_tokens(h);
      free_ast(ast_h);
    }
    sb_free(&line);
  }
  return 0;
}
