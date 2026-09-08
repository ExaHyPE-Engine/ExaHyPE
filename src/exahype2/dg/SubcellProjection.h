// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

TARGET_DECLARE
namespace exahype2::dg {
  template <int Dimensions, int InputDofs, int OutputDofs, int ValuesPerDof, typename Value>
  INLINE_METHOD GPU_CALLABLE_METHOD void projectTensorProduct(
    const Value* const input,
    Value* const       output,
    const Value* const matrix
  );
} // namespace exahype2::dg
TARGET_DECLARE_END

#include "exahype2/dg/SubcellProjection.cpph"
