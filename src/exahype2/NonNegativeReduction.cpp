// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonNegativeReduction.h"

namespace {
  // Taking these addresses makes the core library compile every supported scalar type.
  template <typename Value>
  void instantiateReductionTemplates() {
    [[maybe_unused]] auto* minimum = &exahype2::NonNegativeReduction::minimum<Value>;
    [[maybe_unused]] auto* maximum = &exahype2::NonNegativeReduction::maximum<Value>;
    [[maybe_unused]] auto* sum     = &exahype2::NonNegativeReduction::sum<Value>;
  }

  [[maybe_unused]] void instantiateReductionTemplates() {
    instantiateReductionTemplates<float>();
    instantiateReductionTemplates<double>();
    instantiateReductionTemplates<long double>();
  }
} // namespace
