// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/Limiter.h"
#include "peano4/Constants.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

TARGET_DECLARE
namespace exahype2::dg::limiting {
  template <typename SolverType, bool UsesRelaxationTerm, typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value relaxationParameterOfObservable(int observable, Value relaxationParameter);

  template <
    int NumberOfObservables,
    int NumberOfDofsPerAxis,
    int NumberOfSubcellsPerAxis,
    int NumberOfData,
    typename Value,
    typename ObservableMapping>
  INLINE_METHOD GPU_CALLABLE_METHOD void reduceSampledObservableExtrema(
    Value*            minimum,
    Value*            maximum,
    const Value*      nodalState,
    const Value*      dgToLobattoMatrix,
    const Value*      dgToSubcellMatrix,
    ObservableMapping mapStateToObservables
  );

  template <typename SolverType, int NumberOfObservables, bool UsesRelaxationTerm, typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD bool satisfiesRelaxedMaximumPrinciple(
    const Value* candidateMinimum,
    const Value* candidateMaximum,
    const Value* referenceMinimum,
    const Value* referenceMaximum,
    Value        relaxationParameter,
    Value        differencesScaling
  );
} // namespace exahype2::dg::limiting
TARGET_DECLARE_END

#include "exahype2/dg/limiting/Admissibility.cpph"
