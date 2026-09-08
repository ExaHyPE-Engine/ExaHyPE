// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "MultiresolutionDetail.h"

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
   * The detail routine is defined in the .cpph, so nothing in the core library instantiates
   * it and a mistake in its body would first surface when a user's solver calls it. Naming
   * the specialisations here compiles them along with the library. Taking their address is
   * enough: it is an odr-use, so the compiler has to instantiate the body.
   */
  [[maybe_unused]] void instantiateTemplatesForCompileCheck() {
    [[maybe_unused]] auto* detailOfDoublePatch =
      &exahype2::amr::computeMultiresolutionDetail<double, FirstUnknownIndicator>;
    [[maybe_unused]] auto* detailOfFloatPatch =
      &exahype2::amr::computeMultiresolutionDetail<float, FirstUnknownIndicator>;
  }
} // namespace
