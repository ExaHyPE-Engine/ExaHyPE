// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_CORE_H_
#define _TARCH_MULTICORE_CORE_H_


#include <thread>
#include <set>
#include <string>

#include "tarch/logging/Log.h"


namespace tarch {
  namespace multicore {
    class Core;

    /**
     * This routine runs through the Unix thread mask and counts how many
     * threads SLURM allows a code to use. It returns this count. If you
     * use multiple MPI ranks per node, each rank usually gets the permission
     * to access the same number of cores exclusively.
     */
    int getNumberOfUnmaskedThreads();

    /**
     * Creates a string representation of those threads which are available
     * to the processes. You get a string similar to
     *
     *       0000xxxx0000xxxx00000000000000
     *
     * The example above means that cores 4-7 and 12-15 are available to the
     * process, the other cores are not.
     */
    std::string printUnmaskedThreads();
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
class tarch::multicore::Core {
  private:
    /**
     * Logging device
     */
    static tarch::logging::Log  _log;

    Core();

    int               _numberOfThreads;
    int               _numberOfDevices;
    std::set<int>     _devicesToUse;
  public:
    /**
     * The default is what the system management typically gives you. So if
     * you run four ranks on a 24 core node, then each MPI rank will get 6
     * threads if you choose this constant.
     *
     * Multiply with two to exploit hyperthreading.
     */
    static constexpr int UseDefaultNumberOfThreads         = 0;

    /**
     * Accelerator devices (GPUs) are enumerated starting from 0. If you argue
     * where to offload tasks and want to pick the host, then pass in this
     * constant instead.
     */
    static constexpr int HostDevice = -1;

    /**
     * Destructor
     */
    ~Core();

    /**
     * @return Singleton instance
     */
    static Core& getInstance();

    /**
     * Configure the whole node, i.e. all cores available on a node. If
     * numberOfThreads equals the default, the routine will use the hardware
     * concurrency to determine the number of threads that should be used.
     * On SLURM-based HPC platforms, this will be wrong if multiple MPI ranks
     * are placed on one node. It is also a bad choice if hyperthreading
     * should not/can not be used. Use the helper function
     * getNumberOfUnmaskedThreads().
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
     *
     *
     * @param numberOfThreads Number of threads that shall be used. This
     *        parameter either is greater than zero (which defines the number
     *        of threads) or it equals DefaultNumberOfThreads which means that the code should
     *        use the default number of threads.
     */
    void configure( int numberOfThreads = UseDefaultNumberOfThreads, const std::set<int>& devicesToUse = {} );

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
     * Returns the number of threads that is used.
     *
     * @return Number of threads available.
     */
    int getNumberOfThreads() const;

    /**
     * Return the number of GPUs that are available. It is then Peano's policy
     * that you access these GPUs with numbers from 0 to ... We work with
     * logical GPU numbers, i.e. they might be mapped onto different numbers
     * internally. How this mapping is realised however is strongly depending
     * on the used GPU backend.
     */
    int getNumberOfGPUs() const;

    /**
     * @return Physical core the process is running on
     */
    int getCoreNumber() const;

    /**
     * @return Logical thread number
     */
    int getThreadNumber() const;
};


#endif
