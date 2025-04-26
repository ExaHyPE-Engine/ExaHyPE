// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#define CompilerHasProcStat
#define CompilerHasUTSName
#define CompilerHasTimespec
#define CompilerHasSysinfo

#define LittleEndian

#if !defined(AlignmentOnHeap)
#define AlignmentOnHeap 16
#endif

#if defined(__NVCOMPILER)
#define NOALIAS // nvc++ does seem to give incorrect results with the __restrict__ keyword in inlined functions
#else
#define NOALIAS __restrict__
#endif
