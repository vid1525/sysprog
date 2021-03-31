#ifndef EXEC_PIPELINE_H
#define EXEC_PIPELINE_H

#include <stdio.h>
#include <stdlib.h>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <fcntl.h>
#include <sys/wait.h>

#include "parse_args.h"


void execute(struct command cmd);

char *execute_pipeline(char *cur_buf, int *command_flag, int *bool_value);

void execute_logical_commands(char *cmd_buf, const size_t cmd_buf_len);

#endif
