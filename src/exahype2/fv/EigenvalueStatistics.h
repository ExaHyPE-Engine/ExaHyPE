// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/NonNegativeReduction.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

TARGET_DECLARE
namespace exahype2::fv {
  template <typename Maximum, typename Eigenvalue>
  INLINE_METHOD GPU_CALLABLE_METHOD void accumulateMaxEigenvalue(Maximum& maximum, const Eigenvalue eigenvalue);
} // namespace exahype2::fv
TARGET_DECLARE_END

#include "exahype2/fv/EigenvalueStatistics.cpph"
