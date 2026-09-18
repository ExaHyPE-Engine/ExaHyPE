// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg {
  template <
    typename Solver,
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateNonconservativeProduct,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void solveRiemannProblem(
    StoragePrecision* NOALIAS                    leftFlux,
    StoragePrecision* NOALIAS                    rightFlux,
    const StoragePrecision* NOALIAS              leftState,
    const StoragePrecision* NOALIAS              rightState,
    const ComputePrecision* NOALIAS              faceQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction
  );

  template <
    typename Solver,
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateFlux,
    bool EvaluateNonconservativeProduct,
    bool EvaluateMaterialParameters,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void solveLinearRiemannProblem(
    StoragePrecision* NOALIAS                    leftFlux,
    StoragePrecision* NOALIAS                    rightFlux,
    const StoragePrecision* NOALIAS              leftState,
    const StoragePrecision* NOALIAS              rightState,
    const ComputePrecision* NOALIAS              faceQuadratureWeights,
    const tarch::la::Vector<DIMENSIONS, double>& faceCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize,
    int                                          direction
  );
} // namespace exahype2::dg
TARGET_DECLARE_END

#include "exahype2/dg/Riemann.cpph"
