// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include <string>

namespace peano4 {
  /**
   * \namespace parallel
   *
   * The parallel namespace is Peano's core abstracts from both MPI and
   * multicore parallelisation.
   *
   */
  namespace parallel {
    /**
     * Get unique number (id) for task
     *
     * Each task needs a unique type (number). As I don't want to hard-code
     * these types, I use a simple factory mechanism (aka this routine) to hand
     * out integer types. The routine internally memorises the mapping and
     * you can query it.
     *
     * ## Terminal output
     *
     * Originally, I worked with logInfo() here to dump some info about the
     * used task types. However, some tasks ask for their id at start-up
     * through static variables. At this point, the logging might not be up,
     * i.e. we rely on the linker to get the logging up first. This is
     * generally a bad idea. Therefore, I removed outputs through our
     * well-defined logging interface and instead dump them plainly onto
     * std::cout.
     *
     * I also omit checks that only rank 0 dumps an output. This means that
     * you get lots of output at large rank counts, but once again code might
     * ask for the task type before the ranks are configured properly.
     * Actually, I used to have
     *
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *   if (tarch::mpi::Rank::getInstance().isGlobalMaster() ) {
     *     std::cout << "assign task " << className << " id " << taskTypeCounter  << std::endl;
     *   }
     * ~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~
     *
     * but this introduced assertions, as we only can invoke isGlobalMaster()
     * once the rank is properly configured.
     *
     */
    int getTaskType(const std::string& className);

    std::string getClassNameOfType(int taskType);
  }
}
