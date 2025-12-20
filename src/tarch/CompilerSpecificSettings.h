// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#define COMPILER_HAS_UTS_NAME
#define COMPILER_HAS_TIMESPEC

#if not defined(__APPLE__)
#define COMPILER_HAS_SYSINFO
#define COMPILER_HAS_PROC_STAT
#endif

#if not defined(LITTLE_ENDIAN)
#define LITTLE_ENDIAN
#endif

#ifndef ALIGNMENT
#error "ALIGNMENT must be provided by the build system (CMake)"
#endif

#ifndef SIMDLEN
#error "SIMDLEN must be provided by the build system (CMake)"
#endif

#if defined(__NVCOMPILER)
#define NOALIAS // nvc++ does seem to give incorrect results with the NOALIAS keyword in inlined functions
#define TASKLOOP_NOT_SUPPORTED
#define TASK_DEPENDENCIES_NOT_SUPPORTED
#else
#define NOALIAS __restrict__
#endif

#if defined(_MSC_VER)
#define INLINE_METHOD __forceinline
#elif defined(__GNUC__) or defined(__clang__) or defined(__INTEL_COMPILER) or defined(__INTEL_LLVM_COMPILER) \
  or defined(__NVCOMPILER)
#define INLINE_METHOD inline __attribute__((always_inline))
#else
#define INLINE_METHOD inline
#endif

#define STR1(x) #x
#define STR(x) STR1(x)
#define _PRAGMA(x) _Pragma(STR(x))

#if defined(WITH_OPENMP)
#define SIMD(...) _Pragma(STR(omp simd __VA_ARGS__ simdlen(SIMDLEN)))

#else

// No OpenMP SIMD: Tell compilers the loop is independent (ivdep)
#if defined(__INTEL_COMPILER) or defined(__INTEL_LLVM_COMPILER)
#define SIMD(...) _PRAGMA(ivdep)
#elif defined(__clang__)
#define SIMD(...) _PRAGMA(clang loop vectorize(enable) vectorize_width(SIMDLEN))
#elif defined(__GNUC__)
#define SIMD(...) _PRAGMA(GCC ivdep)
#elif defined(_MSC_VER)
#define SIMD(...) __pragma(loop(ivdep))
#else
#define SIMD(...) /* no-op */
#endif

#endif

#if defined(__clang__) or defined(__INTEL_COMPILER) or defined(__INTEL_LLVM_COMPILER) or defined(__NVCOMPILER) \
  or defined(__PGI) or defined(__CUDACC__)
#define UNROLL(n) _PRAGMA(unroll n)
#elif defined(__GNUC__) and (__GNUC__ >= 8)
#define UNROLL(n) _PRAGMA(GCC unroll n)
#elif defined(_MSC_VER)
#define UNROLL(n) __pragma(loop(unroll(n)))
#else
#define UNROLL(n) /* no-op */
#endif
