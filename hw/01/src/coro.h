#ifndef CORO_H
#define CORO_H

#define _XOPEN_SOURCE /* Mac compatibility. */
#include <stdio.h>
#include <ucontext.h>
#include <stdlib.h>
#include <sys/mman.h>
#include <stdint.h>
#include <time.h>

enum {
    CORO_STACK_SIZE = 64 * 1024
};

struct coroutines {
    size_t context_cnt;
    ucontext_t *ctxt_arr;
    size_t flags_cnt;
    char *flags;
    size_t *index_arr;
};

void init_coros(struct coroutines *res, ucontext_t *main_ctxt,
        const size_t coro_cnt, const size_t stack_size);

void free_coros(struct coroutines *coros);

void scheduler(struct coroutines *coros, ucontext_t *main_ctxt, double *exec_time);

#endif
