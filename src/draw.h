#ifndef INCLUDE_DRAW_H
#define INCLUDE_DRAW_H

#include <limits.h>
#include <math.h>
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
#define CYAN 0xffffff00

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

int lerp(int v0, int v1, float t);

int save_canvas(const char *filename, canvas canvas);
void draw_triangle(canvas canvas, Vector2 p1, Vector2 p2, Vector2 p3);
void draw_line(canvas canvas, Vector2 p1, Vector2 p2);
void clear_canvas(canvas canvas, color color);
void draw_rectangle(canvas canvas, const Rectangle *rect);
#endif

#ifdef DRAW_IMPLEMENTATION

void draw_rectangle(canvas canvas, const Rectangle *rect) {
  size_t rem = rect->w % 8;

  __m256i color_group = _mm256_set1_epi32(canvas.color);
  for (size_t j = rect->y; j < rect->y + rect->h; ++j) {
    size_t offset = j * canvas.stride + rect->x;
    size_t scan_width = rect->w - rem;
    for (size_t i = 0; i < scan_width; i += 8) {
      _mm256_storeu_si256((__m256i *)&canvas.pixels[offset + i], color_group);
    }
    offset += scan_width;
    for (size_t i = 0; i < rem; ++i) {
      canvas.pixels[offset + i] = canvas.color;
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

static inline color brightness(color c, float alpha) {
  if (alpha >= 1.f) {
    return c;
  }
  float ab = 1.f - alpha;

  float bg_r = 24.f * alpha;
  float bg_g = 24.f * alpha;
  float bg_b = 24.f * alpha;

  int r = c & 0xff;
  int g = (c >> 8) & 0xff;
  int b = (c >> 16) & 0xff;
  // printf("a: %f, r: %02x, b: %02x, g: %02x -> ", alpha, r, g, b);

  float new_r = (ab * (float)r) + bg_r;
  float new_g = (ab * (float)g) + bg_g;
  float new_b = (ab * (float)b) + bg_b;

  r = (int)new_r;
  g = (int)new_g;
  b = (int)new_b;
  c = 0xff000000 | (b << 16) | (g << 8) | r;
  // printf("r: %02x, b: %02x, g: %02x = %08x\n", r, g, b, c);
  return c;
}

static inline void swap(int *a, int *b) {
  int tmp = *a;
  *a = *b;
  *b = tmp;
}

Vector2d line_eq(Vector2 p1, Vector2 p2) {

  if (p1.x > p2.x) {
    swap(&p1.x, &p2.x);
    swap(&p1.y, &p2.y);
  }

  int dx = p2.x - p1.x;
  int dy = p2.y - p1.y;

  if (dy == 0 && dx == 0)
    return (Vector2d){0};

  float m = 1.f;
  if (dx > 0)
    m = (float)dy / (float)dx;
  float b = (float)p2.y - (p2.x * m);

  return (Vector2d){
      .x = m,
      .y = b,
  };
}

float f_part(float a) {
  if (a > 0)
    return a - (int)a;
  return a - ((int)a + 1);
}

void draw_line(canvas canvas, Vector2 p1, Vector2 p2) {

  // printf("p1: %d, %d x p2: %d, %d\n", p1.x, p1.y, p2.x, p2.y);

  bool steep = (abs(p2.y - p1.y) - abs(p2.x - p1.x) > 0);
  if (steep) {
    swap(&p1.x, &p1.y);
    swap(&p2.x, &p2.y);
  }

  if (p1.x > p2.x) {
    swap(&p1.x, &p2.x);
    swap(&p1.y, &p2.y);
  }

  int dx = p2.x - p1.x;
  int dy = p2.y - p1.y;

  if (dy == 0 && dx == 0)
    return;

  // horizontal line
  if (dy == 0 && !steep) {
    for (int i = p1.x; i <= p2.x; ++i) {
      canvas.pixels[p1.y * canvas.stride + i] = canvas.color;
    }
    return;
  }

  // vertical line
  if (dy == 0 && steep) {
    for (int j = p1.x; j <= p2.x; ++j) {
      canvas.pixels[j * canvas.stride + p1.y] = canvas.color;
    }
    return;
  }

  float m = 1.f;
  if (dx > 0)
    m = (float)dy / dx;

  int x_start = p1.x;
  int x_end = p2.x;
  float y = p1.y;
  color c = canvas.color;

  // scan vertically
  if (steep) {
    for (int x = x_start; x <= x_end; ++x) {
      float fpart = f_part(y);
      float rpart = 1.f - fpart;

      color c1 = brightness(c, rpart);
      color c2 = brightness(c, fpart);

      canvas.pixels[x * canvas.stride + (size_t)y] = c1;
      canvas.pixels[x * canvas.stride + (size_t)y - 1] = c2;

      y += m;
    }
    return;
  }

  // scan horizontally
  for (int x = x_start; x <= x_end; ++x) {
    float fpart = f_part(y);
    float rpart = 1.f - fpart;

    color c1 = brightness(c, rpart);
    color c2 = brightness(c, fpart);

    canvas.pixels[(size_t)y * canvas.stride + x] = c1;
    canvas.pixels[((size_t)y - 1) * canvas.stride + x] = c2;

    y += m;
  }
}

int lerp(int v0, int v1, float t) { return (1 - t) * v0 + t * v1; }

void draw_triangle(canvas canvas, Vector2 p1, Vector2 p2, Vector2 p3) {

  draw_line(canvas, p1, p2);
  draw_line(canvas, p2, p3);
  draw_line(canvas, p3, p1);

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
    draw_line(canvas,
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
    draw_line(canvas,
              (Vector2){
                  .x = x1,
                  .y = y,
              },
              (Vector2){
                  .x = x2,
                  .y = y,
              });
  }

  // draw_line(canvas, p1, p2);
  // draw_line(canvas, p2, p3);
  // draw_line(canvas, p3, p1);
}

int save_canvas(const char *filename, canvas canvas) {
  stbi_flip_vertically_on_write(1);
  return stbi_write_png(filename, canvas.w, canvas.h, COMP_RGBA, canvas.pixels,
                        sizeof(color) * canvas.stride);
}

#endif
