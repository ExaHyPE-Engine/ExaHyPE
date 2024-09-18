// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "VariableShortcuts.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::swe {
  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif
  static inline GPUCallableMethod
  double rusanov(
    const double* const __restrict__             QR,
    const double* const __restrict__             QL,
    const double* const __restrict__             FR,
    const double* const __restrict__             FL,
    const double* const __restrict__             LR,
    const double* const __restrict__             LL,
    const tarch::la::Vector<Dimensions, double>& xR,
    const tarch::la::Vector<Dimensions, double>& xL,
    const tarch::la::Vector<Dimensions, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    double* const __restrict__                   APDQ,
    double* const __restrict__                   AMDQ
  ) InlineMethod;
  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif
} // namespace applications::exahype2::swe

#include "Rusanov.cpph"
