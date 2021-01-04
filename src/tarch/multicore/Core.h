// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _TARCH_MULTICORE_CORE_H_
#define _TARCH_MULTICORE_CORE_H_


#include <thread>
#include "tarch/logging/Log.h"


namespace tarch {
  namespace multicore {
    class Core;
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

    int _numberOfThreads;
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
     * Destructor
     */
    ~Core();

    /**
     * @return Singleton instance
     */
    static Core& getInstance();

    /**
     * Configure the whole thing. If numberOfThreads equals the default, the
     * routine will use the hardware concurrency to determine the number of
     * threads that should be used. On SLURM-based HPC platforms, this will
     * be wrong if multiple MPI ranks are placed on one node. It is also a
     * bad choice if hyperthreading should not/can not be used. Use the helper
     * function getNumberOfUnmaskedThreads().
     *
     * @param numberOfThreads Number of threads that shall be used. This
     *        parameter either is greater than zero (which defines the number
     *        of threads) or it equals DefaultNumberOfThreads which means that the code should
     *        use the default number of threads.
     */
    void configure( int numberOfThreads = UseDefaultNumberOfThreads, int maxNumberOfConcurrentBackgroundTasks = UseDefaultNumberOfThreads, int maxNumberOfConcurrentBandwidthBoundTasks = UseDefaultNumberOfThreads );

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
     * This is a real string which identifies the (OS) thread.
     */
    std::string getThreadId() const;

    /**
     * @return Physical core the process is running on
     */
    int getCoreNumber() const;

    /**
     * This routine runs through the Unix thread mask and counts how many
     * threads SLURM allows a code to use. It returns this count. If you
     * use multiple MPI ranks per node, each rank usually gets the permission
     * to access the same number of cores exclusively.
     */
    static int getNumberOfUnmaskedThreads();
};


#endif
