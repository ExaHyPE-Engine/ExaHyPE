// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/instrumentation/Log.h"

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
  // Is default. We use init() instead.
  Device() = default;

public:
  static constexpr int DefaultDevice = 0;

  // Is default. We use shutdown() instead.
  ~Device() = default;

  /**
   * @return Singleton instance
   */
  static Device& getInstance();

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
