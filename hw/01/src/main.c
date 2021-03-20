#include <stdio.h>
#include <time.h>

#include "sort_file.h"
#include "merge_files.h"
#include "coro.h"

static ucontext_t main_context;
static struct coroutines coros;

int main(int argc, char **argv) {

    init_coros(&coros, &main_context, argc - 1, CORO_STACK_SIZE);
    
    double *coros_time = calloc(argc - 1, sizeof(double));
    double main_time = clock(), full_time, sort_time;
    
    for (int i = 0; i < argc - 1; ++i) {
        makecontext(&coros.ctxt_arr[i], (void (*)(void)) sort, 
                5, i, argv[i + 1], coros.flags, &main_context, &coros.ctxt_arr[i]);
    }
    
    scheduler(&coros, &main_context, coros_time);
    
    sort_time = clock();
    
    free_coros(&coros);
    
    merge_files(1, argc - 1, argv);
    
    full_time = clock();
    
    double coro_sum = 0;
    printf("\n");
    for (int id = 0; id < argc - 1; ++id) {
        coro_sum += coros_time[id];
        printf("coro #%d -- %.4lfs\n", id + 1, coros_time[id] / 1000000.0);
    }
    printf("\nfull time: %.4lfs\n", (full_time - main_time) / 1000000.0);
    printf("sheduler efficiency %.4lf\n\n", coro_sum / (sort_time - main_time));
    
    free(coros_time);
    return 0;
}
