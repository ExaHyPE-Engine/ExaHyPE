// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#if not defined(__APPLE__)
#define COMPILER_HAS_SYSINFO
#define COMPILER_HAS_PROC_STAT
#endif

#if not defined(LITTLE_ENDIAN)
#define LITTLE_ENDIAN
#endif

#if not defined(PEANO_WITH_ALIGNMENT)
#error "Peano compiles inside its CMake build only."
#endif

#if not defined(PEANO_WITH_SIMDLEN)
#error "Peano compiles inside its CMake build only."
#endif

#if not defined(PEANO_ASSERTIONS)
#define PEANO_ASSERTIONS 0
#endif

#if not defined(PEANO_DEBUG)
#define PEANO_DEBUG 0
#endif

#if defined(_MSC_VER)
#define INLINE_METHOD __forceinline
#elif defined(__GNUC__) or defined(__clang__) or defined(__INTEL_COMPILER) or defined(__INTEL_LLVM_COMPILER) \
  or defined(__NVCOMPILER)
#define INLINE_METHOD inline __attribute__((always_inline))
#else
#define INLINE_METHOD inline
#endif

#define STR1(...) #__VA_ARGS__
#define STR(...) STR1(__VA_ARGS__)
#define _PRAGMA(...) _Pragma(STR(__VA_ARGS__))

// Clang compiles the floating-point operations of a build with PEANO_WITH_FPE as constrained
// intrinsics. LLVM cannot vectorise a constrained intrinsic. The spir64 device pass of icpx rejects
// the simdlen clause.
#if defined(PEANO_WITH_FPE) or defined(__SPIR__) or defined(__SPIRV__)
#define SIMD(...)
// #elif defined(__clang__) and not defined(PEANO_WITH_GPU_BACKEND)
// TODO: Is faster sometimes but Clang warns.
// #define SIMD(...) _PRAGMA(omp simd __VA_ARGS__ simdlen(PEANO_WITH_SIMDLEN)) _PRAGMA(unroll 1)
#else
#define SIMD(...) _PRAGMA(omp simd __VA_ARGS__ simdlen(PEANO_WITH_SIMDLEN))
#endif

#if defined(__CUDA_ARCH__) or defined(__clang__) or defined(__INTEL_COMPILER) or defined(__INTEL_LLVM_COMPILER) \
  or defined(__NVCOMPILER) or defined(__PGI)
#define UNROLL(numberOfIterations) _PRAGMA(unroll numberOfIterations)
#elif defined(__CUDACC__)
#define UNROLL(numberOfIterations)
#elif defined(__GNUC__) and (__GNUC__ >= 8)
#define UNROLL(numberOfIterations) _PRAGMA(GCC unroll numberOfIterations)
#elif defined(_MSC_VER)
#define UNROLL(numberOfIterations) __pragma(loop(unroll(numberOfIterations)))
#else
#define UNROLL(numberOfIterations)
#endif

#if defined(__has_attribute)
#if __has_attribute(force_align_arg_pointer)
#define ALIGN_STACK_ON_ENTRY __attribute__((force_align_arg_pointer))
#endif
#endif

#if not defined(ALIGN_STACK_ON_ENTRY)
#define ALIGN_STACK_ON_ENTRY
#endif
