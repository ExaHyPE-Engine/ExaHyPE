// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

TARGET_DECLARE
namespace exahype2::dg {
  template <
    int  NumberOfEntries,
    int  NumberOfTargetNodes,
    int  NumberOfSourceNodes,
    int  ValueStride,
    int  OperatorStride,
    int  ResultStride,
    bool OverwriteResult,
    typename ResultPrecision,
    typename ComputePrecision,
    typename... Scalings>
  INLINE_METHOD GPU_CALLABLE_METHOD void applyOperatorAlongAxis(
    ResultPrecision* NOALIAS        result,
    const ComputePrecision* NOALIAS values,
    const ComputePrecision* NOALIAS operatorMatrix,
    const Scalings... scalings
  );
} // namespace exahype2::dg
TARGET_DECLARE_END

#include "exahype2/dg/OperatorAlongAxis.cpph"
