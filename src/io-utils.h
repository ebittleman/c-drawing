#ifndef INCLUDE_IOUTILS_H
#define INCLUDE_IOUTILS_H

#include <errno.h>
#include <stdio.h>

#include "arena.h"

const char *read_entire_file(arena *a, const char *filename);

#endif

#ifdef IOUTILS_IMPLEMENTATION

const char *read_entire_file(arena *a, const char *filename) {
  FILE *fp = fopen(filename, "r");
  if (fp == NULL) {
    return NULL;
  }
  if (fseek(fp, 0, SEEK_END) < 0) {
    fclose(fp);
    return NULL;
  }

  long size = ftell(fp);
  if (size < 0) {
    fclose(fp);
    return NULL;
  }

  rewind(fp);

  char *data = (char*) arena_alloc(a, size + 1);
  if (fread(data, 1, (size_t)size, fp) != size) {
    arena_rewind(a, size + 1);
    fclose(fp);
    return NULL;
  }

  data[size] = '\n';

  fclose(fp);

  return (const char*) data;
}

#endif
