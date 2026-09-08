// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "NonNegativeStatistics.h"

namespace {
  // Forces the compiler to instantiate every method of NonNegativeStatistics
  // while the core library is built. The type itself is only used from
  // generated adaptive time-stepping code, so without this the core library
  // never compiles it. The function is never called.
  [[maybe_unused]] double instantiateNonNegativeStatistics() {
    const exahype2::NonNegativeStatistics statistics{1.0, 2.0};

    double result = 0.0;
    result += statistics.hasObservation() ? 1.0 : 0.0;
    result += statistics.hasPhysicalValues() ? 1.0 : 0.0;
    result += statistics.hasInvalidObservation() ? 1.0 : 0.0;
    result += static_cast<double>(statistics.invalidObservationBitsOrIdentity());

    const exahype2::NonNegativeStatistics accumulated = exahype2::NonNegativeStatistics::accumulate(statistics, 3.0);
    result += accumulated.maximum;

    const exahype2::NonNegativeStatistics fromBits = exahype2::NonNegativeStatistics::fromInvalidObservationBits(0u);
    result += fromBits.minimum;

    return result;
  }
} // namespace
