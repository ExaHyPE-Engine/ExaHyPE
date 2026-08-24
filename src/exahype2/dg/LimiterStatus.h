// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/Atomics.h"
#include "tarch/accelerator/accelerator.h"

namespace exahype2::dg {
  /**
   * Identifies a cell's limiter role. @c Regular and @c Halo remain on the
   * Galerkin scheme, while @c Limited and @c Troubled run the subcell scheme.
   * @c Troubled marks a current admissibility failure.
   */
  enum class LimiterStatus : int { Regular = 0, Halo = 1, Limited = 2, Troubled = 3 };

  /**
   * Number of spreading sweeps that grow the subcell domain from a troubled cell
   * out to its halo ring. Each sweep lowers a neighbour by one status level, so
   * the count is the number of levels between the two.
   */
  constexpr int NumberOfSpreadRings = static_cast<int>(LimiterStatus::Troubled) - static_cast<int>(LimiterStatus::Halo);
} // namespace exahype2::dg

TARGET_DECLARE
namespace exahype2::dg {
  INLINE_METHOD GPU_CALLABLE_METHOD constexpr bool runsSubcellScheme(const LimiterStatus status);

  INLINE_METHOD GPU_CALLABLE_METHOD constexpr LimiterStatus mergeLimiterStatus(
    const LimiterStatus left,
    const LimiterStatus right
  );

  INLINE_METHOD GPU_CALLABLE_METHOD constexpr LimiterStatus spreadLimiterStatus(const LimiterStatus neighbourStatus);

  INLINE_METHOD GPU_CALLABLE_METHOD LimiterStatus loadFaceStatus(const int* const faceStatus, const int side);

  INLINE_METHOD GPU_CALLABLE_METHOD void storeFaceStatus(
    int* const          faceStatus,
    const int           side,
    const LimiterStatus status
  );

  INLINE_METHOD GPU_CALLABLE_METHOD void raiseFaceStatus(
    int* const          faceStatus,
    const int           side,
    const LimiterStatus status
  );
} // namespace exahype2::dg
TARGET_DECLARE_END

#include "exahype2/dg/LimiterStatus.cpph"
