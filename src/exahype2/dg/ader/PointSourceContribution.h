// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/ader/PredictorScratch.h"
#include "exahype2/dg/ader/SpaceTimeTerms.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg::ader {
  template <int NumberOfDofsPerAxis, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD double evaluateLagrangeBasis(
    const ComputePrecision* NOALIAS quadraturePoints,
    int                             index,
    double                          position
  );

  template <
    typename SolverType,
    int NumberOfDofsPerAxis,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void evaluatePointSources(
    ComputePrecision* NOALIAS                              pointSourceValues,
    const StoragePrecision* NOALIAS                        cellState,
    const int* NOALIAS                                     pointSourceIndices,
    int                                                    numberOfPointSources,
    const double* NOALIAS                                  pointSourceLocations,
    const ComputePrecision* NOALIAS                        quadraturePoints,
    const ComputePrecision* NOALIAS                        inverseCellQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    const tarch::la::Vector<DIMENSIONS, double>&           cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>&           cellSize,
    double                                                 timeStamp,
    double                                                 timeStepSize
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void addPointSourcesToPredictor(
    ComputePrecision* NOALIAS       state,
    const ComputePrecision* NOALIAS pointSourceValues,
    const ComputePrecision* NOALIAS quadratureWeights,
    double                          timeStepSize
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void addPointSourceDerivativeToState(
    ComputePrecision* NOALIAS       state,
    const ComputePrecision* NOALIAS pointSourceValues,
    const ComputePrecision* NOALIAS derivativeCoefficients
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void addPointSourcesToTimeAveragedFlux(
    ComputePrecision* NOALIAS       timeAveragedFlux,
    const ComputePrecision* NOALIAS pointSourceValues,
    const ComputePrecision* NOALIAS quadratureWeights
  );
} // namespace exahype2::dg::ader
TARGET_DECLARE_END

#include "exahype2/dg/ader/PointSourceContribution.cpph"
