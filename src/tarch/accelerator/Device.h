// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <set>
#include <map>
#include <string>
#include <thread>

#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"

namespace tarch::accelerator {
  class Device;
} // namespace tarch::accelerator

/**
 * Device
 *
 * Any accelerator implementation has to provide a singleton Device. Its full
 * qualified name is tarch::accelerator::Device. If no accelerator variant is
 * switched on, Peano provides a default Device implementation that does
 * nothing.
 *
 * @author Tobias Weinzierl
 */
class tarch::accelerator::Device {
private:
  static tarch::logging::Log _log;

  std::map<void*, std::size_t> _memoryBlockTable;
  multicore::BooleanSemaphore _memoryBlockSemaphore;

  // Is default. We use init() instead.
  Device() = default;

  void checkForMemoryLeaks();

public:
  static constexpr int DefaultDevice = 0;

  // Is default. We use shutdown() instead.
  ~Device() = default;

  /**
   * @return Singleton instance
   */
  static Device& getInstance();

  std::map<void*, std::size_t>& getMemoryBlockTable();
  const std::map<void*, std::size_t>& getMemoryBlockTable() const;

  multicore::BooleanSemaphore& getMemoryBlockSemaphore();
  const multicore::BooleanSemaphore& getMemoryBlockSemaphore() const;

  bool init();

  /**
   * Shutdown accelerator environment.
   */
  void shutdown();

  /**
   * @return Accelerator environment is up and running. Most accelerator
   * implementations work properly with the defaults. They just
   * return true always.
   */
  bool isInitialised() const;

  /**
   * Return the number of GPUs that are available. It is then Peano's policy
   * that you access these GPUs with numbers from 0 to N. We work with
   * logical GPU numbers, i.e. they might be mapped onto different numbers
   * internally. How this mapping is realised however is strongly depending
   * on the used accelerator implementation.
   */
  int getNumberOfDevices() const;
};
