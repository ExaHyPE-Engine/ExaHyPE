// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Constants.h"
#include "FrictionLaw.h"
#include "VariableShortcuts.h"

#include "tarch/Assertions.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/la/Vector.h"

namespace applications::exahype2::landslide {
  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif
  static inline GPUCallableMethod
  double maxEigenvalue(
    const double* __restrict__                    Q,
    const tarch::la::Vector<Dimensions, double>&  x,
    const tarch::la::Vector<Dimensions, double>&  h,
    double                                        t,
    double                                        dt,
    int                                           normal
  ) InlineMethod;
  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif

  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif
  static inline GPUCallableMethod
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

  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif
  static inline GPUCallableMethod
  void nonconservativeProduct(
    const double* __restrict__                    Q,
    const double* __restrict__                    deltaQ,
    const tarch::la::Vector<Dimensions, double>&  x,
    const tarch::la::Vector<Dimensions, double>&  h,
    double                                        t,
    double                                        dt,
    int                                           normal,
    double* __restrict__                          BTimesDeltaQ
  ) InlineMethod;
  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif

  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif
  static inline GPUCallableMethod
  void nonconservativeProductWithElevation(
    const double* __restrict__                    Q,
    const double* __restrict__                    deltaQ,
    const tarch::la::Vector<Dimensions, double>&  x,
    const tarch::la::Vector<Dimensions, double>&  h,
    double                                        t,
    double                                        dt,
    int                                           normal,
    double* __restrict__                          BTimesDeltaQ
  ) InlineMethod;
  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif

  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif
  static inline GPUCallableMethod
  void sourceTerm(
    const double* __restrict__                    Q,
    const tarch::la::Vector<Dimensions, double>&  x,
    const tarch::la::Vector<Dimensions, double>&  h,
    double                                        t,
    double                                        dt,
    double* __restrict__                          S
  ) InlineMethod;
  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif

  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #endif
  static inline GPUCallableMethod
  void sourceTermWithCoulombFriction(
    const double* __restrict__                    Q,
    const tarch::la::Vector<Dimensions, double>&  x,
    const tarch::la::Vector<Dimensions, double>&  h,
    double                                        t,
    double                                        dt,
    double* __restrict__                          S
  ) InlineMethod;
  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif
} // namespace applications::exahype2::landslide

#include "Landslide.cpph"
