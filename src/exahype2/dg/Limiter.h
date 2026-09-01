// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg {
  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value mergeDiscreteMaximumPrincipleMinimum(const Value left, const Value right);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value mergeDiscreteMaximumPrincipleMaximum(const Value left, const Value right);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value quantiseObservableExtremum(const Value extremum);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD bool satisfiesDiscreteMaximumPrinciple(
    const Value candidateMinimum,
    const Value candidateMaximum,
    const Value referenceMinimum,
    const Value referenceMaximum,
    const Value relaxationParameter,
    const Value differencesScaling
  );

  template <int NumberOfObservables, typename Value, typename ObservableMapping>
  INLINE_METHOD GPU_CALLABLE_METHOD void reduceObservableExtrema(
    Value* const       minimum,
    Value* const       maximum,
    const Value* const nodalState,
    const int          numberOfNodes,
    const int          strideBetweenNodes,
    ObservableMapping  mapStateToObservables
  );

  template <
    int Dimensions,
    int InputDofs,
    int SampleDofs,
    int NumberOfData,
    int NumberOfObservables,
    typename Value,
    typename ObservableMapping>
  INLINE_METHOD GPU_CALLABLE_METHOD void reduceProjectedObservableExtrema(
    Value* const       minimum,
    Value* const       maximum,
    const Value* const nodalState,
    const Value* const samplingMatrix,
    ObservableMapping  mapStateToObservables
  );

  template <int Dimensions>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isCoarseVertexOnFacetOfHangingVertex(
    const tarch::la::Vector<Dimensions, int>& hangingVertexPosition,
    const int                                 coarseVertexNumber
  );
} // namespace exahype2::dg
TARGET_DECLARE_END

#include "exahype2/dg/Limiter.cpph"
