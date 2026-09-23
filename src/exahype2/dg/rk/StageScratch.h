// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

namespace exahype2::dg::rk {
  template <int NumberOfValues, typename Precision>
  [[nodiscard]] INLINE_METHOD Precision* stageScratch();
} // namespace exahype2::dg::rk

#include "exahype2/dg/rk/StageScratch.cpph"
