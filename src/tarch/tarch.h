// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "CompilerSpecificSettings.h"

#include <string>

/**
 * @namespace tarch
 *
 * tarch denotes the Technical Architectures. It is used within Peano, but it
 * is designed in a way that it is more or less independent of Peano, i.e.
 * Peano uses the tarch but not the other way round.
 *
 * \section Debug level
 *
 * - PEANO_DEBUG = 1: Enable tracing
 * - PEANO_DEBUG = 2: Enable assertions (includes level 1)
 * - PEANO_DEBUG = 4: Enable debug messages (includes level 2)
 *
 * \section Default libraries
 *
 * By default, Peano builds all core libraries in three different versions:
 *
 * - A production variant with PEANO_DEBUG=0
 * - A trace variant which holds a compile with PEANO_DEBUG=1
 * - A asserts variant which holds a compile with PEANO_DEBUG=2
 * - A debug variant which holds a compile with PEANO_DEBUG=4
 *
 * Note: The trace variant does not contain any assertions, as assertions are
 * typically expensive and slow down the code considerably.
 *
 */
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
   * Method for getting the application's memory footprint
   *
   * We use the exact version of KiloByte with 1024 bytes.
   *
   * The implementation is based on C-version of Michael Moltenbrey.
   */
  double getMemoryUsage(MemoryUsageFormat format);
  double getTotalMemory(MemoryUsageFormat format);
  double getFreeMemory(MemoryUsageFormat format);
} // namespace tarch
