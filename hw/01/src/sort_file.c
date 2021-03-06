#include "sort_file.h"

static void quick_sort(uint32_t *arr, const int64_t begin, const int64_t end,
        ucontext_t *main_ctxt, ucontext_t *cur_ctxt) {
    uint32_t val = arr[(begin + end) / 2];
    int64_t i = begin, j = end;
    swapcontext(cur_ctxt, main_ctxt);
    while (i <= j) {
        while (arr[i] < val) {
            ++i;
            swapcontext(cur_ctxt, main_ctxt);
        }
        while (val < arr[j]) {
            --j;
            swapcontext(cur_ctxt, main_ctxt);
        }
        if (i <= j) {
            uint32_t tmp = arr[i];
            arr[i] = arr[j];
            arr[j] = tmp;
            ++i;
            --j;
        }
        swapcontext(cur_ctxt, main_ctxt);
    }
    if (begin < j) {
        swapcontext(cur_ctxt, main_ctxt);
        quick_sort(arr, begin, j, main_ctxt, cur_ctxt);
    }
    if (i < end) {
        swapcontext(cur_ctxt, main_ctxt);
        quick_sort(arr, i, end, main_ctxt, cur_ctxt);
    }
}

void sort(const int id, const char *filename, char *flags,
        ucontext_t *main_ctxt, ucontext_t *cur_ctxt) {
    uint32_t *arr;
    int64_t len = 0;
    int64_t alloc = MIN_BUF_SIZE;
    uint32_t val;
    FILE *fin;

    if (!(arr = calloc(alloc, sizeof(uint32_t)))) {
        fprintf(stderr, "Bad alloc\n");
        exit(1);
    }
    swapcontext(cur_ctxt, main_ctxt);

    if (!(fin = fopen(filename, "r"))) {
        free(arr);
        fprintf(stderr, "Bad opening file for reading: %s\n", filename);
        exit(2);
    }
    swapcontext(cur_ctxt, main_ctxt);
    while ((fscanf(fin, "%u", &val)) != -1) {
        if (len == alloc) {
            alloc <<= 1u;
            if (!(arr = realloc(arr, alloc * sizeof(uint32_t)))) {
                fprintf(stderr, "Bad alloc\n");
                exit(1);
            }
            swapcontext(cur_ctxt, main_ctxt);
        }
        arr[len++] = val;
        swapcontext(cur_ctxt, main_ctxt);
    }
    fclose(fin);

    swapcontext(cur_ctxt, main_ctxt);
    quick_sort(arr, 0, len - 1, main_ctxt, cur_ctxt);

    FILE *fout;
    if (!(fout = fopen(filename, "w"))) {
        free(arr);
        fprintf(stderr, "Bad opening file for writing: %s\n", filename);
        exit(2);
    }
    for (int64_t i = 0; i < len; ++i) {
        fprintf(fout, "%u ", arr[i]);
        swapcontext(cur_ctxt, main_ctxt);
    }
    fclose(fout);
    free(arr);
    swapcontext(cur_ctxt, main_ctxt);
    flags[id] = 0;
}
