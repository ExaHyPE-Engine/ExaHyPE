// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace applications::exahype2::landslide {
#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
  double maxEigenvalue(
    const double* __restrict__ Q,
    const ::tarch::la::Vector<Dimensions, double>& x,
    const ::tarch::la::Vector<Dimensions, double>& h,
    double                                         t,
    double                                         dt,
    int                                            normal
  );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
  void flux(
    const double* __restrict__ Q,
    const ::tarch::la::Vector<Dimensions, double>& x,
    const ::tarch::la::Vector<Dimensions, double>& h,
    double                                         t,
    double                                         dt,
    int                                            normal,
    double* __restrict__ F
  );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
  void nonconservativeProduct(
    const double* __restrict__ Q,
    const double* __restrict__ deltaQ,
    const ::tarch::la::Vector<Dimensions, double>& x,
    const ::tarch::la::Vector<Dimensions, double>& h,
    double                                         t,
    double                                         dt,
    int                                            normal,
    double* __restrict__ BTimesDeltaQ
  );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif

#if defined(GPUOffloadingOMP)
#pragma omp declare target
#endif
  void sourceTerm(
    const double* __restrict__ Q,
    const ::tarch::la::Vector<Dimensions, double>& x,
    const ::tarch::la::Vector<Dimensions, double>& h,
    double                                         t,
    double                                         dt,
    double* __restrict__ S
  );
#if defined(GPUOffloadingOMP)
#pragma omp end declare target
#endif
} // namespace applications::exahype2::landslide
