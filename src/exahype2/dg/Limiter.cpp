// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "Limiter.h"

namespace {
  template <typename Value>
  struct ObservableMapping final {
    GPU_CALLABLE_METHOD void operator()(const Value* const state, Value* const observables) const {}
  };

  template <typename Value>
  void instantiateLimiterTemplates() {
    [[maybe_unused]] auto* mergeMinimum              = &exahype2::dg::mergeDiscreteMaximumPrincipleMinimum<Value>;
    [[maybe_unused]] auto* mergeMaximum              = &exahype2::dg::mergeDiscreteMaximumPrincipleMaximum<Value>;
    [[maybe_unused]] auto* satisfiesMaximumPrinciple = &exahype2::dg::satisfiesDiscreteMaximumPrinciple<Value>;
    [[maybe_unused]] auto* quantiseExtremum          = &exahype2::dg::quantiseObservableExtremum<Value>;
    [[maybe_unused]] auto* reduceObservables =
      &exahype2::dg::reduceObservableExtrema<2, Value, ObservableMapping<Value>>;
    [[maybe_unused]] auto* reduceProjectedObservables =
      &exahype2::dg::reduceProjectedObservableExtrema<DIMENSIONS, 2, 3, 2, 2, Value, ObservableMapping<Value>>;
  }

  [[maybe_unused]] void instantiateLimiterTemplates() {
    instantiateLimiterTemplates<float>();
    instantiateLimiterTemplates<double>();
    [[maybe_unused]] auto* hangingVertexFacet = &exahype2::dg::isCoarseVertexOnFacetOfHangingVertex<DIMENSIONS>;
  }
} // namespace
