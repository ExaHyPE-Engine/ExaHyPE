// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#define COMPILER_HAS_UTS_NAME
#define COMPILER_HAS_TIMESPEC

#if !defined(__APPLE__)
#define COMPILER_HAS_SYSINFO
#define COMPILER_HAS_PROC_STAT
#endif

#if !defined(LITTLE_ENDIAN)
#define LITTLE_ENDIAN
#endif

#ifndef ALIGNMENT
#error "ALIGNMENT must be provided by the build system (CMake)"
#endif

#ifndef SIMDLEN
#error "SIMDLEN must be provided by the build system (CMake)"
#endif

#ifndef PEANO_TRACE
#define PEANO_TRACE 0
#endif

#ifndef PEANO_ASSERTIONS
#define PEANO_ASSERTIONS 0
#endif

#ifndef PEANO_DEBUG
#define PEANO_DEBUG 0
#endif

#if defined(__NVCOMPILER)
#define TASKLOOP_NOT_SUPPORTED
#define TASK_DEPENDENCIES_NOT_SUPPORTED
#endif

#define NOALIAS __restrict__

#if defined(_MSC_VER)
#define INLINE_METHOD __forceinline
#elif defined(__GNUC__) || defined(__clang__) || defined(__INTEL_COMPILER) || defined(__INTEL_LLVM_COMPILER) \
  || defined(__NVCOMPILER)
#define INLINE_METHOD inline __attribute__((always_inline))
#else
#define INLINE_METHOD inline
#endif

#define STR1(...) #__VA_ARGS__
#define STR(...) STR1(__VA_ARGS__)
#define _PRAGMA(...) _Pragma(STR(__VA_ARGS__))

#if defined(WITH_OPENMP)
#define SIMD(...) _PRAGMA(omp simd __VA_ARGS__ simdlen(SIMDLEN))

#else
// No OpenMP: Compiler-specific hints

#if defined(__INTEL_COMPILER) || defined(__INTEL_LLVM_COMPILER)
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

#if defined(__clang__) || defined(__INTEL_COMPILER) || defined(__INTEL_LLVM_COMPILER) || defined(__NVCOMPILER) \
  || defined(__PGI) || defined(__CUDACC__)
#define UNROLL(n) _PRAGMA(unroll n)
#elif defined(__GNUC__) && (__GNUC__ >= 8)
#define UNROLL(n) _PRAGMA(GCC unroll n)
#elif defined(_MSC_VER)
#define UNROLL(n) __pragma(loop(unroll(n)))
#else
#define UNROLL(n) /* no-op */
#endif
