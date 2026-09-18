// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

TARGET_DECLARE
namespace exahype2::dg::rk {
  template <
    int NumberOfRungeKuttaSteps,
    int NumberOfDofsPerAxis,
    int NumberOfUnknowns,
    int NumberOfAuxiliaryVariables,
    typename StoragePrecision,
    typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void linearCombinationOfEstimates(
    int                             rungeKuttaStep,
    const StoragePrecision*         cellState,
    const StoragePrecision* NOALIAS rightHandSideEstimates,
    StoragePrecision*               stageState,
    const ComputePrecision* NOALIAS coefficients
  );
} // namespace exahype2::dg::rk
TARGET_DECLARE_END

#include "exahype2/dg/rk/LinearCombination.cpph"
