// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <set>
#include <string>
#include <thread>

#include "accelerator.h"
#include "tarch/logging/Log.h"

namespace tarch::accelerator {
  class Device;
} // namespace tarch::accelerator

/**
 * Device
 *
 * Any accelerator implementation has to provide a singleton Device. Its full
 * qualified name is tarch::multicore::Device. If no accelerator variant is
 * switched on, Peano provides a default Device implementation that does nothing.
 *
 * If you don't configure the Device explicitly, it will try to use some
 * meaningful default.
 *
 * @see configure()
 *
 * @author Tobias Weinzierl
 */
class tarch::accelerator::Device {
private:
  static tarch::logging::Log _log;

  Device();

public:
  /**
   * Destructor
   */
  ~Device();

  /**
   * @return Singleton instance
   */
  static Device& getInstance();

  /**
   *
   * ## TBB
   *
   * In the TBB context, the GPU setting are ignored. If you however combine
   * TBB with SYCL, then the SYCL policies (see below) hold.
   *
   * ## OpenMP
   *
   * By default, OpenMP makes all devices visible to the user.
   *
   * ## SYCL
   *
   * In SYCL, you cannot alter the core count on the node. Obviously,
   * we could split the queue on the host according to this spec, but then
   * we'd end up with some cores idling. So we do not support resetting the
   * thread count on the host with SYCL.
   *
   * By default, SYCL makes no device visible to the user. You have to call
   * configure() manually to make GPUs available to Peano.
   *
   */
  void configure(const std::set<int>& devicesToUse = {});

  /**
   * Shutdown accelerator environment.
   */
  void shutdown();

  /**
   * @return Accelerator environment is up and running.
   * Most accelerator implementations work properly with the defaults
   * They just return true always.
   */
  bool isInitialised() const;

  /**
   * Return the number of GPUs that are available. It is then Peano's policy
   * that you access these GPUs with numbers from 0 to N. We work with
   * logical GPU numbers, i.e., they might be mapped onto different numbers
   * internally. How this mapping is realised however is strongly depending
   * on the used accelerator implementation.
   */
  int getNumberOfDevices() const;

  int getRankLocalDeviceId() const;
};
