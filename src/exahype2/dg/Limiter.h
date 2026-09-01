// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg {
  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value mergeDiscreteMaximumPrincipleMinimum(Value left, Value right);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value mergeDiscreteMaximumPrincipleMaximum(Value left, Value right);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD Value quantiseObservableExtremum(Value extremum);

  template <typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD bool satisfiesDiscreteMaximumPrinciple(
    Value candidateMinimum,
    Value candidateMaximum,
    Value referenceMinimum,
    Value referenceMaximum,
    Value relaxationParameter,
    Value differencesScaling
  );

  template <int NumberOfObservables, typename Value, typename ObservableMapping>
  INLINE_METHOD GPU_CALLABLE_METHOD void reduceObservableExtrema(
    Value*            minimum,
    Value*            maximum,
    const Value*      nodalState,
    int               numberOfNodes,
    int               strideBetweenNodes,
    ObservableMapping mapStateToObservables
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
    Value*            minimum,
    Value*            maximum,
    const Value*      nodalState,
    const Value*      samplingMatrix,
    ObservableMapping mapStateToObservables
  );

  template <int Dimensions>
  INLINE_METHOD GPU_CALLABLE_METHOD bool isCoarseVertexOnFacetOfHangingVertex(
    const tarch::la::Vector<Dimensions, int>& hangingVertexPosition,
    int                                       coarseVertexNumber
  );
} // namespace exahype2::dg
TARGET_DECLARE_END

#include "exahype2/dg/Limiter.cpph"
