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

#if !defined(ALIGNMENT_ON_HEAP)
#define ALIGNMENT_ON_HEAP 16
#endif

#if defined(__NVCOMPILER)
#define NOALIAS // nvc++ does seem to give incorrect results with the NOALIAS keyword in inlined functions
#define TASKLOOP_NOT_SUPPORTED
#define TASK_DEPENDENCIES_NOT_SUPPORTED
#else
#define NOALIAS __restrict__
#endif

#if defined(__GNUC__) || defined(__clang__)
#define INLINE_METHOD inline __attribute__((always_inline))
#else
#define INLINE_METHOD inline
#endif
