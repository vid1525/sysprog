#include "parse_args.h"

void get_buffer(FILE *stream, char **res) {
    size_t buf_alloc = 1;
    char *buf = calloc(buf_alloc, sizeof(char));
    if (!buf) {
        fprintf(stderr, "Bad alloc\n");
        exit(1);
    }
    size_t size = 0;

    char c;
    int bracket_flag = 0;
    int back_slash_flag = 0;
    while ((c = getc(stream)) != EOF) {
        if (size == buf_alloc) {
            buf_alloc <<= 1u;
            buf = realloc(buf, sizeof(*buf) * buf_alloc);
            if (!buf) {
                fprintf(stderr, "Bad alloc\n");
                exit(1);
            }
        }

        back_slash_flag = (back_slash_flag ^ (c == '\\'));
        switch (c) {
            case '\n':
                if (size > 0 && buf[size - 1] == '\\' && back_slash_flag) {
                    back_slash_flag = 0;
                    --size;
                    continue;
                }
                if (bracket_flag) {
                    buf[size++] = '\n';
                    continue;
                }
                buf[size] = '\n';
                memset(buf + size + 1, 0, sizeof(*buf) * (buf_alloc - size - 1));
                *res = buf;
                return;
            case '"':
                if (size > 0 && buf[size - 1] == '\\' && back_slash_flag) {
                    back_slash_flag = 0;
                    break;
                }
                if (bracket_flag == 1) {
                    bracket_flag = 0;
                } else if (bracket_flag == 0) {
                    bracket_flag = 1;
                }
                break;
            case '\'':
                if (size > 0 && buf[size - 1] == '\\' && back_slash_flag) {
                    back_slash_flag = 0;
                    break;
                }
                if (bracket_flag == 2) {
                    bracket_flag = 0;
                } else if (bracket_flag == 0) {
                    bracket_flag = 2;
                }
                break;
            default:
                break;
        }
        buf[size++] = c;
    }
    memset(buf + size + 1, 0, sizeof(*buf) * (buf_alloc - size - 1));
    *res = buf;
}

void command_init(struct command *cmd) {
    cmd->args = 0;
    cmd->args_alloc = 1;
    if (!(cmd->argv = calloc(cmd->args_alloc, sizeof(char *)))) {
        fprintf(stderr, "Bad alloc\n");
        exit(1);
    }
}

void append_arg(struct command *cmd, char *arg) {
    if (cmd->args == cmd->args_alloc) {
        cmd->args_alloc <<= 1u;
        cmd->argv = realloc(cmd->argv, sizeof(*cmd->argv) * cmd->args_alloc);
        if (!cmd->argv) {
            fprintf(stderr, "Bad alloc\n");
            exit(1);
        }
    }
    cmd->argv[cmd->args++] = arg;
    for (size_t i = cmd->args; i < cmd->args_alloc; ++i) {
        cmd->argv[i] = NULL;
    }
}

char *skip_spaces(char *buf) {
    while (*buf == ' ') {
        ++buf;
    }
    return buf;
}

int not_special_chars(const char c) {
    return c != '>' && c != '<' && c != '|' && c != '&' && c != '\n';
}

char *read_filename(char **buf) {
    *buf = skip_spaces(*buf);
    char *res = calloc(1, sizeof(*res));
    if (!res) {
        fprintf(stderr, "Bad alloc\n");
        exit(1);
    }
    size_t alloc = 1;
    size_t size = 0;

    int flag = 0;
    switch (**buf) {
        case '"':
            ++(*buf);
            while (flag || **buf != '"') {
                if (size == alloc) {
                    alloc <<= 1u;
                    res = realloc(res, sizeof(*res) * alloc);
                    if (!res) {
                        fprintf(stderr, "Bad alloc\n");
                        exit(1);
                    }
                }
                if (**buf == '\\' && !flag) {
                    flag = 1;
                    ++(*buf);
                } else {
                    flag = 0;
                    res[size++] = **buf;
                    ++(*buf);
                }
            }
            if (**buf != '"') {
                fprintf(stderr, "Bad command\n");
                exit(5);
            }
            ++(*buf);
            break;
        case '\'':
            ++(*buf);
            while (flag || **buf != '\'') {
                if (size == alloc) {
                    alloc <<= 1u;
                    res = realloc(res, sizeof(*res) * alloc);
                    if (!res) {
                        fprintf(stderr, "Bad alloc\n");
                        exit(1);
                    }
                }
                if (**buf == '\\' && !flag) {
                    flag = 1;
                    ++(*buf);
                } else {
                    flag = 0;
                    res[size++] = **buf;
                    ++(*buf);
                }
            }
            if (**buf != '\'') {
                fprintf(stderr, "Bad command\n");
                exit(5);
            }
            ++(*buf);
            break;
        default:
            while (not_special_chars(**buf) && (flag || **buf != ' ')) {
                if (size == alloc) {
                    alloc <<= 1u;
                    res = realloc(res, sizeof(*res) * alloc);
                    if (!res) {
                        fprintf(stderr, "Bad alloc\n");
                        exit(1);
                    }
                }
                if (**buf == '\\' && !flag) {
                    flag = 1;
                    ++(*buf);
                } else {
                    flag = 0;
                    res[size++] = **buf;
                    ++(*buf);
                }
            }
            break;
    }
    if (size == alloc) {
        res = realloc(res, sizeof(*res) * (alloc + 1));
    }
    if (!res) {
        fprintf(stderr, "Bad alloc\n");
        exit(1);
    }
    res[size] = 0;
    return res;
}

static void update_arg(char **arg) {
    char *buf = *arg;
    size_t i = 0;
    size_t j = 0;
    
    while (buf[j] && buf[j] != '\n') {
        if (buf[j] == '\\') {
            ++j;
        }
        buf[i] = buf[j];
        ++i;
        ++j;
    }
    for (size_t k = i; k < j; ++k) {
        buf[k] = 0;
    }
}

char make_command(struct command *cmd, char **global_buf) {
    char *buf = *global_buf;
    int32_t i = 0;
    int flag = 0;
    while (not_special_chars(buf[i])) {
        switch (buf[i]) {
            case ' ':
                buf[i] = 0;
                ++i;
                break;
            case '"':
                ++i;
                append_arg(cmd, buf + i);
                while (flag || buf[i] != '"') {
                    flag = 0;
                    if (buf[i] == '\\' && buf[i + 1] == '"') {
                       flag = 1;
                    }
                    while (buf[i] == '\\' && buf[i + 1] == '\\') {
                        ++i;
                    }
                    ++i;
                }
                buf[i++] = 0;
                break;
            case '\'':
                ++i;
                append_arg(cmd, buf + i);
                while (flag || buf[i] != '\'') {
                    flag = 0;
                    if (buf[i] == '\\' && buf[i + 1] == '\'') {
                       flag = 1;
                    }
                    while (buf[i] == '\\' && buf[i + 1] == '\\') {
                        ++i;
                    }
                    ++i;
                }
                buf[i++] = 0;
                break;
            default:
                append_arg(cmd, buf + i);
                while (not_special_chars(buf[i]) && (flag || buf[i] != ' ')) {
                    flag = 0;
                    if (buf[i] == '\\' && buf[i + 1] == ' ') {
                       flag = 1;
                    }
                    while (buf[i] == '\\' && buf[i + 1] == '\\') {
                        ++i;
                    }
                    ++i;
                }
                break;
        }
    }
    cmd->argv[cmd->args] = NULL;

    char res_char = buf[i];
    buf[i] = 0;
    
    for (size_t j = 0; j < cmd->args; ++j) {
        update_arg(&(cmd->argv[j]));
    }
    
    *global_buf = buf + i;
    return res_char;
}

void free_command(struct command *cmd) {
    free(cmd->argv);
}
