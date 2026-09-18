// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "runner/Engine.h"
#include "tarch/la/Vector.h"

namespace runner {
  int launch(
    const tarch::la::Vector<DIMENSIONS, double>& domainOffset,
    const tarch::la::Vector<DIMENSIONS, double>& domainSize,
    const std::bitset<DIMENSIONS>&               periodicBoundaries,
    const RepositoryCallbacks&                   callbacks,
    std::vector<Step>                            steps,
    int&                                         argc,
    char**                                       argv
  );
}
