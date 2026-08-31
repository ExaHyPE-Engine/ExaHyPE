// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementCriterionHelpers.h"

namespace {
  /**
   * Stand-in for a solver's scalar indicator.
   */
  struct FirstUnknownIndicator {
    template <typename Value>
    double operator()(const Value* volume) const {
      return static_cast<double>(volume[0]);
    }
  };

  /**
   * refineOnDetail() is defined in the .cpph, so nothing in the core library instantiates it.
   * Taking the address of a specialisation is an odr-use, so the compiler instantiates its
   * body along with the library.
   */
  [[maybe_unused]] void instantiateTemplatesForCompileCheck() {
    [[maybe_unused]] auto* refineOnDoublePatch = &exahype2::refineOnDetail<double, FirstUnknownIndicator>;
    [[maybe_unused]] auto* refineOnFloatPatch  = &exahype2::refineOnDetail<float, FirstUnknownIndicator>;
    [[maybe_unused]] auto* refineOnTwoIndicators =
      &exahype2::refineOnDetail<double, FirstUnknownIndicator, FirstUnknownIndicator>;
  }
} // namespace
