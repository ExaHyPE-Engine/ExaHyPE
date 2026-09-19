// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/OperatorAlongAxis.h"
#include "exahype2/dg/ader/PredictorScratch.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg::ader {
  template <
    int NumberOfDofsPerAxis,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void initialisePredictorState(
    ComputePrecision* NOALIAS       spaceTimeState,
    const StoragePrecision* NOALIAS cellState
  );

  template <
    int NumberOfDofsPerAxis,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void initialisePicardRightHandSide(
    ComputePrecision* NOALIAS       rightHandSide,
    const StoragePrecision* NOALIAS cellState,
    const ComputePrecision* NOALIAS cellQuadratureWeights,
    const ComputePrecision* NOALIAS leftFaceCoefficients,
    int                             timeNode
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void subtractFluxDivergence(
    ComputePrecision* NOALIAS                              rightHandSide,
    const ComputePrecision* NOALIAS                        spaceTimeFlux,
    const ComputePrecision* NOALIAS                        stiffnessOperatorTransposed,
    const ComputePrecision* NOALIAS                        cellQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    double                                                 timeStepSize,
    int                                                    timeNode
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD double solvePicardSystem(
    ComputePrecision* NOALIAS       spaceTimeState,
    ComputePrecision* NOALIAS       updatedTimeSlice,
    const ComputePrecision* NOALIAS rightHandSide,
    const ComputePrecision* NOALIAS inversePicardOperator,
    const ComputePrecision* NOALIAS inverseCellQuadratureWeights
  );
} // namespace exahype2::dg::ader
TARGET_DECLARE_END

#include "exahype2/dg/ader/PicardIteration.cpph"
