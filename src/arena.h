#ifndef INCLUDE_ARENA_H
#define INCLUDE_ARENA_H

#include <stddef.h>
#include <stdint.h>

#ifndef ARENA_MALLOC
#include <stdlib.h>
#define ARENA_MALLOC(size) malloc((size))
#endif

#ifndef ARENA_REALLOC
#include <stdlib.h>
#define ARENA_REALLOC(ptr, size) realloc((ptr), (size))
#endif

#define WORD_SIZE sizeof(intptr_t)

typedef struct {
  void *data;
  size_t size;
  size_t capacity;
} arena;

void *init_arena(arena *a, size_t s);
void *arena_alloc(arena *a, size_t s);
void arena_rewind(arena *a, size_t s);
void arena_free(arena *a);

#endif

#ifdef ARENA_IMPLEMENTATION

void *init_arena(arena *a, size_t s) {
  size_t aligned_size = (s + WORD_SIZE - 1) & ~(WORD_SIZE - 1);
  void *data = ARENA_MALLOC(aligned_size);
  *a = (arena){
      .data = data,
      .size = 0,
      .capacity = s,
  };
  return data;
}

void *arena_alloc(arena *a, size_t s) {
  size_t aligned_size = (s + WORD_SIZE - 1) & ~(WORD_SIZE - 1);
  size_t new_size = aligned_size + a->size;

  if (new_size >= a->capacity) {
    size_t new_capacity = a->capacity * 2;
    new_capacity = (new_capacity + WORD_SIZE - 1) & ~(WORD_SIZE - 1);
    void *new_data = ARENA_REALLOC(a->data, new_capacity);
    if (new_data == NULL) {
      return NULL;
    }
    a->capacity = new_capacity;
    a->data = new_data;
  }

  void *new_data = &((char *)a->data)[a->size];
  a->size = new_size;
  return new_data;
}

void arena_rewind(arena *a, size_t s) { a->size = s; }

void arena_free(arena *a) { free(a->data); }

#endif
