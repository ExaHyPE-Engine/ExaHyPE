// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "EigenvalueStatistics.h"

namespace {
  // Taking these addresses makes the core library compile every supported precision pair.
  [[maybe_unused]] void instantiateEigenvalueStatisticsTemplates() {
    [[maybe_unused]] auto* floatFromFloat           = &exahype2::fv::accumulateMaxEigenvalue<float, float>;
    [[maybe_unused]] auto* floatFromDouble          = &exahype2::fv::accumulateMaxEigenvalue<float, double>;
    [[maybe_unused]] auto* doubleFromFloat          = &exahype2::fv::accumulateMaxEigenvalue<double, float>;
    [[maybe_unused]] auto* doubleFromDouble         = &exahype2::fv::accumulateMaxEigenvalue<double, double>;
    [[maybe_unused]] auto* longDoubleFromLongDouble = &exahype2::fv::accumulateMaxEigenvalue<long double, long double>;
  }
} // namespace
