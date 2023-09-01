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
     * Each task needs a unique type (number). As I don't want to hard-code these
     * types, I use a simple factory mechanism (aka this routine) to hand out integer
     * types.
     */
    int getTaskType(const std::string& className);
  }
}
