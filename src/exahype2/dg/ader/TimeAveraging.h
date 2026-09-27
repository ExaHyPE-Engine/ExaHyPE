// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/ader/PredictorScratch.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

TARGET_DECLARE
namespace exahype2::dg::ader {
  template <int NumberOfDofsPerAxis, int NumberOfUnknowns, int NumberOfAuxiliaryVariables, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD void averageStateOverTime(
    ComputePrecision* NOALIAS       spaceTimeState,
    const ComputePrecision* NOALIAS quadratureWeights
  );
} // namespace exahype2::dg::ader
TARGET_DECLARE_END

#include "exahype2/dg/ader/TimeAveraging.cpph"
