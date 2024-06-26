int main2() {
  int size = 4; // Matrix size is fixed to 4x4
  mat4 A, B, C;

  initialize_matrix(A, size);
  initialize_matrix(B, size);

  printf("Matrix A:\n");
  print_matrix(A, size);
  printf("\nMatrix B:\n");
  print_matrix(B, size);

  matrix_multiply_4x4(C, A, B);

  printf("\nResultant Matrix C:\n");
  print_matrix(C, size);

  return 0;
}

int main3() {
  int size = 4; // Matrix size is fixed to 4x4
  /* mat4 A = { */
  /*   1, 0, 0, 3, */
  /*   0, 1, 0, 2, */
  /*   0, 0, 1, 4, */
  /*   0, 0, 0, 1 */
  /* }; */
  mat4 A = {
      1, 0, 0, 0, //
      0, 1, 0, 0, //
      0, 0, 1, 0, //
      3, 2, 4, 1, //
  };
  vec4 B = {1, 1, 1, 1};
  vec4 C;

  printf("Matrix A:\n");
  print_matrix(A, size);
  printf("\nMatrix B:\n");
  print_vec(B, size);

  // matmult_vec_4x4(A, B, C);
  matrix_multiply_1x4_4x4(B, A, C);

  printf("\nResultant Matrix C:\n");
  print_vec(C, size);

  return 0;
}

int main4() {
  vec4 r;
  __m128 a = _mm_setr_ps(1, 2, 3, 4);
  __m128 b = _mm_setr_ps(1, 2, 3, 4);
  __m128 result = _mm_dp_ps(a, b, 0xFF);
  _mm_storeu_ps(&r[0], result);

  print_vec(r, 4);

  return 0;
}

int main5() {
  arena a;
  void *data = init_arena(&a, 10 * 1024 * 1024);
  if (data == NULL) {
    fprintf(stderr, "Could not allocate arena\n");
    return 1;
  }

  const char *file = read_entire_file(&a, ".clangd");
  if (file == NULL) {
    fprintf(stderr, "Error reading file:\n%d: %s\n", errno, strerror(errno));
    return 1;
  }
  printf("%s\n", file);
  return 0;
}
