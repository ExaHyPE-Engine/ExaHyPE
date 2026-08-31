// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/la/Vector.h"

namespace applications::exahype2::exagrype {
  void initialCondition(const tarch::la::Vector<3, double>& x, double t, int nVars, double* NOALIAS Q);
} // namespace applications::exahype2::exagrype
