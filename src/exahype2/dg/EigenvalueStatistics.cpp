// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EigenvalueStatistics.h"

namespace {
  template <typename Value>
  struct EigenvalueEvaluator final {
    Value operator()(const int dimension) const { return Value{}; }
  };

  // Taking this address makes the core library compile the evaluator overload.
  template <typename Value>
  void instantiateEigenvalueStatisticsTemplates() {
    using Evaluator  = EigenvalueEvaluator<Value>;
    using Accumulate = void (exahype2::dg::MaxEigenvalueStatistics::*)(
      const tarch::la::Vector<DIMENSIONS, double>& cellSize,
      Evaluator&&                                  evaluateEigenvalue
    );
    [[maybe_unused]] Accumulate accumulate = &exahype2::dg::MaxEigenvalueStatistics::accumulate<DIMENSIONS, Evaluator>;
  }

  [[maybe_unused]] void instantiateEigenvalueStatisticsTemplates() {
    instantiateEigenvalueStatisticsTemplates<float>();
    instantiateEigenvalueStatisticsTemplates<double>();
    instantiateEigenvalueStatisticsTemplates<long double>();
  }
} // namespace
