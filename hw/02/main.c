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

int main(void) {
    int main_loop = 1;
    while (main_loop) {
        char *cmd_buffer;
        get_buffer(stdin, &cmd_buffer);
        size_t cmd_buffer_len = strlen(cmd_buffer) - 1;
        //printf("%s\n", cmd_buffer);
        
        char *cur_buf = cmd_buffer;
        struct command *cmds = calloc(1, sizeof(*cmds));
        if (!cmds) {
            fprintf(stderr, "Bad alloc\n");
            exit(1);
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
            cur_buf = skip_spaces(cur_buf);
            if (*cur_buf == '\n' || *cur_buf == '#') {
                break;
            }
            if (cmd_size == cmd_alloc) {
                cmd_alloc <<= 1u;
                cmds = realloc(cmds, sizeof(*cmds) * cmd_alloc);
                if (!cmds) {
                    fprintf(stderr, "Bad alloc\n");
                    exit(1);
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
                            fprintf(stderr, "Too many args\n");
                            exit(6);
                        }
                        /* for command cd ~
                        if (cmds[cmd_size - 1].args == 1) {
                            if (chdir("~")) {
                                fprintf(stderr, "Wrong dir path\n");
                                exit(5);
                            }
                            break;
                        }
                        */
                        if (chdir(cmds[cmd_size - 1].argv[1])) {
                            fprintf(stderr, "Wrong dir path\n");
                            exit(5);
                        }
                        break;
                    }
                    if (cmds[cmd_size - 1].args > 0 && !fork()) {
                        if (read_pipe) {
                            close(pipe_fds[1 - pipe_turn][1]);
                            dup2(pipe_fds[1 - pipe_turn][0], 0);
                            close(pipe_fds[1 - pipe_turn][0]);
                        }
                        if (!strcmp(cmds[cmd_size - 1].argv[0], "exit")) {
                            if (cmds[cmd_size - 1].args == 1) {
                                exit(5);
                            } else {
                                int status;
                                sscanf(cmds[cmd_size - 1].argv[1], "%d", &status);
                                exit(status);
                            }
                        }
                        execvp(cmds[cmd_size - 1].argv[0], cmds[cmd_size - 1].argv);
                        fprintf(stderr, "Bad launch of func %s\n", cmds[cmd_size - 1].argv[0]);
                        exit(2);
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
                        fprintf(stderr, "Bad file descriptor\n");
                        exit(3);
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
                        if (!strcmp(cmds[cmd_size - 1].argv[0], "exit")) {
                            if (cmds[cmd_size - 1].args == 1) {
                                exit(5);
                            } else {
                                int status;
                                sscanf(cmds[cmd_size - 1].argv[1], "%d", &status);
                                exit(status);
                            }
                        }
                        execvp(cmds[cmd_size - 1].argv[0], cmds[cmd_size - 1].argv);
                        fprintf(stderr, "Bad launch of func %s\n", cmds[cmd_size - 1].argv[0]);
                        exit(2);
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
                    if (pipe(pipe_fds[pipe_turn])) {
                        fprintf(stderr, "Bad pipe opening\n");
                        exit(4);
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
                        
                        if (!strcmp(cmds[cmd_size - 1].argv[0], "exit")) {
                            if (cmds[cmd_size - 1].args == 1) {
                                exit(5);
                            } else {
                                int status;
                                sscanf(cmds[cmd_size - 1].argv[1], "%d", &status);
                                exit(status);
                            }
                        }
                        execvp(cmds[cmd_size - 1].argv[0], cmds[cmd_size - 1].argv);
                        fprintf(stderr, "Bad launch of func %s\n", cmds[cmd_size - 1].argv[0]);
                        exit(2);
                    }
                    if (read_pipe) {
                        read_pipe = 0;
                        close(pipe_fds[1 - pipe_turn][0]);
                        close(pipe_fds[1 - pipe_turn][1]);
                    }
                    read_pipe = 1;
                    pipe_turn = 1 - pipe_turn;
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
