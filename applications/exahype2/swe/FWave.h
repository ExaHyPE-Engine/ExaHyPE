// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace applications::exahype2::swe {
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
  double fwave(
    const double* __restrict__ QR,
    const double* __restrict__ QL,
    const double* __restrict__ FR,
    const double* __restrict__ FL,
    const double* __restrict__ LR,
    const double* __restrict__ LL,
    const ::tarch::la::Vector<Dimensions, double>& xR,
    const ::tarch::la::Vector<Dimensions, double>& xL,
    const ::tarch::la::Vector<Dimensions, double>& h,
    double                                         t,
    double                                         dt,
    int                                            normal,
    double* __restrict__ APDQ,
    double* __restrict__ AMDQ
  );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
} // namespace applications::exahype2::swe
