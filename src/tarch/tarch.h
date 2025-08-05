// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "CompilerSpecificSettings.h"

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
 * - A debug variant which holds a compile with PEANO_DEBUG=4
 * - A trace variant which holds a compile with PEANO_DEBUG=1
 * - A production variant with PEANO_DEBUG=0
 *
 * Note: The trace variant does not contain any assertions, as assertions are
 * typically expensive and slow down the code considerably.
 *
 */
namespace tarch {
  enum class MemoryUsageFormat {
    MByte,
    GByte,
  };

  int convertMemorySize(long long value, MemoryUsageFormat format);

  /**
   * Method for getting the application's memory footprint
   *
   * We use the exact version of KiloByte with 1024 bytes.
   *
   * The implementation is based on C-version of Michael Moltenbrey.
   */
  int getMemoryUsage(MemoryUsageFormat format);
  int getTotalMemory(MemoryUsageFormat format);
  int getFreeMemory(MemoryUsageFormat format);
} // namespace tarch
