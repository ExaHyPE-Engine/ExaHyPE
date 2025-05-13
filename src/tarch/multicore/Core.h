// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "config.h"
#include "tarch/logging/Log.h"

#include <set>
#include <string>
#include <thread>

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
  } // namespace multicore
} // namespace tarch

/**
 * Core
 *
 * Any shared memory implementation has to provide a singleton Core. Its full
 * qualified name is tarch::multicore::Core. If no shared memory variant is
 * switched on, Peano provides a default Core implementation that does nothing.
 *
 * @author Tobias Weinzierl
 */
class tarch::multicore::Core {
private:
  static tarch::logging::Log _log;

  Core() = default;

  int _numberOfThreads;

public:
  ~Core() = default;

  /**
   * @return Singleton instance
   */
  static Core& getInstance();

  bool init();

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
   * @return Physical core the process is running on
   */
  int getCoreNumber() const;

  /**
   * @return Logical thread number
   */
  int getThreadNumber() const;

  /**
   * Wrapper around backend-specific yield.
   *
   * For most backends, this should not really be a yield(). It should
   * interrupt the current task, not the current thread, and tell the
   * runtime to continue with another task.
   */
  void yield();
};
