#include "parse_args.h"
#include "exec_pipeline.h"

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
