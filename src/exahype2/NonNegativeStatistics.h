// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

TARGET_DECLARE
namespace exahype2 {
  struct NonNegativeStatistics;
} // namespace exahype2

struct exahype2::NonNegativeStatistics final {
  double minimum{std::numeric_limits<double>::infinity()};
  double maximum{0.0};

  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD bool hasObservation() const;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD bool hasPhysicalValues() const;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD bool hasInvalidObservation() const;
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD std::uint64_t invalidObservationBitsOrIdentity() const;

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD NonNegativeStatistics
  accumulate(const NonNegativeStatistics& currentStatistics, double observedValue);

  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD NonNegativeStatistics
  fromInvalidObservationBits(std::uint64_t invalidObservationBits);

private:
  [[nodiscard]] static INLINE_METHOD GPU_CALLABLE_METHOD bool isPhysicalValue(double value);
};
TARGET_DECLARE_END

#include "exahype2/NonNegativeStatistics.cpph"
