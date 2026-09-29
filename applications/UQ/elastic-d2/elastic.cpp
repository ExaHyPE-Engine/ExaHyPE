// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "elastic.h"


void applications::exahype2::exaseis::elastic::initPointSourceLocations([[maybe_unused]] double location[NumberOfPointSources][DIMENSIONS]) {

  location[0][0] = RuntimeParameters::Storage::pointSourcePosition[0];
  location[0][1] = RuntimeParameters::Storage::pointSourcePosition[1];
#if DIMENSIONS==3
  location[0][2] = RuntimeParameters::Storage::pointSourcePosition[2];
#endif

}

