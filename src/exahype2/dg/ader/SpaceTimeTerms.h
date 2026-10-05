// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/DGUtils.h"
#include "exahype2/dg/OperatorAlongAxis.h"
#include "exahype2/dg/ader/PredictorScratch.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg::ader {
  template <int NumberOfDofsPerAxis>
  INLINE_METHOD GPU_CALLABLE_METHOD tarch::la::Vector<DIMENSIONS, int> spatialNodeIndex(int cellNode);

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void computeSpaceTimeGradient(
    ComputePrecision* NOALIAS                              stateGradient,
    const ComputePrecision* NOALIAS                        spaceTimeState,
    const ComputePrecision* NOALIAS                        derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    int                                                    timeNode
  );

  template <
    typename SolverType,
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateDiffusiveFlux,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void evaluateSpaceTimeFlux(
    ComputePrecision* NOALIAS                    flux,
    ComputePrecision* NOALIAS                    timeAveragedFlux,
    const ComputePrecision* NOALIAS              spaceTimeState,
    const ComputePrecision* NOALIAS              stateGradient,
    const ComputePrecision* NOALIAS              quadraturePoints,
    const ComputePrecision* NOALIAS              quadratureWeights,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          timeNode
  );

  template <
    typename SolverType,
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateNonconservativeProduct,
    bool EvaluateSource,
    bool EvaluateDiffusiveSource,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void evaluateSpaceTimeSource(
    ComputePrecision* NOALIAS                    rightHandSide,
    ComputePrecision* NOALIAS                    timeAveragedSource,
    const ComputePrecision* NOALIAS              spaceTimeState,
    const ComputePrecision* NOALIAS              stateGradient,
    const ComputePrecision* NOALIAS              quadraturePoints,
    const ComputePrecision* NOALIAS              quadratureWeights,
    const ComputePrecision* NOALIAS              cellQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          timeNode
  );
} // namespace exahype2::dg::ader
TARGET_DECLARE_END

#include "exahype2/dg/ader/SpaceTimeTerms.cpph"
