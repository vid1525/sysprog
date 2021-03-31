#include <stdio.h>
#include <ctype.h>
#include <stdlib.h>
#include <sys/wait.h>
#include <sys/types.h>
#include <unistd.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <string.h>

#include "parse_args.h"

void exit_call(struct command cmd) {
    if (!strcmp(cmd.argv[0], "exit")) {
        if (cmd.args == 1) {
            exit(0);
        } else {
            int status;
            sscanf(cmd.argv[1], "%d", &status);
            exit(status);
        }
    }
}

int chdir_call(struct command cmd) {
    if (!strcmp(cmd.argv[0], "cd")) {
        if (cmd.args > 2) {
            return 2;
        }
        if (chdir(cmd.argv[1])) {
            return 2;
        }
        return 1;
    }
    return 0;
}

void execute(struct command cmd) {
    if (!strcmp(cmd.argv[0], "true")) {
        exit(0);
    } else if (!strcmp(cmd.argv[0], "false")) {
        exit(1);
    }
    exit_call(cmd);
    execvp(cmd.argv[0], cmd.argv);
    exit(2);
}

int skip_before_next_logical(char **cmd_buf) {
    char *buf = *cmd_buf;
    while (*buf) {
        if (*buf == '&') {
            buf[0] = buf[1] = 0;
            buf += 2;
            *cmd_buf = buf;
            return 0;
        } else if (*buf == '|') {
            *buf = 0;
            ++buf;
            if (*buf == '|') {
                *buf = 0;
                ++buf;
                *cmd_buf = buf;
                return 1;
            }
        }
        ++buf;
    }
    *cmd_buf = buf;
    return -1;
}

char *execute_pipeline(char *cur_buf, int *command_flag, int *bool_value) {
    char *cmd_line = cur_buf;
    *command_flag = skip_before_next_logical(&cur_buf);
    
    int pipe_fds[2][2];
    int read_pipe = 0;
    int pipe_turn = 0;
    while (cmd_line < cur_buf) {
        struct command cmd = {0};
        command_init(&cmd);

        char c = make_command(&cmd, &cmd_line);
        switch (c) {
            case '\n':
                ++cmd_line;
                
                {
                    if (chdir_call(cmd)) {
                        *bool_value = 0;
                        if (read_pipe) {
		            close(pipe_fds[1 - pipe_turn][0]);
		            close(pipe_fds[1 - pipe_turn][1]);
		        }
		        free_command(&cmd);
		        return cur_buf;
                    }
                }

                if (cmd.args > 0 && !fork()) {
                    if (read_pipe) {
                        close(pipe_fds[1 - pipe_turn][1]);
                        dup2(pipe_fds[1 - pipe_turn][0], 0);
                        close(pipe_fds[1 - pipe_turn][0]);
                    }
                    execute(cmd);
                }
                
                if (read_pipe) {
                    read_pipe = 0;
                    close(pipe_fds[1 - pipe_turn][0]);
                    close(pipe_fds[1 - pipe_turn][1]);
                }

                break;
            case '>':
                ++cmd_line;
                char *filename;
                int fd = -1;
                if (*cmd_line == '>') {
                    ++cmd_line;
                    filename = read_filename(&cmd_line);
                    fd = open(filename, O_CREAT | O_APPEND | O_WRONLY, 0664);
                } else {
                    filename = read_filename(&cmd_line);
                    fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0664);
                }
                free(filename);
                
                if (fd < 0) {
                    error_msg(stderr, "Bad descriptor\n", 4);
                }
                
                {
                    if (chdir_call(cmd)) {
                        *bool_value = 0;
                        if (read_pipe) {
		            close(pipe_fds[1 - pipe_turn][0]);
		            close(pipe_fds[1 - pipe_turn][1]);
		        }
		        free_command(&cmd);
		        return cur_buf;
                    }
                }

                if (cmd.args > 0 && !fork()) {
                    if (read_pipe) {
                        close(pipe_fds[1 - pipe_turn][1]);
                        dup2(pipe_fds[1 - pipe_turn][0], 0);
                        close(pipe_fds[1 - pipe_turn][0]);
                    }
                    dup2(fd, 1);
                    close(fd);
                    execute(cmd);
                }
                close(fd);
                
                if (read_pipe) {
                    read_pipe = 0;
                    close(pipe_fds[1 - pipe_turn][0]);
                    close(pipe_fds[1 - pipe_turn][1]);
                }
                
                break;
            case '|':
                ++cmd_line;
                if (pipe(pipe_fds[pipe_turn])) {
                    error_msg(stderr, "Bad pipe opening\n", 5);
                }

                {
                    if (chdir_call(cmd)) {
                        *bool_value = 0;
                        if (read_pipe) {
		            close(pipe_fds[1 - pipe_turn][0]);
		            close(pipe_fds[1 - pipe_turn][1]);
		        }
		        free_command(&cmd);
		        return cur_buf;
                    }
                }

                if (cmd.args > 0 && !fork()) {
                    if (read_pipe) {
                        close(pipe_fds[1 - pipe_turn][1]);
                        dup2(pipe_fds[1 - pipe_turn][0], 0);
                        close(pipe_fds[1 - pipe_turn][0]);
                    }
                    close(pipe_fds[pipe_turn][0]);
                    dup2(pipe_fds[pipe_turn][1], 1);
                    close(pipe_fds[pipe_turn][1]);
                    execute(cmd);
                }
                
                if (read_pipe) {
                    close(pipe_fds[1 - pipe_turn][0]);
                    close(pipe_fds[1 - pipe_turn][1]);
                }
                pipe_turn = 1 - pipe_turn;
                break;
            default:
                break;
        }
        free_command(&cmd);
    }
    {
        int status;
        *bool_value = 0;
        while (wait(&status) > 0) {
            *bool_value = (*bool_value || (status == 0));
        }
    }
    return cur_buf;
}

void execute_logical_commands(char *cmd_buf, const size_t cmd_buf_len) {
    char *cur_buf = cmd_buf;
    while (cur_buf < cmd_buf + cmd_buf_len) {
        int command_flag = 0; // 0 -- and; 1 -- or
        int bool_value = 0;
        cur_buf = execute_pipeline(cur_buf, &command_flag, &bool_value);
        if (bool_value) {
            if (command_flag == 1) {
                while (1) {
                    int c = skip_before_next_logical(&cur_buf);
                    if (c == 0) {
                        break;
                    }
                    if (c == -1) {
                        return;
                    }
                }
            }
        } else {
            if (command_flag == 0) {
                while (1) {
                    int c = skip_before_next_logical(&cur_buf);
                    if (c == 1) {
                        break;
                    }
                    if (c == -1) {
                        return;
                    }
                }
            }
        }
        if (command_flag == -1) {
            break;
        }
    }
}

int main(void) {
    while (1) {
        char *cmd_buffer;
        if (!get_buffer(stdin, &cmd_buffer)) {
            break;
        }
        size_t cmd_buffer_len = strlen(cmd_buffer) - 1;

        execute_logical_commands(cmd_buffer, cmd_buffer_len);
        free(cmd_buffer);
    }
    return 0;
}
