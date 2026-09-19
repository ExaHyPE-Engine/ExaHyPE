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
  INLINE_METHOD GPU_CALLABLE_METHOD void initialiseLinearPredictorState(
    ComputePrecision* NOALIAS       state,
    const StoragePrecision* NOALIAS cellState
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void seedTimeAveragedState(
    ComputePrecision* NOALIAS       timeAveragedState,
    const ComputePrecision* NOALIAS state
  );

  template <
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateSource,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void clearTaylorTerms(
    ComputePrecision* NOALIAS flux,
    ComputePrecision* NOALIAS source
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void differentiateFluxInPlace(
    ComputePrecision* NOALIAS                              flux,
    const ComputePrecision* NOALIAS                        derivativeOperator,
    const tarch::la::Vector<DIMENSIONS, ComputePrecision>& inverseCellSize
  );

  template <
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateSource,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void accumulateTimeAveragedTerms(
    ComputePrecision* NOALIAS       timeAveragedFlux,
    ComputePrecision* NOALIAS       timeAveragedSource,
    const ComputePrecision* NOALIAS flux,
    const ComputePrecision* NOALIAS source,
    ComputePrecision                timeAverageFactor
  );

  template <
    int  NumberOfDofsPerAxis,
    int  NumberOfUnknowns,
    int  NumberOfAuxiliaryVariables,
    bool EvaluateSource,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void advanceTimeDerivative(
    ComputePrecision* NOALIAS       state,
    const ComputePrecision* NOALIAS flux,
    const ComputePrecision* NOALIAS source
  );

  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void accumulateTimeAveragedState(
    ComputePrecision* NOALIAS       timeAveragedState,
    const ComputePrecision* NOALIAS state,
    ComputePrecision                timeAverageFactor
  );
} // namespace exahype2::dg::ader
TARGET_DECLARE_END

#include "exahype2/dg/ader/CauchyKovalewski.cpph"
