void generated::kernels::AderDG::nonlinear::gemm_4_4_4_rhs_x(const double* A, const double* B, double* C) {
#ifdef __SSE3__
#ifdef __AVX__
#pragma message ("LIBXSMM KERNEL COMPILATION WARNING: compiling SSE3 code on AVX or newer architecture: " __FILE__)
#endif
#endif
  unsigned int l_m = 0;
  unsigned int l_n = 0;
  unsigned int l_k = 0;

  for ( l_n = 0; l_n < 4; l_n++ ) {
    for ( l_k = 0; l_k < 4; l_k++ ) {
      #pragma simd
      for ( l_m = 0; l_m < 4; l_m++ ) {
        C[(l_n*4)+l_m] += A[(l_k*4)+l_m] * B[(l_n*4)+l_k];
      }
    }
  }
#ifndef NDEBUG
#ifdef _OPENMP
#pragma omp atomic
#endif
libxsmm_num_total_flops += 128;
#endif
}

void generated::kernels::AderDG::nonlinear::gemm_4_4_4_rhs_y(const double* A, const double* B, double* C) {
#ifdef __SSE3__
#ifdef __AVX__
#pragma message ("LIBXSMM KERNEL COMPILATION WARNING: compiling SSE3 code on AVX or newer architecture: " __FILE__)
#endif
#endif
  unsigned int l_m = 0;
  unsigned int l_n = 0;
  unsigned int l_k = 0;

  for ( l_n = 0; l_n < 4; l_n++ ) {
    for ( l_k = 0; l_k < 4; l_k++ ) {
      #pragma simd
      for ( l_m = 0; l_m < 4; l_m++ ) {
        C[(l_n*16)+l_m] += A[(l_k*16)+l_m] * B[(l_n*4)+l_k];
      }
    }
  }
#ifndef NDEBUG
#ifdef _OPENMP
#pragma omp atomic
#endif
libxsmm_num_total_flops += 128;
#endif
}

void generated::kernels::AderDG::nonlinear::gemm_4_4_4_lduh_x(const double* A, const double* B, double* C) {
#ifdef __SSE3__
#ifdef __AVX__
#pragma message ("LIBXSMM KERNEL COMPILATION WARNING: compiling SSE3 code on AVX or newer architecture: " __FILE__)
#endif
#endif
  unsigned int l_m = 0;
  unsigned int l_n = 0;
  unsigned int l_k = 0;

  for ( l_n = 0; l_n < 4; l_n++ ) {
    for ( l_k = 0; l_k < 4; l_k++ ) {
      #pragma simd
      for ( l_m = 0; l_m < 4; l_m++ ) {
        C[(l_n*4)+l_m] += A[(l_k*4)+l_m] * B[(l_n*4)+l_k];
      }
    }
  }
#ifndef NDEBUG
#ifdef _OPENMP
#pragma omp atomic
#endif
libxsmm_num_total_flops += 128;
#endif
}

void generated::kernels::AderDG::nonlinear::gemm_4_4_4_lduh_y(const double* A, const double* B, double* C) {
#ifdef __SSE3__
#ifdef __AVX__
#pragma message ("LIBXSMM KERNEL COMPILATION WARNING: compiling SSE3 code on AVX or newer architecture: " __FILE__)
#endif
#endif
  unsigned int l_m = 0;
  unsigned int l_n = 0;
  unsigned int l_k = 0;

  for ( l_n = 0; l_n < 4; l_n++ ) {
    for ( l_k = 0; l_k < 4; l_k++ ) {
      #pragma simd
      for ( l_m = 0; l_m < 4; l_m++ ) {
        C[(l_n*16)+l_m] += A[(l_k*4)+l_m] * B[(l_n*4)+l_k];
      }
    }
  }
#ifndef NDEBUG
#ifdef _OPENMP
#pragma omp atomic
#endif
libxsmm_num_total_flops += 128;
#endif
}

void generated::kernels::AderDG::nonlinear::gemm_4_4_4_lqi(const double* A, const double* B, double* C) {
#ifdef __SSE3__
#ifdef __AVX__
#pragma message ("LIBXSMM KERNEL COMPILATION WARNING: compiling SSE3 code on AVX or newer architecture: " __FILE__)
#endif
#endif
  unsigned int l_m = 0;
  unsigned int l_n = 0;
  unsigned int l_k = 0;

  for ( l_n = 0; l_n < 4; l_n++ ) {
    for ( l_m = 0; l_m < 4; l_m++ ) { C[(l_n*4)+l_m] = 0.0; }

    for ( l_k = 0; l_k < 4; l_k++ ) {
      #pragma simd
      for ( l_m = 0; l_m < 4; l_m++ ) {
        C[(l_n*4)+l_m] += A[(l_k*64)+l_m] * B[(l_n*4)+l_k];
      }
    }
  }
#ifndef NDEBUG
#ifdef _OPENMP
#pragma omp atomic
#endif
libxsmm_num_total_flops += 128;
#endif
}

