// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/CompileTarget.h"

namespace exahype2::dg::limiting {
  /**
   * The role of a cell in the subcell limiter. Limited and Troubled cells recompute their step on
   * the subcell scheme, and the other cells keep their Galerkin solution. A Troubled cell failed
   * the admissibility test in the current step.
   */
  enum class LimiterStatus : int { Regular = 0, Halo = 1, Limited = 2, Troubled = 3 };

  /**
   * The name of the plotted LimiterStatus field, which spells out every status.
   */
  constexpr const char* LimiterStatusFieldName = "LimiterStatus[0=Regular,1=Halo,2=Limited,3=Troubled]";

  /**
   * The entries of the status chunk of one cell.
   */
  enum class StatusEntry : int { Status = 0, PreviousStatus = 1, WasTroubled = 2, PredictorDiverged = 3 };

  TARGET_DECLARE
  /**
   * The number of spreading rounds from a troubled cell out to its halo ring. Every round lowers
   * the status of a neighbour by one level.
   */
  constexpr int NumberOfSpreadRings = static_cast<int>(LimiterStatus::Troubled) - static_cast<int>(LimiterStatus::Halo);

  /**
   * The size of the status chunk of one cell.
   */
  constexpr int NumberOfStatusEntries = 4;
  TARGET_DECLARE_END
} // namespace exahype2::dg::limiting

TARGET_DECLARE
namespace exahype2::dg::limiting {
  INLINE_METHOD GPU_CALLABLE_METHOD constexpr bool runsSubcellScheme(LimiterStatus status);

  INLINE_METHOD GPU_CALLABLE_METHOD constexpr LimiterStatus mergeLimiterStatus(LimiterStatus left, LimiterStatus right);

  INLINE_METHOD GPU_CALLABLE_METHOD constexpr LimiterStatus spreadLimiterStatus(LimiterStatus neighbourStatus);
} // namespace exahype2::dg::limiting
TARGET_DECLARE_END

#include "exahype2/dg/limiting/LimiterStatus.cpph"
