#include <errno.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define GRAPHICS_IMPLEMENTATION
#include "graphics.h"

#define IOUTILS_IMPLEMENTATION
#include "io-utils.h"

#define OBJECTS_IMPLEMENTATION
#include "objects.h"

#define ARENA_IMPLEMENTATION
#include "arena.h"

#define GLAD_GL_IMPLEMENTATION
#include "third_party/GLAD/gl.h"
#include <GLFW/glfw3.h>

#define DRAW_IMPLEMENTATION
#include "draw.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "third_party/stb/stb_image_write.h"

#define LINMATH_IMPLEMENTATION
#include "linmath.h"

#define ARENA_SIZE 104857600 // 10MB

#define CANVAS_FACTOR 120
#define CANVAS_WIDTH CANVAS_FACTOR * 16
#define CANVAS_HEIGHT CANVAS_FACTOR * 9

float randf(float min, float max) {
  // 0 to RAND_MAX
  float num = rand() / (float)RAND_MAX;
  return lerp(min, max, num);
}

void animate(objid id, double dt, Rectangle *rect, int bound_width,
             int bound_height) {

  // calc new position
  float values[15] = {0};
  calc_next_pos(id, dt, values);

  float new_x = values[9];
  float new_y = values[10];

  // query collision
  bool x_collision = (new_x < 0 || new_x + values[12] > bound_width);
  bool y_collision = (new_y < 0 || new_y + values[13] > bound_height);

  // assign position and velocity based on collision
  if (x_collision) {
    values[3] = -values[3];
    values[9] = values[6];
  }

  if (y_collision) {
    values[4] = -values[4];
    values[10] = values[7];
  }

  // commit updates
  update_velocity(id, &values[3]);
  update_position(id, &values[9]);

  // assign position to rectangle
  rect->x = floor(values[9]);
  rect->y = floor(values[10]);
  rect->w = floor(values[12]);
  rect->h = floor(values[13]);
}

void draw(canvas g, objid num_items, double dt) {

  clear_canvas(g, DARK_GRAY);
  Rectangle rect = {0};

  for (objid x = 0; x < num_items; x++) {
    animate(x, dt, &rect, g.w, g.h);
    g.color = RED;
    draw_rectangle(g, &rect);
  }

  g.color = BLUE;
  draw_line(g, (Vector2){50, 50}, (Vector2){300, 333});
  g.color = RED;
  draw_line(g, (Vector2){100, 400}, (Vector2){500, 400});
  draw_line(g, (Vector2){100, 400}, (Vector2){100, 600});
  g.color = BLUE;
  draw_line(g, (Vector2){50, 50}, (Vector2){15, 333});
  g.color = GREEN;
  draw_line(g, (Vector2){300, 40}, (Vector2){600, 60});
  g.color = BLUE;
  draw_line(g, (Vector2){300, 60}, (Vector2){600, 40});

  g.color = GREEN;
  draw_triangle(g, (Vector2){200, 200}, (Vector2){150, 300},
                (Vector2){250, 250});
}

typedef struct {
  arena *arena;
  canvas *g;
  GLuint fb;
  GLuint texture;
  GLuint vao;
  GLuint vbo;
  GLuint shader;
  GLint mvp_location;
  objid num_items;
} Ctx;

float *get_verts(void *ctx, size_t *num_elements) {
  (void)ctx;
  static float vertices[] = {
      0.5f,  0.5f,  0.0f, // top right
      0.5f,  -0.5f, 0.0f, // bottom right
      -0.5f, -0.5f, 0.0f, // bottom left
      -0.5f, 0.5f,  0.0f  // top left
  };
  *num_elements = sizeof(vertices) / sizeof(vertices[0]);
  return vertices;
}

unsigned int *get_indices(void *ctx, size_t *num_elements) {
  (void)ctx;
  static unsigned int indices[] = {
      // note that we start from 0!
      0, 1, 3, // first Triangle
      1, 2, 3  // second Triangle
  };
  *num_elements = sizeof(indices) / sizeof(indices[0]);
  return indices;
}

void *init(int width, int height) {
  arena *_arena = malloc(sizeof(arena));
  if (init_arena(_arena, ARENA_SIZE) == NULL) {
    fprintf(stderr, "Error allocating arena\n");
    exit(EXIT_FAILURE);
    return NULL;
  }

  init_motion_tables(_arena, 400000);

  objid num_items = 0;
  for (int x = 0; x < 400000; x++) {
    float size = randf(1.f, 3.f);
    num_items = new_object((float[12]){
        randf(-500.f, 500.f), randf(-500.f, 500.f), 0.f, //
        randf(-250.f, 250.f), randf(-250.f, 250.f), 0.f, //
        randf(0.f, CANVAS_WIDTH - size), randf(0.f, CANVAS_HEIGHT - size),
        0.f,             //
        size, size, 0.f, //
    });
  }

  Ctx *ctx = arena_alloc(_arena, sizeof(Ctx));

  color *pixels = arena_alloc(_arena, sizeof(color) * width * height);
  canvas *g = arena_alloc(_arena, sizeof(canvas));

  GLuint vao = 0, vbo = 0, texture = 0, fb = 0, program;

  texture = init_texture(width, height);
  fb = init_framebuffer(texture);

  program = init_shader(_arena, "assets/shaders/tutorial1/vertex.glsl",
                        "assets/shaders/tutorial1/frag.glsl");
  if (program == 0) {
    exit(EXIT_FAILURE);
  }

  const GLint mvp_location = glGetUniformLocation(program, "mvp");

  glGenVertexArrays(1, &vao);
  glBindVertexArray(vao);

  vbo = init_indexed_vertex_buffer(ctx, get_verts, get_indices);

  glBindVertexArray(0);

  *g = (canvas){
      .pixels = pixels,
      .w = width,
      .h = height,
      .stride = width,
  };

  *ctx = (Ctx){
      .arena = _arena,
      .g = g,
      .fb = fb,
      .texture = texture,
      .vao = vao,
      .vbo = vbo,
      .shader = program,
      .mvp_location = mvp_location,
      .num_items = num_items,
  };

  return ctx;
};

void render(Ctx *ctx, int width, int height) {
  glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
  glClear(GL_COLOR_BUFFER_BIT);

  glUseProgram(0);
  render_texture(ctx->texture, ctx->g->w, ctx->g->h, ctx->g->pixels);
  render_fb(ctx->fb, width, height, ctx->g->w, ctx->g->h);

  const float ratio = width / (float)height;
  mat4 m, p, mvp;
  identity_matrix(m);
  mat4x4_rotate_Z(m, m, (float)glfwGetTime());
  mat4x4_ortho(p, -ratio, ratio, -1.f, 1.f, 1.f, -1.f);
  matrix_multiply_4x4(mvp, p, m);

  glUseProgram(ctx->shader);
  glUniformMatrix4fv(ctx->mvp_location, 1, GL_FALSE, (const GLfloat *)&mvp);
  glBindVertexArray(ctx->vao);
  glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
  glBindVertexArray(0);
}

void update(void *ctx, int width, int height, double dt) {
  Ctx *_ctx = (Ctx *)ctx;

  draw(*_ctx->g, _ctx->num_items, dt);
  render(_ctx, width, height);
}

int main(void) {
  Ctx *_ctx = run(CANVAS_WIDTH, CANVAS_HEIGHT, init, update);

  char const *filename = "dist/canvas.png";
  save_canvas(filename, *_ctx->g);

  arena *a = _ctx->arena;
  arena_free(a);
  free(a);
  return 0;
}

int main1(void) {
  mat4 dest, b;
  vec4 src = {1.0, 2.0, 3.0, 0.f};
  mat4_from_vec4_mul_outer(b, src, src);

  identity_matrix(dest);
  mat4_scale(dest, dest, 2.0);

  vec4 r1, r2;
  matmult_vec_4x4(dest, src, r1);
  matrix_multiply_1x4_4x4(src, dest, r2);

  print_vec(src, 4);
  printf("\n");
  print_matrix(b, 4);
  printf("\n");
  print_matrix(dest, 4);
  printf("\nr1: ");
  print_vec(r1, 4);
  printf("\nr2: ");
  print_vec(r2, 4);
  return 0;
}
