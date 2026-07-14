// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Engine.h" // Step, RepositoryCallbacks
#include "tarch/la/Vector.h"

namespace runner {
  /**
   * The whole run lifecycle in one call: parse the merged RuntimeParameters, bring up tarch and
   * peano4, build the data repository, run the schedule through the Engine, tear it all down. A
   * generated main collapses to a single call into this.
   */
  int launch(
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainSize,
    const std::bitset<DIMENSIONS>&               periodicBC,
    RepositoryCallbacks                          callbacks,
    std::vector<Step>                            steps,
    int&                                         argc,
    char**                                       argv
  );
} // namespace runner
