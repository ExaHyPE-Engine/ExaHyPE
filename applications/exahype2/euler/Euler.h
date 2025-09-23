// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "VariableShortcuts.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::euler {

  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif
  static GPUCallableInlineMethod
  void maxEigenvalue(
    const double* const __restrict__             Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    double* const __restrict__                   maxEigenvalue
  ) InlineMethod;
  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif

  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif
  static GPUCallableInlineMethod
  void flux(
    const double* const __restrict__             Q,
    const tarch::la::Vector<Dimensions, double>& x,
    const tarch::la::Vector<Dimensions, double>& h,
    const double                                 t,
    const double                                 dt,
    const int                                    normal,
    double* const __restrict__                   F
  ) InlineMethod;
  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif
} // namespace applications::exahype2::euler

#include "Euler.cpph"
