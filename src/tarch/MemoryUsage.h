// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "CompilerSpecificSettings.h"

namespace tarch {
  enum class MemoryUsageFormat {
    KiB,
    MiB,
    GiB,
  };

  constexpr unsigned long long KiB = 1ull << 10;
  constexpr unsigned long long MiB = 1ull << 20;
  constexpr unsigned long long GiB = 1ull << 30;

  std::string toString(MemoryUsageFormat format);

  constexpr MemoryUsageFormat chooseMemoryUsageFormat(unsigned long long bytes) noexcept {
    if (bytes < MiB) {
      return MemoryUsageFormat::KiB;
    } else if (bytes < GiB) {
      return MemoryUsageFormat::MiB;
    } else {
      return MemoryUsageFormat::GiB;
    }
  }

  double convertMemorySize(unsigned long long bytes, MemoryUsageFormat format);

  /**
   * Return memory counters in bytes. The formatted overloads below are kept
   * for callers that display one particular binary unit.
   *
   * The implementation of the process footprint is based on the C version by
   * Michael Moltenbrey.
   */
  std::uint64_t getMemoryUsageInBytes();
  std::uint64_t getTotalMemoryInBytes();
  std::uint64_t getFreeMemoryInBytes();

  double getMemoryUsage(MemoryUsageFormat format);
  double getTotalMemory(MemoryUsageFormat format);
  double getFreeMemory(MemoryUsageFormat format);
} // namespace tarch
