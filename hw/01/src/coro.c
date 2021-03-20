#include "coro.h"

static void *allocate_stack(const size_t stack_size) {
    void *stack = calloc(stack_size, sizeof(char));
    if (stack == NULL) {
        return NULL;
    }
    mprotect(stack, stack_size, PROT_READ | PROT_WRITE | PROT_EXEC);
    return stack;
}

void init_coros(struct coroutines *res, ucontext_t *main_ctxt, const size_t coro_cnt,
        const size_t stack_size) {
    if (!(res->ctxt_arr = calloc(coro_cnt, sizeof(ucontext_t)))) {
        fprintf(stderr, "Bad alloc\n");
        exit(1);
    }
    if (!(res->flags = calloc(coro_cnt, sizeof(char)))) {
        free(res->ctxt_arr);
        fprintf(stderr, "Bad alloc\n");
        exit(1);
    }
    if (!(res->index_arr = calloc(coro_cnt, sizeof(size_t)))) {
        free(res->ctxt_arr);
        free(res->flags);
        fprintf(stderr, "Bad alloc\n");
        exit(1);
    }
    res->context_cnt = coro_cnt;
    res->flags_cnt = coro_cnt;
    for (int64_t i = 0; i < coro_cnt; ++i) {
        if ((getcontext(&res->ctxt_arr[i])) == -1) {
            --i;
            for (; i >= 0; --i) {
                free(res->ctxt_arr[i].uc_stack.ss_sp);
            }
            free(res->ctxt_arr);
            fprintf(stderr, "Bad alloc\n");
            exit(1);
        }
        if (!(res->ctxt_arr[i].uc_stack.ss_sp = allocate_stack(stack_size))) {
            --i;
            for (; i >= 0; --i) {
                free(res->ctxt_arr[i].uc_stack.ss_sp);
            }
            free(res->ctxt_arr);
            fprintf(stderr, "Bad alloc\n");
            exit(1);
        }
        res->ctxt_arr[i].uc_stack.ss_size = stack_size;
        res->ctxt_arr[i].uc_link = main_ctxt;
    }

    for (size_t i = 0; i < coro_cnt; ++i) {
        res->flags[i] = 1;
        res->index_arr[i] = i;
    }
}

void free_coros(struct coroutines *coros) {
    for (size_t i = 0; i < coros->context_cnt; ++i) {
        free(coros->ctxt_arr[i].uc_stack.ss_sp);
    }
    free(coros->flags);
    free(coros->index_arr);
    free(coros->ctxt_arr);
}

void scheduler(struct coroutines *coros, ucontext_t *main_ctxt, double *exec_time) {
    size_t index = 0;
    while (coros->flags_cnt > 0) {
        index = (index + 1) % coros->flags_cnt;
        if (coros->flags[coros->index_arr[index]]) {
            double t = clock();
            swapcontext(main_ctxt, &coros->ctxt_arr[coros->index_arr[index]]);
            exec_time[coros->index_arr[index]] += clock() - t;
        } else {
            --coros->flags_cnt;
            coros->index_arr[index] = coros->index_arr[coros->flags_cnt];
        }
    }
}
