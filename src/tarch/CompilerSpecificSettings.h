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

#ifndef PEANO_WITH_ALIGNMENT
#error "PEANO_WITH_ALIGNMENT must be provided by the build system (CMake)"
#endif

#ifndef PEANO_WITH_SIMDLEN
#error "PEANO_WITH_SIMDLEN must be provided by the build system (CMake)"
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

#if defined(PEANO_WITH_OPENMP)
#define SIMD(...) _PRAGMA(omp simd __VA_ARGS__ simdlen(PEANO_WITH_SIMDLEN))

#else
// No OpenMP: Compiler-specific hints

#if defined(__INTEL_COMPILER) || defined(__INTEL_LLVM_COMPILER)
#define SIMD(...) _PRAGMA(ivdep)
#elif defined(__clang__)
#define SIMD(...) _PRAGMA(clang loop vectorize(enable) vectorize_width(PEANO_WITH_SIMDLEN))
#elif defined(__GNUC__)
#define SIMD(...) _PRAGMA(GCC ivdep)
#elif defined(_MSC_VER)
#define SIMD(...) __pragma(loop(ivdep))
#else
#define SIMD(...) /* no-op */
#endif

#endif

// Loop-unroll hint. Under nvcc the host pass rejects "#pragma unroll" (-Wunknown-pragmas) and the
// nvcc front-end rejects "#pragma GCC unroll" (#1675), so emit the device form only where a single
// compiler handles it (device pass, or clang/nvhpc/intel) and nothing in nvcc's host pass.
#if defined(__CUDA_ARCH__) || defined(__clang__) || defined(__INTEL_COMPILER) || defined(__INTEL_LLVM_COMPILER) \
  || defined(__NVCOMPILER) || defined(__PGI)
#define UNROLL(n) _PRAGMA(unroll n)
#elif defined(__CUDACC__)
#define UNROLL(n) /* no-op: nvcc host pass; neither unroll spelling satisfies front-end and host */
#elif defined(__GNUC__) && (__GNUC__ >= 8)
#define UNROLL(n) _PRAGMA(GCC unroll n)
#elif defined(_MSC_VER)
#define UNROLL(n) __pragma(loop(unroll(n)))
#else
#define UNROLL(n) /* no-op */
#endif
