#ifndef MERGE_FILES_H
#define MERGE_FILES_H

#include <stdio.h>
#include <stdlib.h>
#include <stdint.h>

#include "coro.h"

#define MIN_BUF_SIZE 16

void merge(const char *file1, const char *file2, const char *res_file);

void merge_files(const int left, const int right, char **files);

#endif
