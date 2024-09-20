// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <thread>
#include <set>
#include <string>

#include "tarch/logging/Log.h"

namespace tarch {
  namespace accelerator{
    class Device;
  }
}

/**
 * Core
 *
 * Any shared memory implementation has to provide a singleton Core. Its full
 * qualified name is tarch::multicore::Core. If no shared memory variant is
 * switched on, Peano provides a default Core implementation that does nothing.
 *
 * If you don't configure the core explicitly, it will try to use some
 * meaningful default.
 *
 * @see configure()
 *
 * @author Tobias Weinzierl
 */
class tarch::accelerator::Device {
  private:
    /**
     * Logging device
     */
    static tarch::logging::Log  _log;

    static int _localDeviceId;

    Device();
  public:
    /**
     * Accelerator devices (GPUs) are enumerated starting from 0. If you argue
     * where to offload tasks and want to pick the host, then pass in this
     * constant instead.
     */
    static constexpr int HostDevice = -1;

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
     * In SYCL, you cannot alter the core count atm on the node. Obviously,
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
     * Shutdown parallel environment.
     */
    void shutdown();

    /**
     * @return Shared memory environment is up and running. Most shared
     * memory implementations work properly with the defaults. They just
     * return true always.
     */
    bool isInitialised() const;

    /**
     * Return the number of GPUs that are available. It is then Peano's policy
     * that you access these GPUs with numbers from 0 to ... We work with
     * logical GPU numbers, i.e. they might be mapped onto different numbers
     * internally. How this mapping is realised however is strongly depending
     * on the used GPU backend.
     */
    int getNumberOfDevices() const;

    int getLocalDeviceId() const;
};
