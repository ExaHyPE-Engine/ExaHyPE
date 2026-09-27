// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"

namespace exahype2::dg::limiting {
  /**
   * The role of a cell in the subcell limiter. Regular and Halo cells keep their Galerkin
   * solution, Limited and Troubled cells recompute their step on the subcell scheme, and Troubled
   * marks a cell whose candidate failed the admissibility test of the current step.
   */
  enum class LimiterStatus : int { Regular = 0, Halo = 1, Limited = 2, Troubled = 3 };

  /**
   * Number of spreading rounds that grow the subcell domain from a troubled cell out to its halo
   * ring; every round lowers the status of a neighbour by one level.
   */
  constexpr int NumberOfSpreadRings = static_cast<int>(LimiterStatus::Troubled) - static_cast<int>(LimiterStatus::Halo);

  /** The entries of the status chunk of one cell. */
  enum class StatusEntry : int { Status = 0, PreviousStatus = 1, WasTroubled = 2, PredictorDiverged = 3 };

  /** The size of the status chunk of one cell. */
  constexpr int NumberOfStatusEntries = 4;
} // namespace exahype2::dg::limiting

TARGET_DECLARE
namespace exahype2::dg::limiting {
  INLINE_METHOD GPU_CALLABLE_METHOD constexpr bool runsSubcellScheme(LimiterStatus status);

  INLINE_METHOD GPU_CALLABLE_METHOD constexpr LimiterStatus mergeLimiterStatus(LimiterStatus left, LimiterStatus right);

  INLINE_METHOD GPU_CALLABLE_METHOD constexpr LimiterStatus spreadLimiterStatus(LimiterStatus neighbourStatus);
} // namespace exahype2::dg::limiting
TARGET_DECLARE_END

#include "exahype2/dg/limiting/LimiterStatus.cpph"
