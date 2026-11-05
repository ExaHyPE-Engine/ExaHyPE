// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/ChunkedMemoryPool.h"
#include "tarch/accelerator/CompileTarget.h"

namespace exahype2::fv::ader {
  /**
   * The role of a patch, or of a volume on a patch face with a fallback per volume, in the
   * fallback limiter after its candidate update. Limited and Troubled ones repeat their step with
   * the fallback scheme and the others keep their ADER-WENO step. A Troubled one failed the
   * admissibility test or its prediction diverged in the current step, a Limited one lies beside a
   * troubled volume, and a Halo patch takes the fallback flux on a face to a limited neighbour.
   */
  enum class LimiterStatus : int { Regular = 0, Halo = 1, Limited = 2, Troubled = 3 };

  /**
   * The entries of the status chunk of one cell. Between the prediction and the correction,
   * PredictorDiverged marks a patch with a volume whose Picard iteration diverged or stopped at its
   * sweep limit.
   */
  enum class StatusEntry : int { Status = 0, PredictorDiverged = 1 };

  /**
   * Why a volume repeated its step with the fallback scheme in the last time step, or Accepted: Its
   * candidate state is not finite, fails the physical admissibility term on its own or the discrete
   * maximum principle, it lies beside such a volume, or its patch repeated the step as a whole.
   */
  enum class VolumeStatus : int {
    Accepted         = 0,
    NotFinite        = 1,
    Inadmissible     = 2,
    MaximumPrinciple = 3,
    Neighbour        = 4,
    Patch            = 5
  };

  /**
   * The name of the plotted VolumeStatus field, which spells out every status.
   */
  constexpr const char* VolumeStatusFieldName =
    "LimiterStatus[0=Accepted,1=NotFinite,2=Inadmissible,3=MaximumPrinciple,4=Neighbour,5=Patch]";

  TARGET_DECLARE
  /**
   * The size of the status chunk of one cell.
   */
  constexpr int NumberOfStatusEntries = 2;
  TARGET_DECLARE_END
} // namespace exahype2::fv::ader

TARGET_DECLARE
namespace exahype2::fv::ader {
  INLINE_METHOD GPU_CALLABLE_METHOD constexpr bool runsFallbackScheme(LimiterStatus status);
} // namespace exahype2::fv::ader
TARGET_DECLARE_END

namespace exahype2::fv::ader {
  INLINE_METHOD void fillEveryChunk(tarch::accelerator::ChunkedMemoryPool<int>& pool, int value);
} // namespace exahype2::fv::ader

#include "exahype2/fv/ader/LimiterStatus.cpph"
