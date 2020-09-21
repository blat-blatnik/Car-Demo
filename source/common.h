#pragma once

#include <stdint.h>
#include <assert.h>
#include <stdlib.h>
#include <stdio.h>

#define countof(array) (sizeof(array) / sizeof(array[0]))

typedef uint32_t uint;

char *readWholeFile(const char *filename, size_t *outSize=NULL);

size_t hashBytes(const void *bytes, size_t numBytes);