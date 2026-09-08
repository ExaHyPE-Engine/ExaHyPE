// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/NonNegativeReduction.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/la/Vector.h"

namespace exahype2::dg {
  struct MaxEigenvalueStatistics;
} // namespace exahype2::dg

struct exahype2::dg::MaxEigenvalueStatistics final {
  double maxPhysicalEigenvalue{0.0};
  double maxScaledEigenvalue{0.0};

  template <int Dimensions, typename EigenvalueEvaluator>
  INLINE_METHOD void accumulate(
    const tarch::la::Vector<Dimensions, double>& cellSize,
    EigenvalueEvaluator&&                        evaluateEigenvalue
  );
};

#include "exahype2/dg/EigenvalueStatistics.cpph"
