#ifndef SORT_H
#define SORT_H

#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#include "coro.h"

#define MIN_BUF_SIZE 16

void sort(const int id, const char *filename, char *flags,
        ucontext_t *main_ctxt, ucontext_t *cur_ctxt);

#endif
