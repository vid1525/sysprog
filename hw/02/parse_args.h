#ifndef PARSE_ARGS_H
#define PARSE_ARGS_H

#include <stdio.h>
#include <unistd.h>
#include <fcntl.h>
#include <stdlib.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <string.h>

struct command {
    char **argv;
    size_t args;
    size_t args_alloc;
};

void get_buffer(FILE *stream, char **res);

void command_init(struct command *cmd);

void append_arg(struct command *cmd, char *arg);

char *skip_spaces(char *buf);

int not_special_chars(const char c);

char *read_filename(char **buf);

char make_command(struct command *cmd, char **global_buf);

void free_command(struct command *cmd);

#endif
