// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "{{SOLVER_NAME}}.h"

#include "exahype2/dg/EigenvalueStatistics.h"
#include "tarch/la/Vector.h"

namespace {{NAMESPACE | join("::")}} {
  ::exahype2::dg::MaxEigenvalueStatistics reduceMaxEigenvalues(
    {{FULL_QUALIFIED_SOLVER_NAME}}& solver,
    const {{SOLUTION_STORAGE_PRECISION}}* const Q,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    const double timeStamp,
    const double timeStepSize
  );
} // namespace {{NAMESPACE | join("::")}}
