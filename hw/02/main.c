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
            exit(5);
        } else {
            int status;
            sscanf(cmd.argv[1], "%d", &status);
            exit(status);
        }
    }
}

void execute(struct command cmd) {
    exit_call(cmd);
    execvp(cmd.argv[0], cmd.argv);
    fprintf(stderr, "Bad launch of func %s\n", cmd.argv[0]);
    exit(2);
}

int main(void) {
    while (1) {
        char *cmd_buffer;
        get_buffer(stdin, &cmd_buffer);
        size_t cmd_buffer_len = strlen(cmd_buffer) - 1;

        char *cur_buf = cmd_buffer;
        struct command *cmds = calloc(1, sizeof(*cmds));
        if (!cmds) {
            error_msg(stderr, "Bad alloc\n", 1);
        }
        size_t cmd_alloc = 1;
        size_t cmd_size = 0;

        if (!cmd_buffer[0]) {
            free(cmd_buffer);
            free(cmds);
            break;
        }
        
        int loop_flag = 1;
        
	int pipe_fds[2][2];
	int read_pipe = 0;
	int pipe_turn = 0;
        while (loop_flag) {
            //printf("$>");
            //fflush(stdout);
            cur_buf = skip_spaces(cur_buf);
            if (*cur_buf == '\n' || *cur_buf == '#') {
                break;
            }
            if (cmd_size == cmd_alloc) {
                cmd_alloc <<= 1u;
                cmds = realloc(cmds, sizeof(*cmds) * cmd_alloc);
                if (!cmds) {
                    error_msg(stderr, "Bad alloc\n", 1);
                }
            }
            ++cmd_size;
            command_init(&cmds[cmd_size - 1]);
            
            char next_char = make_command(&cmds[cmd_size - 1], &cur_buf);

            switch (next_char) {
                case '\n':
                    if (cur_buf < cmd_buffer + cmd_buffer_len) {
                        ++cur_buf;
                        continue;
                    }
                    loop_flag = 0;
                    if (!strcmp(cmds[cmd_size - 1].argv[0], "cd")) {
                        if (cmds[cmd_size - 1].args > 2) {
                            error_msg(stderr, "Too many args\n", 6);
                        }
                        if (chdir(cmds[cmd_size - 1].argv[1])) {
                            error_msg(stderr, "Wrong dir path\n", 5);
                        }
                        break;
                    }
                    if (cmds[cmd_size - 1].args > 0 && !fork()) {
                        if (read_pipe) {
                            close(pipe_fds[1 - pipe_turn][1]);
                            dup2(pipe_fds[1 - pipe_turn][0], 0);
                            close(pipe_fds[1 - pipe_turn][0]);
                        }
                        execute(cmds[cmd_size - 1]);
                    }
                    if (read_pipe) {
                        close(pipe_fds[1 - pipe_turn][0]);
                        close(pipe_fds[1 - pipe_turn][1]);
                    }
                    break;
                case '>':
                    ++cur_buf;
                    int fd;
                    char *filename;
                    if (*cur_buf == '>') {
                        ++cur_buf;
                        filename = read_filename(&cur_buf);
                        fd = open(filename, O_CREAT | O_APPEND | O_WRONLY, 0664);
                    } else {
                        filename = read_filename(&cur_buf);
                        fd = open(filename, O_CREAT | O_WRONLY | O_TRUNC, 0664);
                    }
                    if (fd < 0) {
                        error_msg(stderr, "Bad file descriptor\n", 3);
                    }
                    free(filename);
                    if (cmds[cmd_size - 1].args > 0 && !fork()) {
                        if (read_pipe) {
                            close(pipe_fds[1 - pipe_turn][1]);
                            dup2(pipe_fds[1 - pipe_turn][0], 0);
                            close(pipe_fds[1 - pipe_turn][0]);
                        }
                        dup2(fd, 1);
                        close(fd);
                        execute(cmds[cmd_size - 1]);
                    }
                    close(fd);
                    if (read_pipe) {
                        read_pipe = 0;
                        close(pipe_fds[1 - pipe_turn][0]);
                        close(pipe_fds[1 - pipe_turn][1]);
                    }
                    break;
                case '|':
                    ++cur_buf;
                    if (*cur_buf == '|') {
                        
                    } else {
                        if (pipe(pipe_fds[pipe_turn])) {
                            error_msg(stderr, "Bad pipe opening\n", 4);
                        }
                        if (cmds[cmd_size - 1].args > 0 && !fork()) {
                            if (read_pipe) {
                                close(pipe_fds[1 - pipe_turn][1]);
                                dup2(pipe_fds[1 - pipe_turn][0], 0);
                                close(pipe_fds[1 - pipe_turn][0]);
                            }
                            close(pipe_fds[pipe_turn][0]);
                            dup2(pipe_fds[pipe_turn][1], 1);
                            close(pipe_fds[pipe_turn][1]);
                            execute(cmds[cmd_size - 1]);
                        }
                        if (read_pipe) {
                            read_pipe = 0;
                            close(pipe_fds[1 - pipe_turn][0]);
                            close(pipe_fds[1 - pipe_turn][1]);
                        }
                        read_pipe = 1;
                        pipe_turn = 1 - pipe_turn;
                    }
                    break;
                case '&':
                    break;
                default:
                    break;
            }
        }
        
        while (wait(NULL) != -1);
        
        for (size_t i = 0; i < cmd_size; ++i) {
            free_command(&cmds[i]);
        }
        free(cmds);
        free(cmd_buffer);
    }
    return 0;
}
