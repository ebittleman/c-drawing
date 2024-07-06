#ifndef INCLUDE_GRAPHICS_H
#define INCLUDE_GRAPHICS_H

#include <stdio.h>
#include <time.h>

#include <immintrin.h>

#include "third_party/GLAD/gl.h"
#include <GLFW/glfw3.h>

#include "arena.h"
#include "io-utils.h"

typedef void *(*init_func)(int width, int height);
typedef void (*update_func)(void *ctx, int width, int height, double dt);
typedef float *(*vertex_provider)(void *ctx, size_t *num_elements);
typedef unsigned int *(*index_provider)(void *ctx, size_t *num_elements);

GLFWwindow *init_window(int w, int h);
GLuint init_texture(int width, int height);
GLuint init_framebuffer(GLuint texture);
GLuint init_shader(arena *a, const char *vert_shader, const char *frag_shader);
GLuint init_indexed_vertex_buffer(void *ctx, vertex_provider vertex_func,
                                  index_provider index_func);
void render_fb(GLuint fb, int width, int height, int img_width, int img_height);
void flip_image(unsigned int *image, int width, int height);
void render_texture(GLuint texture, int w, int h, void *pixels);
void *run(int width, int height, init_func init_func, update_func update_func);

#endif

#ifdef GRAPHICS_IMPLEMENTATION
static void key_callback(GLFWwindow *window, int key, int scancode, int action,
                         int mods) {
  (void)scancode;
  (void)action;
  (void)mods;

  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

GLFWwindow *init_window(int w, int h) {
  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow *window = glfwCreateWindow(w, h, "Drawing", NULL, NULL);

  if (!window) {
    glfwTerminate();
    exit(EXIT_FAILURE);
  }

  glfwSetKeyCallback(window, key_callback);
  glfwMakeContextCurrent(window);
  gladLoadGL(glfwGetProcAddress);
  glfwSwapInterval(1);

  return window;
}

void update_fps_counter(GLFWwindow *window, double current_seconds) {
  static double previous_seconds = 0.0;
  char tmp[128];

  static int frame_count;

  double elapsed_seconds = current_seconds - previous_seconds;
  if (elapsed_seconds > 0.25) {
    previous_seconds = current_seconds;

    float fps = (float)frame_count / elapsed_seconds;
    sprintf(tmp, "opengl @ fps: %.2f", fps);
    glfwSetWindowTitle(window, tmp);
    frame_count = 0;
  }
  frame_count++;
}

GLuint init_shader(arena *a, const char *vert_shader, const char *frag_shader) {
  int success;
  char infoLog[512];

  const char *vs = read_entire_file(a, vert_shader);
  if (vs == NULL) {
    fprintf(stderr, "Error reading %s:\n%d: %s\n", vert_shader, errno,
            strerror(errno));
    return 0;
  }
  const GLuint vertex_shader = glCreateShader(GL_VERTEX_SHADER);
  glShaderSource(vertex_shader, 1, &vs, NULL);
  glCompileShader(vertex_shader);

  glGetShaderiv(vertex_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(vertex_shader, 512, NULL, infoLog);
    fprintf(stderr, "ERROR::VERTEX_SHADER::COMPILATION_FAILED\n%s\n", infoLog);
    glfwTerminate();
  }

  const char *fs = read_entire_file(a, frag_shader);
  if (fs == NULL) {
    fprintf(stderr, "Error reading %s:\n%d: %s\n", frag_shader, errno,
            strerror(errno));
    return 0;
  }
  const GLuint fragment_shader = glCreateShader(GL_FRAGMENT_SHADER);
  glShaderSource(fragment_shader, 1, &fs, NULL);
  glCompileShader(fragment_shader);

  glGetShaderiv(fragment_shader, GL_COMPILE_STATUS, &success);
  if (!success) {
    glGetShaderInfoLog(fragment_shader, 512, NULL, infoLog);
    fprintf(stderr, "ERROR::FRAGEMENT_SHADER::COMPILATION_FAILED\n%s\n",
            infoLog);
    glfwTerminate();
  }

  const GLuint program = glCreateProgram();
  glAttachShader(program, vertex_shader);
  glAttachShader(program, fragment_shader);
  glLinkProgram(program);

  glDeleteShader(vertex_shader);
  glDeleteShader(fragment_shader);

  glGetProgramiv(program, GL_LINK_STATUS, &success);
  if (!success) {
    glGetProgramInfoLog(program, 512, NULL, infoLog);
    fprintf(stderr, "ERROR::SHADER::PROGRAM::LINKING_FAILED\n%s\n", infoLog);
    glfwTerminate();
    return 0;
  }

  return program;
}

GLuint init_indexed_vertex_buffer(void *ctx, vertex_provider vertex_func,
                                  index_provider index_func) {
  GLuint vbo = 0, ebo = 0;

  size_t num_verts, num_indices;
  float *vertices = vertex_func(ctx, &num_verts);
  unsigned int *indices = index_func(ctx, &num_indices);

  glGenBuffers(1, &vbo);
  glBindBuffer(GL_ARRAY_BUFFER, vbo);
  glBufferData(GL_ARRAY_BUFFER, sizeof(vertices) * num_verts, vertices,
               GL_STATIC_DRAW);

  glGenBuffers(1, &ebo);
  glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ebo);
  glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices) * num_indices, indices,
               GL_STATIC_DRAW);
  glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);
  glEnableVertexAttribArray(0);

  glBindBuffer(GL_ARRAY_BUFFER, 0);

  return vbo;
}

GLuint init_texture(int width, int height) {
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, width, height, 0, GL_RGBA,
               GL_UNSIGNED_BYTE, NULL);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA, width, height);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER,
                  GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}

GLuint init_framebuffer(GLuint texture) {
  GLuint fb = 0;
  glGenFramebuffers(1, &fb);
  glBindFramebuffer(GL_FRAMEBUFFER, fb);
  glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
                         texture, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    return 1;

  return fb;
}

void flip_image(unsigned int *image, int width, int height) {
  __m256i temp1, temp2;

  for (int row = 0; row < height / 2; ++row) {
    unsigned int *topRow = image + row * width;
    unsigned int *bottomRow = image + (height - row - 1) * width;

    // Process 8 elements at a time using AVX2 intrinsics
    for (int col = 0; col < width; col += 8) {
      // Load two 256-bit registers with top and bottom rows
      temp1 = _mm256_loadu_si256((__m256i *)(topRow + col));
      temp2 = _mm256_loadu_si256((__m256i *)(bottomRow + col));

      // Swap or copy elements using AVX2 instructions
      _mm256_storeu_si256((__m256i *)(topRow + col), temp2);
      _mm256_storeu_si256((__m256i *)(bottomRow + col), temp1);
    }
  }
}

void render_texture(GLuint texture, int w, int h, void *pixels) {
  flip_image(pixels, w, h);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h, GL_RGBA, GL_UNSIGNED_BYTE,
                  pixels);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void render_fb(GLuint fb, int width, int height, int img_width,
               int img_height) {
  glBindFramebuffer(GL_READ_FRAMEBUFFER, fb);
  glViewport(0, 0, width, height);
  glReadBuffer(GL_COLOR_ATTACHMENT0);

  glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
  glBlitFramebuffer(0, 0, img_width, img_height, 0, 0, width, height,
                    GL_COLOR_BUFFER_BIT, GL_LINEAR);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void *run(int width, int height, init_func init_func, update_func update_func) {
  GLFWwindow *window = init_window(width, height);

  void *ctx = init_func(width, height);

  double currentFrame = glfwGetTime();
  double lastFrame = currentFrame - 1e-3;
  double deltaTime;
  //  struct timespec rem = {0};

  while (!glfwWindowShouldClose(window)) {
    currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;

    /* if (deltaTime < .04167) { */
    /*   struct timespec req = {0, (.04167 - deltaTime) * 1e9}; */
    /*   nanosleep(&req, &rem); */
    /*   currentFrame = glfwGetTime(); */
    /*   deltaTime = currentFrame - lastFrame; */
    /* } */

    lastFrame = currentFrame;
    glfwGetFramebufferSize(window, &width, &height);
    glViewport(0, 0, width, height);
    update_fps_counter(window, currentFrame);

    update_func(ctx, width, height, deltaTime);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return ctx;
}

#endif
