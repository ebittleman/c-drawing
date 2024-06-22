#include <stdlib.h>

#define GRAPHICS_IMPLEMENTATION
#include "graphics.h"

#define GLAD_GL_IMPLEMENTATION
#include "gl.h"
#include <GLFW/glfw3.h>

#define DRAW_IMPLEMENTATION
#include "draw.h"

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"

#define CANVAS_WIDTH 1920
#define CANVAS_HEIGHT 1080

#define VELOCITY 120.0

Vector2d pos = {0.0, 0.0};
Vector2d vel = {VELOCITY, VELOCITY};

void animate(double dt, int bound_width, int bound_height,
	     int width, int height) {
  pos.x += vel.x * dt;
  pos.y += vel.y * dt;

  if (pos.x+width > bound_width) {
    vel.x = -VELOCITY;
    pos.x = bound_width - width - 1.0;
  }

  if (pos.y+height > bound_height) {
    vel.y = -VELOCITY;
    pos.y = bound_height - height - 1.0;
  }

  if (pos.x < 0) {
    vel.x = VELOCITY;
    pos.x = 1.0;
  }

  if (pos.y < 0) {
    vel.y = VELOCITY;
    pos.y = 1.0;
  }
}

void draw(canvas canvas, double dt) {

  clear_canvas(canvas, DARK_GRAY);

  Vector2 rect_size = {100, 100};
  animate(dt, canvas.w, canvas.h, rect_size.x, rect_size.y);
  draw_rectangle(canvas, RED, (int) pos.x, (int) pos.y,
		 rect_size.x, rect_size.y);

  draw_line(canvas, BLUE,  (Vector2){50, 50},   (Vector2){300, 333});
  draw_line(canvas, RED,   (Vector2){100, 400}, (Vector2){500, 400});
  draw_line(canvas, RED,   (Vector2){100, 400}, (Vector2){100, 600});
  draw_line(canvas, BLUE,  (Vector2){50, 50},   (Vector2){15, 333});
  draw_line(canvas, GREEN, (Vector2){300, 40},  (Vector2){600, 60});
  draw_line(canvas, BLUE,  (Vector2){300, 60},  (Vector2){600, 40});

  draw_triangle(canvas, GREEN,
		(Vector2){200, 200},
		(Vector2){150, 300},
		(Vector2){250, 250});
}

void render(int width, int height, canvas canvas, GLuint fb, GLuint texture) {
  glUseProgram(0);
  render_texture(texture, canvas.w, canvas.h, canvas.pixels);
  render_fb(fb, width, height, canvas.w, canvas.h);
}

typedef struct {
  canvas *canvas;
  GLuint fb;
  GLuint texture;
} Ctx;

void* init(int width, int height){
  color *pixels = malloc(sizeof(color) * width * height);
  canvas *_canvas = malloc(sizeof(canvas));
  Ctx *ctx = malloc(sizeof(Ctx));

  GLuint texture = init_texture(width, height);
  GLuint fb = init_framebuffer(texture);

  *_canvas = (canvas){
    .pixels = pixels,
    .w = width,
    .h = height,
    .stride = width,
  };

  *ctx = (Ctx){
    .canvas = _canvas,
    .fb = fb,
    .texture = texture,
  };

  return ctx;
};

void update(void *ctx, int width, int height, double dt){
  Ctx *_ctx = (Ctx*) ctx;

  canvas canvas = *_ctx->canvas;
  draw(canvas, dt);
  render(width, height, canvas, _ctx->fb, _ctx->texture);
}

int main(void) {
  Ctx *_ctx = run(CANVAS_WIDTH, CANVAS_HEIGHT, init, update);;
  char const *filename = "dist/canvas.png";
  save_canvas(filename, *_ctx->canvas);
  return 0;
}
