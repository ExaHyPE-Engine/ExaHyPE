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
    typename SolverType,
    int NumberOfDofsPerAxis,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void evaluateLinearFlux(
    ComputePrecision* NOALIAS                    flux,
    const ComputePrecision* NOALIAS              state,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  template <
    typename SolverType,
    int NumberOfDofsPerAxis,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void addNonconservativeProduct(
    ComputePrecision* NOALIAS                              flux,
    ComputePrecision* NOALIAS                              stateGradient,
    const ComputePrecision* NOALIAS                        state,
    const ComputePrecision* NOALIAS                        derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize,
    const tarch::la::Vector<DIMENSIONS, double>&           cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>&           cellSize,
    double                                                 timeStamp,
    double                                                 timeStepSize
  );

  template <
    typename SolverType,
    int NumberOfDofsPerAxis,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void applyMaterialParameterMatrix(
    ComputePrecision* NOALIAS                    flux,
    const ComputePrecision* NOALIAS              state,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );

  template <
    typename SolverType,
    int NumberOfDofsPerAxis,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void evaluateLinearSource(
    ComputePrecision* NOALIAS                    source,
    const ComputePrecision* NOALIAS              state,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );
} // namespace exahype2::dg::ader
TARGET_DECLARE_END

#include "exahype2/dg/ader/LinearPredictorTerms.cpph"
