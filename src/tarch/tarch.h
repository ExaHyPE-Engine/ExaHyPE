// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_H_
#define _TARCH_H_


/**
 * @namespace tarch
 *
 * tarch denotes the Technical Architectures. It is used within Peano, but it
 * is designed in a way that it is more or less independent of Peano, i.e.
 * Peano uses the tarch but not the other way round.
 *
 * Please note that the tarch in the Peano4 repository/branch is not the same
 * as the tarch in previous Peano versions. I've cleaned it up quite a lot and
 * old features that are not used anymore are removed.
 *
 * \section Debug level
 *
 * - PeanoDebug = 1: Enable tracing
 * - PeanoDebug = 2: Enable assertions (includes level 1)
 * - PeanoDebug = 4: Enable debug messages (includes level 2)
 *
 * \section Default libraries
 *
 * By default, Peano 4 builds all core libraries in three different versions:
 *
 * - A debug variant which holds a compile with PeanoDebug=4
 * - A trace variant which holds a compile with PeanoDebug=1
 * - A production variants with PeanoDebug=0
 *
 * Note: The trace variant does not contain any assertions, as assertions are
 * typically expensive and slow down the code considerably.
 *
 */
namespace tarch {
  enum class MemoryUsageFormat {
	MByte
  };

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
}


#endif
