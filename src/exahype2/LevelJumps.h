// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/Partition.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/Span.h"
#include "tarch/accelerator/CompileTarget.h"

namespace exahype2 {
  template <typename Operation>
  struct ForEachLevelJump;

  INLINE_METHOD bool holdsIntermediateFace(
    const peano4::Partition& partition,
    peano4::Adjacency        adjacency,
    int                      level
  );

  template <typename Operation>
  INLINE_METHOD void interpolateCoarseFaces(
    const peano4::Partition& partition,
    peano4::Adjacency        adjacency,
    const Operation&         operation
  );

  template <typename Operation>
  INLINE_METHOD void restrictFineFaces(
    const peano4::Partition& partition,
    peano4::Adjacency        adjacency,
    const Operation&         operation
  );

  template <typename Operation>
  INLINE_METHOD void restrictFineFaces(const peano4::Partition& partition, const Operation& operation);
} // namespace exahype2

TARGET_DECLARE
template <typename Operation>
struct exahype2::ForEachLevelJump final {
  tarch::Span<const peano4::LevelJump> levelJumps{};
  Operation                            operation{};

  INLINE_METHOD GPU_CALLABLE_METHOD void operator()(int entry) const;
};
TARGET_DECLARE_END

#include "exahype2/LevelJumps.cpph"
