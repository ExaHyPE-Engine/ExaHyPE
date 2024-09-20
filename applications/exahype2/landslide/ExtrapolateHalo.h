// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/utils/Loop.h"

#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"
#include "tarch/NonCriticalAssertions.h"

namespace applications::exahype2::landslide {
  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif
  template <int NumberOfDofs, int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
  static inline GPUCallableMethod
  void extrapolateHalo(double* const __restrict__ oldQWithHalo) InlineMethod;
  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif
} // namespace applications::exahype2::landslide

#include "ExtrapolateHalo.cpph"
