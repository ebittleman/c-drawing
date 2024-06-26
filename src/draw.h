#ifndef INCLUDE_DRAW_H
#define INCLUDE_DRAW_H

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

typedef unsigned int color;

typedef struct {
  color *pixels;
  int w;
  int h;
  int stride;
} canvas;

typedef struct {
  int x;
  int y;
} Vector2;

typedef struct {
  double x;
  double y;
} Vector2d;

int save_canvas(const char *filename, canvas canvas);
void draw_triangle();
void draw_line();
void clear_canvas(canvas canvas, color color);
void draw_rectangle();
#endif

#ifdef DRAW_IMPLEMENTATION
void draw_rectangle(canvas canvas, color color, int x, int y, int w, int h) {
  for (int j = y; j < y + h; j++) {
    for (int i = x; i < x + w; i++) {
      canvas.pixels[j * canvas.stride + i] = color;
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

  float c = (float)dy / (float)dx;
  float k = end_y - (end_x * c);

  if (abs(dx) >= abs(dy)) {
    for (int x = start_x; x <= end_x; x++) {
      int y = (int)(c * x + k);
      canvas.pixels[y * canvas.stride + x] = color;
    }
  } else {
    int end = end_y >= start_y ? end_y : start_y;
    int start = end_y == end ? start_y : end_y;
    for (int y = start; y <= end; y++) {
      int x = (int)((y - k) / c);
      canvas.pixels[y * canvas.stride + x] = color;
    }
  }
}

void draw_triangle(canvas canvas, color color, Vector2 p1, Vector2 p2,
                   Vector2 p3) {
  draw_line(canvas, color, p1, p2);
  draw_line(canvas, color, p1, p3);
  draw_line(canvas, color, p2, p3);
}

int save_canvas(const char *filename, canvas canvas) {
  stbi_flip_vertically_on_write(1);
  return stbi_write_png(filename, canvas.w, canvas.h, COMP_RGBA, canvas.pixels,
                        sizeof(color) * canvas.stride);
}

#endif
