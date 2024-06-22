#ifndef INCLUDE_GRAPHICS_H
#define INCLUDE_GRAPHICS_H

#include <immintrin.h> // AVX2 intrinsics header

#include "gl.h"
#include <GLFW/glfw3.h>

typedef void* (*init_func)(int width, int height);
typedef void (*update_func)(void *ctx, int width, int height, double dt);

void render_fb(GLuint fb, int width, int height,
	       int img_width, int img_height);
void render_texture(GLuint texture, int w, int h, void* pixels);
void flip_image(unsigned int* image, int width, int height);
GLuint init_framebuffer(GLuint texture);
GLuint init_texture(int width, int height);
GLFWwindow* init_window(int w, int h);
void* run(int width, int height, init_func init_func, update_func update_func);
#endif

#ifdef GRAPHICS_IMPLEMENTATION
static void key_callback(
  GLFWwindow* window, int key, int scancode, int action, int mods
) {
  (void) scancode;
  (void) action;
  (void) mods;

  if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
    glfwSetWindowShouldClose(window, GLFW_TRUE);
}

GLFWwindow* init_window(int w, int h) {
  if (!glfwInit())
    exit(EXIT_FAILURE);

  glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
  glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
  glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

  GLFWwindow* window = glfwCreateWindow(w, h, "Drawing", NULL, NULL);

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

GLuint init_texture(int width, int height) {
  unsigned int texture;
  glGenTextures(1, &texture);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexImage2D(
    GL_TEXTURE_2D, 0, GL_RGBA,
    width, height, 0, GL_RGBA, GL_UNSIGNED_BYTE,
    NULL);
  glTexStorage2D(GL_TEXTURE_2D, 1, GL_RGBA, width, height);

  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
  glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

  glBindTexture(GL_TEXTURE_2D, 0);
  return texture;
}

GLuint init_framebuffer(GLuint texture) {
  GLuint fb = 0;
  glGenFramebuffers(1, &fb);
  glBindFramebuffer(GL_FRAMEBUFFER, fb);
  glFramebufferTexture2D(
    GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D,
    texture, 0);
  glBindFramebuffer(GL_FRAMEBUFFER, 0);

  if(glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
    return 1;

  return fb;
}

void flip_image(unsigned int* image, int width, int height) {
    // Assuming AVX2 intrinsics for illustration
    __m256i temp1, temp2;

    for (int row = 0; row < height / 2; ++row) {
        unsigned int* topRow = image + row * width;
        unsigned int* bottomRow = image + (height - row - 1) * width;

        // Process 8 elements at a time using AVX2 intrinsics
        for (int col = 0; col < width; col += 8) { // Assuming 8 elements at a time (AVX2)
            // Load two 256-bit registers with top and bottom rows
            temp1 = _mm256_loadu_si256((__m256i*)(topRow + col));
            temp2 = _mm256_loadu_si256((__m256i*)(bottomRow + col));

            // Swap or copy elements using AVX2 instructions
            _mm256_storeu_si256((__m256i*)(topRow + col), temp2);
            _mm256_storeu_si256((__m256i*)(bottomRow + col), temp1);
        }
    }
}

void render_texture(GLuint texture, int w, int h, void* pixels) {
  flip_image(pixels, w, h);
  glBindTexture(GL_TEXTURE_2D, texture);
  glTexSubImage2D(GL_TEXTURE_2D, 0, 0, 0, w, h,
		  GL_RGBA, GL_UNSIGNED_BYTE, pixels);
  glBindTexture(GL_TEXTURE_2D, 0);
}

void render_fb(GLuint fb, int width, int height,
	       int img_width, int img_height) {
    glBindFramebuffer(GL_READ_FRAMEBUFFER, fb);
    glViewport(0, 0, width, height);
    glReadBuffer(GL_COLOR_ATTACHMENT0);

    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
    glBlitFramebuffer(0, 0, img_width, img_height,
                  0, 0, width, height,
                  GL_COLOR_BUFFER_BIT, GL_LINEAR);
    glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void* run(int width, int height, init_func init_func, update_func update_func){
  GLFWwindow* window = init_window(width, height);
  void *ctx = init_func(width, height);

  double currentFrame = glfwGetTime();
  double lastFrame = currentFrame - 1e-3;
  double deltaTime;

  while (!glfwWindowShouldClose(window)) {
    currentFrame = glfwGetTime();
    deltaTime = currentFrame - lastFrame;
    lastFrame = currentFrame;
    glfwGetFramebufferSize(window, &width, &height);

    update_func(ctx, width, height, deltaTime);

    glfwSwapBuffers(window);
    glfwPollEvents();
  }

  glfwDestroyWindow(window);
  glfwTerminate();

  return ctx;
}

#endif
