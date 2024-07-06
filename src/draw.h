#ifndef INCLUDE_DRAW_H
#define INCLUDE_DRAW_H

#include <limits.h>
#include <stddef.h>

#include <immintrin.h>

#include "third_party/stb/stb_image_write.h"

#define COMP_Y 1
#define COMP_YA 2
#define COMP_RGB 3
#define COMP_RGBA 4

#define DARK_GRAY 0xff181818
#define RED 0xff0000ff
#define BLUE 0xffff0000
#define GREEN 0xff00ff00
#define PURPLE 0xffff00ff

typedef unsigned int color;

typedef struct {
  color *pixels;
  int w;
  int h;
  int stride;
  color color;
} canvas;

typedef struct {
  int x;
  int y;
} Vector2;

typedef struct {
  double x;
  double y;
} Vector2d;

typedef struct {
  int x;
  int y;
  int w;
  int h;
} Rectangle;

int save_canvas(const char *filename, canvas canvas);
void draw_triangle(canvas canvas, color color, Vector2 p1, Vector2 p2,
                   Vector2 p3);
void draw_line(canvas canvas, color color, Vector2 p1, Vector2 p2);
void clear_canvas(canvas canvas, color color);
void draw_rectangle(canvas canvas, const Rectangle *rect);
#endif

#ifdef DRAW_IMPLEMENTATION

void draw_rectangle(canvas canvas, const Rectangle *rect) {
  size_t rem = rect->w % 8;

  __m256i color_group = _mm256_set1_epi32(canvas.color);
  for (size_t j = rect->y; j < rect->y+rect->h; ++j) {
    size_t offset = j * canvas.stride + rect->x;
    size_t scan_width = rect->w - rem;
    for (size_t i = 0; i < scan_width; i += 8){
      _mm256_storeu_si256((__m256i *)&canvas.pixels[offset + i], color_group);
    }
    offset += scan_width;
    for (size_t i = 0; i < rem; ++i){
      canvas.pixels[offset+i] = canvas.color;
    }
  }
}

void clear_canvas(canvas canvas, color color) {
  int num_pixels = canvas.w * canvas.h;

  // Broadcast the integer value across all lanes of the 256-bit register
  __m256i broadcasted_value = _mm256_set1_epi32(color);

  for (int x = 0; x < num_pixels; x += 8) {
    // Store the broadcasted value into the output array
    _mm256_storeu_si256((__m256i *)&canvas.pixels[x], broadcasted_value);
    // canvas.pixels[x] = color;
  }
}

Vector2d line_eq(Vector2 p1, Vector2 p2) {
  int start_x = p1.x < p2.x ? p1.x : p2.x;
  int end_x = p2.x == start_x ? p1.x : p2.x;

  int start_y = p1.x == start_x ? p1.y : p2.y;
  int end_y = p1.x == end_x ? p1.y : p2.y;

  int dx = end_x - start_x;
  int dy = end_y - start_y;

  if (dy == 0 && dx == 0)
    return (Vector2d){0};

  float m = (float)dy / (float)dx;
  float b = end_y - (end_x * m);
  return (Vector2d){
      .x = m,
      .y = b,
  };
}

void draw_line(canvas canvas, color color, Vector2 p1, Vector2 p2) {
  int start_x = p1.x < p2.x ? p1.x : p2.x;
  int end_x = p2.x == start_x ? p1.x : p2.x;

  int start_y = p1.x == start_x ? p1.y : p2.y;
  int end_y = p1.x == end_x ? p1.y : p2.y;

  int dx = end_x - start_x;
  int dy = end_y - start_y;

  if (dy == 0 && dx == 0)
    return;

  if (dx == 0) {
    int end = end_y >= start_y ? end_y : start_y;
    int start = end_y == end ? start_y : end_y;
    for (int y = start; y <= end; y++) {
      canvas.pixels[y * canvas.stride + start_x] = color;
    }
    return;
  }

  float m = (float)dy / (float)dx;
  float b = end_y - (end_x * m);

  if (abs(dx) >= abs(dy)) {
    for (int x = start_x; x <= end_x; x++) {
      int y = (int)(m * x + b);
      canvas.pixels[y * canvas.stride + x] = color;
    }
  } else {
    int end = end_y >= start_y ? end_y : start_y;
    int start = end_y == end ? start_y : end_y;
    for (int y = start; y <= end; y++) {
      int x = (int)((y - b) / m);
      canvas.pixels[y * canvas.stride + x] = color;
    }
  }
}

int lerp(int v0, int v1, float t) { return (1 - t) * v0 + t * v1; }

void draw_triangle(canvas canvas, color color, Vector2 p1, Vector2 p2,
                   Vector2 p3) {

  Vector2 *vecs[3] = {&p1, &p2, &p3};
  for (int x = 0; x < 3; x++) {
    Vector2 *c = vecs[x];
    if (c->y < vecs[0]->y) {
      Vector2 *tmp = vecs[0];
      vecs[0] = c;
      vecs[x] = tmp;
      x--;
    }
    if (c->y > vecs[2]->y) {
      Vector2 *tmp = vecs[2];
      vecs[2] = c;
      vecs[x] = tmp;
      x--;
    }
  }

  Vector2 min = *vecs[0];
  Vector2 med = *vecs[1];
  Vector2 max = *vecs[2];
  Vector2 mid = {
      lerp(min.x, max.x, .5),
      lerp(min.y, max.y, .5),
  };

  Vector2d l1 = line_eq(min, mid);
  Vector2d l2 = line_eq(min, med);

  for (int y = min.y + 1; y < mid.y; ++y) {
    int x1 = (y - l1.y) / l1.x;
    int x2 = (y - l2.y) / l2.x;
    draw_line(canvas, color,
              (Vector2){
                  .x = x1,
                  .y = y,
              },
              (Vector2){
                  .x = x2,
                  .y = y,
              });
  }

  l1 = line_eq(mid, max);
  l2 = line_eq(med, max);

  for (int y = mid.y; y < max.y; ++y) {
    int x1 = (y - l1.y) / l1.x;
    int x2 = (y - l2.y) / l2.x;
    draw_line(canvas, color,
              (Vector2){
                  .x = x1,
                  .y = y,
              },
              (Vector2){
                  .x = x2,
                  .y = y,
              });
  }

  draw_line(canvas, color, p1, p2);
  draw_line(canvas, color, p2, p3);
  draw_line(canvas, color, p3, p1);
}

int save_canvas(const char *filename, canvas canvas) {
  stbi_flip_vertically_on_write(1);
  return stbi_write_png(filename, canvas.w, canvas.h, COMP_RGBA, canvas.pixels,
                        sizeof(color) * canvas.stride);
}

#endif
