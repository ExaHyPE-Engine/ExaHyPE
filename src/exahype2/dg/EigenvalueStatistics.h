// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/NonNegativeReduction.h"
#include "exahype2/dg/DGUtils.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

TARGET_DECLARE
namespace exahype2::dg {
  struct MaxEigenvalueStatistics;

  template <
    typename Solver,
    int NumberOfDofsPerAxis,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD MaxEigenvalueStatistics reduceMaxEigenvalues(
    const StoragePrecision* NOALIAS              cellState,
    const ComputePrecision* NOALIAS              quadratureNodes,
    const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
    const tarch::la::Vector<DIMENSIONS, double>& cellSize,
    double                                       timeStamp,
    double                                       timeStepSize
  );
} // namespace exahype2::dg

struct exahype2::dg::MaxEigenvalueStatistics final {
  double maxPhysicalEigenvalue{0.0};
  double maxScaledEigenvalue{0.0};

  template <int Dimensions, typename EigenvalueEvaluator>
  INLINE_METHOD GPU_CALLABLE_METHOD void accumulate(
    const tarch::la::Vector<Dimensions, double>& cellSize,
    EigenvalueEvaluator&&                        evaluateEigenvalue
  );
};
TARGET_DECLARE_END

#include "exahype2/dg/EigenvalueStatistics.cpph"
