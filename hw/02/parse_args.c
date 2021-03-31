#include "parse_args.h"

// error message

void error_msg(FILE *stream, const char *msg, const int status) {
    fprintf(stream, "%s\n", msg);
    exit(status);
}

// help functions

char *skip_spaces(char *buf) {
    while (*buf == ' ') {
        ++buf;
    }
    return buf;
}

int not_special_chars(const char c) {
    return c != '|' && c != '\n' && c != 0;
}

// input command string

static void get_buffer_bracket_case(const char *buf, const size_t size,
        int *bracket_flag, int *back_slash_flag, const int bracket_type) {
    if (size > 0 && buf[size - 1] == '\\' && *back_slash_flag) {
        *back_slash_flag = 0;
        return;
    }
    if (*bracket_flag == bracket_type) {
        *bracket_flag = 0;
    } else if (*bracket_flag == 0) {
        *bracket_flag = bracket_type;
    }
}

void get_buffer(FILE *stream, char **res) {
    size_t buf_alloc = 1;
    char *buf = calloc(buf_alloc, sizeof(char));
    if (!buf) {
        error_msg(stderr, "Bad alloc\n", 1);
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
                error_msg(stderr, "Bad alloc\n", 1);
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
                buf[size] = 0;
                *res = buf;
                return;
            case '"':
                get_buffer_bracket_case(buf, size, &bracket_flag, &back_slash_flag, 1);
                break;
            case '\'':
                get_buffer_bracket_case(buf, size, &bracket_flag, &back_slash_flag, 2);
                break;
            default:
                break;
        }
        buf[size++] = c;
    }
    buf = realloc(buf, sizeof(*buf) * (size + 1));
    if (!buf) {
        error_msg(stderr, "Bad alloc\n", 1);
    }
    buf[size] = 0;
    *res = buf;
}

// help functions working with filenames and commands' names

static int pred_bracket_1(const char c, const int flag) {
    return (flag || c != '"');
}

static int pred_bracket_2(const char c, const int flag) {
    return (flag || c != '\'');
}

static int pred_default(const char c, const int flag) {
    return (not_special_chars(c) && (flag || c != ' '));
}

// input filename

static char* filename_skip_by_predicate(char *buf, int *flag, 
        int (*pred)(const char, const int), size_t *alloc, size_t *size, char **res) {
     while (pred(*buf, *flag)) {
         if (*size == *alloc) {
             *alloc <<= 1u;
             *res = realloc(*res, sizeof(**res) * (*alloc));
             if (!(*res)) {
                 error_msg(stderr, "Bad alloc\n", 1);
             }
         }
         if (*buf == '\\' && !(*flag)) {
             *flag = 1;
             ++buf;
         } else {
             *flag = 0;
             (*res)[(*size)++] = *buf;
             ++buf;
         }
     }
     return buf;
}

char *read_filename(char **buf) {
    *buf = skip_spaces(*buf);
    char *res = calloc(1, sizeof(*res));
    if (!res) {
        error_msg(stderr, "Bad alloc\n", 1);
    }
    size_t alloc = 1;
    size_t size = 0;

    int flag = 0;
    switch (**buf) {
        case '"':
            ++(*buf);
            *buf = filename_skip_by_predicate(*buf, &flag, pred_bracket_1, &alloc, &size, &res);
            if (**buf != '"') {
                error_msg(stderr, "Bad command\n", 2);
            }
            ++(*buf);
            break;
        case '\'':
            ++(*buf);
            *buf = filename_skip_by_predicate(*buf, &flag, pred_bracket_2, &alloc, &size, &res);
            if (**buf != '\'') {
                error_msg(stderr, "Bad command\n", 2);
            }
            ++(*buf);
            break;
        default:
            *buf = filename_skip_by_predicate(*buf, &flag, pred_default, &alloc, &size, &res);
            break;
    }
    if (size == alloc) {
        res = realloc(res, sizeof(*res) * (alloc + 1));
    }
    if (!res) {
        error_msg(stderr, "Bad alloc\n", 1);
    }
    res[size] = 0;
    return res;
}

// work with struct command

static int32_t skip_by_predicate(const char *buf, int32_t i, int *flag, 
        int (*pred)(const char, const int)) {
    while (pred(buf[i], *flag)) {
        *flag = 0;
        if (buf[i] == '\\' && buf[i + 1] == ' ') {
            *flag = 1;
        }
        while (buf[i] == '\\' && buf[i + 1] == '\\') {
            ++i;
        }
        ++i;
    }
    return i;
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
    if (i < j) {
        buf[i] = 0;
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
                i = skip_by_predicate(buf, i, &flag, pred_bracket_1);
                buf[i++] = 0;
                break;
            case '\'':
                ++i;
                append_arg(cmd, buf + i);
                i = skip_by_predicate(buf, i, &flag, pred_bracket_2);
                buf[i++] = 0;
                break;
            default:
                append_arg(cmd, buf + i);
                i = skip_by_predicate(buf, i, &flag, pred_default);
                break;
        }
    }
    append_arg(cmd, NULL);
    --cmd->args;

    char res_char = buf[i];
    buf[i] = 0;
    
    for (size_t j = 0; j < cmd->args; ++j) {
        update_arg(&(cmd->argv[j]));
    }
    
    *global_buf = buf + i;
    return res_char;
}

void command_init(struct command *cmd) {
    cmd->args = 0;
    cmd->args_alloc = 1;
    if (!(cmd->argv = calloc(cmd->args_alloc, sizeof(char *)))) {
        error_msg(stderr, "Bad alloc\n", 1);
    }
}

void append_arg(struct command *cmd, char *arg) {
    if (cmd->args == cmd->args_alloc) {
        cmd->args_alloc <<= 1u;
        cmd->argv = realloc(cmd->argv, sizeof(*cmd->argv) * cmd->args_alloc);
        if (!cmd->argv) {
            error_msg(stderr, "Bad alloc\n", 1);
        }
    }
    cmd->argv[cmd->args++] = arg;
}

void free_command(struct command *cmd) {
    free(cmd->argv);
}
