#include "merge_files.h"

static uint32_t *read_file(const char *filename, size_t *res_len) {
    FILE *fin = fopen(filename, "r");
    uint32_t len = 0;
    uint32_t alloc = MIN_BUF_SIZE;
    uint32_t *arr = malloc(sizeof(uint32_t) * alloc);
    uint32_t val;
    while (fscanf(fin, "%u", &val) != -1) {
        if (len == alloc) {
            alloc <<= 1u;
            arr = realloc(arr, sizeof(uint32_t) * alloc);
        }
        arr[len++] = val;
    }
    fclose(fin);
    *res_len = len;
    return arr;
}

void merge(const char *file1, const char *file2, const char *res_file) {
    uint32_t *arr1, *arr2;
    size_t len1, len2;
    
    arr1 = read_file(file1, &len1);
    arr2 = read_file(file2, &len2);
    
    FILE *fout = fopen(res_file, "w");
    
    size_t i = 0;
    size_t j = 0;
    while (i < len1 && j < len2) {
        if (arr1[i] < arr2[j]) {
            fprintf(fout, "%u ", arr1[i]);
            ++i;
        } else {
            fprintf(fout, "%u ", arr2[j]);
            ++j;
        }
    }
    if (i == len1) {
        while (j < len2) {
            fprintf(fout, "%u ", arr2[j]);
            ++j;
        }
    } else {
        while (i < len1) {
            fprintf(fout, "%u ", arr1[i]);
            ++i;
        }
    }
    
    fclose(fout);
    free(arr1);
    free(arr2);
    return;
}

void merge_files(const int left, const int right, char **files) {
    if (left >= right) {
        return;
    }
    int mid = (left + right) / 2;
    merge_files(left, mid, files);
    merge_files(mid + 1, right, files);
    merge(files[left], files[mid + 1], files[left]);
}
