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

int lerp(int v0, int v1, float t) { return (1 - t) * v0 + t * v1; }

void interpolate(float *ds, float i0, float d0, float i1, float d1) {
  if (i0 == i1) {
    ds[0] = d0;
    return;
  }
  float a = (d1 - d0) / (i1 - i0);
  float d = d0;
  for (int i = i0; i <= i1; ++i) {
    ds[i - (int)i0] = d;
    d = d + a;
  }
}

static inline color alpha_composite(color c, color bg, float alpha) {
  if (alpha >= 1.f) {
    return c;
  }
  float ab = 1.f - alpha;

  float bg_r = (float)(bg & 0xff) * alpha;
  float bg_g = (float)((bg >> 8) & 0xff) * alpha;
  float bg_b = (float)((bg >> 16) & 0xff) * alpha;

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

bool line_eq(Vector2 *a, Vector2 *b, float eq[2]) {

  Vector2 p1 = *a;
  Vector2 p2 = *b;

  bool steep = (abs(p2.y - p1.y) - abs(p2.x - p1.x)) > 0;
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

  if (dy == 0 && dx == 0) {
    eq[0] = 0;
    eq[1] = 0;
    return false;
  }

  eq[0] = 1.f;
  if (dx > 0)
    eq[0] = (float)dy / (float)dx;
  eq[1] = (float)p2.y - ((float)p2.x * eq[0]);

  *a = p1;
  *b = p2;
  return steep;
}

float f_part(float a) {
  if (a > 0)
    return a - (int)a;
  return a - ((int)a + 1);
}

void draw_line(canvas canvas, Vector2 p0, Vector2 p1) {
  if (abs(p1.x - p0.x) > abs(p1.y - p0.y)) {
    // Line is horizontal-ish
    // Make sure x0 < x1
    if (p0.x > p1.x) {
      swap(&p0.x, &p1.x);
      swap(&p0.y, &p1.y);
    }
    const int length = p1.x - p0.x + 1;
    float ys[length];
    interpolate(&ys[0], p0.x, p0.y, p1.x, p1.y);
    for (int x = p0.x; x <= p1.x; ++x) {
      float y = ys[x - p0.x];

      float fpart = f_part(y);
      float rpart = 1.f - fpart;

      size_t p1 = (size_t)y * canvas.stride + x;
      size_t p2 = ((size_t)y - 1) * canvas.stride + x;

      color c1 = alpha_composite(canvas.color, canvas.pixels[p1], rpart);
      color c2 = alpha_composite(canvas.color, canvas.pixels[p2], fpart);

      canvas.pixels[p1] = c1;
      canvas.pixels[p2] = c2;

      // canvas.pixels[(int) * canvas.stride + x] = canvas.color;
    }
  } else {
    // Line is vertical-ish
    // Make sure y0 < y1
    if (p0.y > p1.y) {
      swap(&p0.x, &p1.x);
      swap(&p0.y, &p1.y);
    }
    const int length = p1.y - p0.y + 1;
    float xs[length];
    interpolate(&xs[0], p0.y, p0.x, p1.y, p1.x);
    for (int y = p0.y; y <= p1.y; ++y) {
      float x = xs[y - p0.y];

      float fpart = f_part(x);
      float rpart = 1.f - fpart;

      size_t p1 = y * canvas.stride + (size_t)x;
      size_t p2 = y * canvas.stride + (size_t)x - 1;

      color c1 = alpha_composite(canvas.color, canvas.pixels[p1], rpart);
      color c2 = alpha_composite(canvas.color, canvas.pixels[p2], fpart);

      canvas.pixels[p1] = c1;
      canvas.pixels[p2] = c2;
    }
  }
}

void draw_triangle(canvas canvas, Vector2 p0, Vector2 p1, Vector2 p2) {
  if (p1.y < p0.y) {
    swap(&p1.x, &p0.x);
    swap(&p1.y, &p0.y);
  }
  if (p2.y < p0.y) {
    swap(&p2.x, &p0.x);
    swap(&p2.y, &p0.y);
  }
  if (p2.y < p1.y) {
    swap(&p2.x, &p1.x);
    swap(&p2.y, &p1.y);
  }

  const int length = p2.y - p0.y + 1;
  float xs1[length];
  float xs2[length];
  interpolate(&xs1[0], p0.y, p0.x, p1.y, p1.x);
  interpolate(&xs1[p1.y - p0.y], p1.y, p1.x, p2.y, p2.x);
  interpolate(&xs2[0], p0.y, p0.x, p2.y, p2.x);

  size_t m = length / 2;
  float *left = &xs1[0];
  float *right = &xs2[0];
  if (xs2[m] < xs1[m]) {
    left = &xs2[0];
    right = &xs1[0];
  }

  for (int y = p0.y; y <= p2.y; ++y) {
    int start = left[y - p0.y];
    int end = right[y - p0.y];
    int yoffset = y * canvas.stride;
    for (int x = start; x <= end; ++x) {
      canvas.pixels[yoffset + x] = canvas.color;
    }
  }

  //  canvas.color = BLUE;
  // draw_line(canvas, p0, p1);
  // canvas.color = GREEN;
  // draw_line(canvas, p1, p2);
  // canvas.color = RED;
  // draw_line(canvas, p0, p2);
}

int save_canvas(const char *filename, canvas canvas) {
  stbi_flip_vertically_on_write(1);
  return stbi_write_png(filename, canvas.w, canvas.h, COMP_RGBA, canvas.pixels,
                        sizeof(color) * canvas.stride);
}

#endif
