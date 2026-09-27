// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/accelerator.h"

namespace exahype2 {
  template <typename Operation>
  struct ForEachLevelJump;

  template <typename Operation>
  void interpolateCoarseFaces(
    const peano4::Partition& partition,
    peano4::Dependence       dependence,
    const Operation&         operation
  );

  template <typename Operation>
  void restrictFineFaces(const peano4::Partition& partition, peano4::Dependence dependence, const Operation& operation);

  template <typename Operation>
  void restrictFineFaces(const peano4::Partition& partition, const Operation& operation);
} // namespace exahype2

TARGET_DECLARE
/** Runs operation on one level jump of the span. */
template <typename Operation>
struct exahype2::ForEachLevelJump final {
  tarch::Span<const peano4::LevelJump> levelJumps{};
  Operation                            operation{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};
TARGET_DECLARE_END

#include "exahype2/LevelJumps.cpph"
