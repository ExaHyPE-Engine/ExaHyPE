// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "Enumerator.h"

#include "exahype2/Solver.h"
#include "exahype2/VolumeIndex.h"
#include "exahype2/fv/PatchUtils.h"

#include "tarch/accelerator/accelerator.h"
#include "tarch/compiler/CompilerSpecificSettings.h"

namespace exahype2::fv::riemann {
  #if defined(SharedOMP)
  #pragma omp declare simd
  #endif
  template <
    class SolverType,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class QInEnumeratorType,
    class QOutEnumeratorType>
  static inline double updateSolutionWithRiemann_LoopBody(
    SolverType&                                   solver,
    const double* const __restrict__              QIn,
    const QInEnumeratorType&                      QInEnumerator,
    const tarch::la::Vector<Dimensions, double>&  patchCentre,
    const tarch::la::Vector<Dimensions, double>&  patchSize,
    const tarch::la::Vector<Dimensions, int>&     volume,
    const double                                  timeStamp,
    const double                                  timeStep,
    double* const __restrict__                    QOut,
    const QOutEnumeratorType&                     QOutEnumerator
  ) InlineMethod;

  /**
   * Specialisation invoking the static variant via SolverType (for GPUs).
   */
  #if defined(GPUOffloadingOMP)
  #pragma omp declare target
  #elif defined(SharedOMP)
  #pragma omp declare simd
  #endif
  template <
    class SolverType,
    bool EvaluateMaximumEigenvalueAfterTimeStep,
    class QInEnumeratorType,
    class QOutEnumeratorType>
  static inline GPUCallableMethod
  double updateSolutionWithRiemann_LoopBody(
    const double* const __restrict__              QIn,
    const QInEnumeratorType&                      QInEnumerator,
    const tarch::la::Vector<Dimensions, double>&  patchCentre,
    const tarch::la::Vector<Dimensions, double>&  patchSize,
    const tarch::la::Vector<Dimensions, int>&     volume,
    const double                                  timeStamp,
    const double                                  timeStep,
    double* const __restrict__                    QOut,
    const QOutEnumeratorType&                     QOutEnumerator
  ) InlineMethod;
  #if defined(GPUOffloadingOMP)
  #pragma omp end declare target
  #endif
} // namespace exahype2::fv::riemann

#include "LoopBody.cpph"
