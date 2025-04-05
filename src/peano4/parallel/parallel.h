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
     * out integer types.
     *
     * ## Terminal output
     *
     * Originally, I worked with logInfo() here to dump some info about the
     * used task types. However, some tasks ask for their id at start-up
     * through static variables. At this point, the logging might not be up,
     * i.e. we rely on the linker to get the logging up first. This is
     * generally a bad idea. Therefore, I write to std::cout in this particular
     * case. Otherwise, I encountered seg faults on some systems.
     */
    int getTaskType(const std::string& className);
  }
}
