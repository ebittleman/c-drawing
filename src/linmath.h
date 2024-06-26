
#ifndef INCLUDE_LINMATH_H
#define INCLUDE_LINMATH_H
#include <immintrin.h>
#include <math.h>
#include <stdio.h>
#include <stdlib.h>

typedef float vec3[3];
typedef float vec4[4];
typedef float mat3[9];
typedef float mat4[16];

void print_matrix(float *matrix, int size);
void print_vec(float *vec, int size);
void identity_matrix(mat4 matrix);
void initialize_matrix(float *matrix, int size);
void translate(mat4 m, float x, float y, float z);
void rotate(mat4 A, mat4 B, mat4 C);
void matrix_multiply_4x4(mat4 R, const mat4 A, const mat4 B);
void matmult_vec_4x4(mat4 A, vec4 B, vec4 C);
void matrix_multiply_1x4_4x4(vec4 A, mat4 B, vec4 C);
void mat4x4_rotate_Z(mat4 Q, mat4 const M, float angle);
void mat4x4_ortho(mat4 M, float l, float r, float b, float t, float n, float f);

static inline void mat4x4_dup(mat4 M, mat4 const N) {
  __m256 first = _mm256_loadu_ps(&N[0]);
  __m256 second = _mm256_loadu_ps(&N[8]);
  _mm256_storeu_ps(&M[0], first);
  _mm256_storeu_ps(&M[8], second);
}

#endif

#ifdef LINMATH_IMPLEMENTATION

void matrix_multiply_4x4(mat4 R, const mat4 A, const mat4 B) {
  __m128 a_row1 = _mm_loadu_ps(&B[0]);
  __m128 a_row2 = _mm_loadu_ps(&B[4]);
  __m128 a_row3 = _mm_loadu_ps(&B[8]);
  __m128 a_row4 = _mm_loadu_ps(&B[12]);

  for (int i = 0; i < 4; i++) {
    __m128 b_col = _mm_setr_ps(A[i], A[i + 4], A[i + 8], A[i + 12]);

    __m128 c_val1 = _mm_dp_ps(a_row1, b_col, 0xFF);
    __m128 c_val2 = _mm_dp_ps(a_row2, b_col, 0xFF);
    __m128 c_val3 = _mm_dp_ps(a_row3, b_col, 0xFF);
    __m128 c_val4 = _mm_dp_ps(a_row4, b_col, 0xFF);

    R[i] = _mm_cvtss_f32(c_val1);
    R[i + 4] = _mm_cvtss_f32(c_val2);
    R[i + 8] = _mm_cvtss_f32(c_val3);
    R[i + 12] = _mm_cvtss_f32(c_val4);
  }
}

void matmult_vec_4x4(mat4 A, vec4 B, vec4 C) {
  // A is 4x4 matrix, B is 4x1 vector, C is the result 4x1 vector

  // Load the vector B into an AVX register
  __m128 vec = _mm_loadu_ps(B);

  for (int i = 0; i < 4; i++) {
    // Load the entire row of the matrix A[i, :]
    __m128 row = _mm_loadu_ps(&A[i * 4]);

    // Multiply row by vector
    __m128 result = _mm_mul_ps(row, vec);

    // Horizontally add the elements of the result to get the final dot product
    // for this row
    result = _mm_hadd_ps(result, result);
    result = _mm_hadd_ps(result, result);

    // Store the result in C[i]
    _mm_store_ss(&C[i], result);
  }
}

void matrix_multiply_1x4_4x4(vec4 A, mat4 B, vec4 C) {
  // Load the 1x4 row vector A
  __m128 a_row = _mm_loadu_ps(&A[0]);

  // Compute the resulting 1x4 row vector C
  for (int i = 0; i < 4; i++) {
    __m128 b_col = _mm_setr_ps(B[i], B[i + 4], B[i + 8], B[i + 12]);
    __m128 c_val = _mm_dp_ps(a_row, b_col, 0xF1);
    C[i] = _mm_cvtss_f32(c_val);
  }
}

const static mat4 _identity_matrix = {
    1, 0, 0, 0, //
    0, 1, 0, 0, //
    0, 0, 1, 0, //
    0, 0, 0, 1, //
};

void identity_matrix(mat4 matrix) { mat4x4_dup(matrix, _identity_matrix); }

void translate(mat4 m, float x, float y, float z) {
  identity_matrix(m);
  __m128 row = _mm_setr_ps(x, y, z, 1);
  _mm_storeu_ps(&m[12], row);
}

void mat4x4_rotate_Z(mat4 Q, mat4 const M, float angle) {
  float s = sinf(angle);
  float c = cosf(angle);

  mat4 R = {
      c,   s,   0.f, 0.f, //
      -s,  c,   0.f, 0.f, //
      0.f, 0.f, 1.f, 0.f, //
      0.f, 0.f, 0.f, 1.f  //
  };

  matrix_multiply_4x4(Q, M, R);
}

void mat4x4_ortho(mat4 M, float l, float r,
		  float b, float t, float n,
                  float f) {

  mat4 A = {
    2.f / (r - l), 0.f, 0.f, 0.f, //
    0.f, 2.f / (t - b), 0.f, 0.f, //
    0.f, 0.f, -2.f / (f - n), 0.f, //
    -(r + l) / (r - l),  -(t + b) / (t - b),  -(f + n) / (f - n), 1.f, //
  };

  mat4x4_dup(M, A);
}

void initialize_matrix(float *matrix, int size) {
  for (int i = 0; i < size * size; i++) {
    matrix[i] = rand() % 10;
  }
}

void print_matrix(float *matrix, int size) {
  for (int i = 0; i < size; i++) {
    for (int j = 0; j < size; j++) {
      printf("%f", matrix[i * size + j]);
      if (j < size - 1) {
        printf(" ");
      }
    }
    printf("\n");
  }
}

void print_vec(float *vec, int size) {
  for (int i = 0; i < size; i++) {
    printf("%f", vec[i]);
    if (i < size - 1) {
      printf(" ");
    }
  }
  printf("\n");
}
#endif
