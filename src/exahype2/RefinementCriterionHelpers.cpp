// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "RefinementCriterionHelpers.h"

namespace {
  /**
   * Stand-in for a solver's scalar indicator. The parameter is generic, so naming this type
   * once covers a patch held in double and one held in reduced precision.
   */
  struct FirstUnknownIndicator {
    template <typename Value>
    double operator()(const Value* volume) const {
      return static_cast<double>(volume[0]);
    }
  };

  /**
   * refineOnDetail() is defined in the .cpph, so nothing in the core library instantiates it
   * and a mistake in its body would first surface when a user's solver calls it. Naming the
   * specialisations here compiles them along with the library. Taking their address is
   * enough: it is an odr-use, so the compiler has to instantiate the body. The two-indicator
   * specialisation covers the fold over the parameter pack, which a single indicator leaves
   * untested.
   */
  [[maybe_unused]] void instantiateTemplatesForCompileCheck() {
    [[maybe_unused]] auto* refineOnDoublePatch = &exahype2::refineOnDetail<double, FirstUnknownIndicator>;
    [[maybe_unused]] auto* refineOnFloatPatch  = &exahype2::refineOnDetail<float, FirstUnknownIndicator>;
    [[maybe_unused]] auto* refineOnTwoIndicators =
      &exahype2::refineOnDetail<double, FirstUnknownIndicator, FirstUnknownIndicator>;
  }
} // namespace
