// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"
#include "tarch/accelerator/ParallelFor.h"

namespace exahype2::dg::ader {
  struct TeamWorkers;
} // namespace exahype2::dg::ader

TARGET_DECLARE
/**
 * The threads that run the predictor of one cell. A team of teamThreads threads runs groups
 * cells at once, threads threads per cell. hasCell is false on a thread whose group received no
 * cell and that thread takes part in every barrier and computes nothing.
 */
struct exahype2::dg::ader::TeamWorkers final {
  int                                   thread{0};
  int                                   threads{1};
  int                                   teamThread{0};
  int                                   teamThreads{1};
  int                                   group{0};
  int                                   groups{1};
  bool                                  hasCell{true};
  const tarch::accelerator::TeamHandle* team{nullptr};
  unsigned char*                        scratch{nullptr};
};

namespace exahype2::dg::ader {
  [[nodiscard]] INLINE_METHOD GPU_CALLABLE_METHOD TeamWorkers getWorkers(
    const tarch::accelerator::TeamHandle& team,
    int                                   threadsPerCell
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void synchronise(const TeamWorkers& workers);
} // namespace exahype2::dg::ader
TARGET_DECLARE_END

#include "exahype2/dg/ader/TeamWorkers.cpph"
