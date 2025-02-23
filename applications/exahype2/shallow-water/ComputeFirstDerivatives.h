// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/utils/Loop.h"

#include "tarch/Assertions.h"
#include "tarch/la/Vector.h"
#include "tarch/NonCriticalAssertions.h"

namespace applications::exahype2::swe {
  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif

  /**
   * Partial derivative calculation:
   *
   *  Backward difference for single derivatives.
   *  Central difference for double derivatives.
   *
   *  Only numerator of the finite difference scheme is calculated here.
   *
   *  The division by the denominator takes place in the ncp function after
   *  retrieval from the average value.
   *
   *  For single derivatives, one can change
   *  the numerical scheme to forward, central or backward by using
   *  appropriate neighbour cell values defined above. For double
   *  derivatives, only central difference can be used with the variables
   *  defined above.
   */
  template <int NumberOfDofs, int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
  static inline GPUCallableMethod
  void computeFirstDerivatives(double* const __restrict__ oldQWithHalo) InlineMethod;

  template <int NumberOfDofs, int NumberOfUnknowns, int NumberOfAuxiliaryVariables>
  static inline GPUCallableMethod
  void extrapolateHalo(double* const __restrict__ oldQWithHalo) InlineMethod;

  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif
} // namespace applications::exahype2::swe

#include "ComputeFirstDerivatives.cpph"
