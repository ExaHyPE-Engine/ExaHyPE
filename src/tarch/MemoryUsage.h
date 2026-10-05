// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"

namespace tarch {
  enum class MemoryUsageFormat { KiB, MiB, GiB };

  constexpr unsigned long long KiB = 1ULL << 10;
  constexpr unsigned long long MiB = 1ULL << 20;
  constexpr unsigned long long GiB = 1ULL << 30;

  template <typename Element>
  INLINE_METHOD std::size_t getAllocatedBytesOf(const std::vector<Element>& elements);

  std::string toString(MemoryUsageFormat format);

  INLINE_METHOD constexpr MemoryUsageFormat chooseMemoryUsageFormat(unsigned long long bytes) noexcept;

  double convertMemorySize(unsigned long long bytes, MemoryUsageFormat format);

  std::uint64_t getMemoryUsageInBytes();
  std::uint64_t getTotalMemoryInBytes();
  std::uint64_t getAvailableMemoryInBytes();
} // namespace tarch

#include "tarch/MemoryUsage.cpph"
