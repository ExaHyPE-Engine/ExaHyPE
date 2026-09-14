// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

namespace exahype2::fv::weno {
  /**
   * A weighting maps the ideal weights and the smoothness indicators of the
   * candidate stencils of one volume onto the normalised nonlinear weights
   * that combine the candidate reconstructions. It is stateless with static
   * members only, so the kernel dispatches to it without moving an object
   * onto a GPU device. A weighting sees only these two arrays, so a scheme
   * that selects stencils differently, such as TENO, plugs in without a change
   * to the reconstruction.
   */
  template <typename Weighting>
  concept NonlinearWeighting = std::is_empty_v<Weighting>;

  constexpr int NumberOfCandidateStencils = 3;

  struct JiangShu;
} // namespace exahype2::fv::weno

TARGET_DECLARE
/**
 * The weights of Jiang and Shu (1996): every ideal weight is divided by the
 * square of its regularised smoothness indicator, and the results are
 * normalised. A candidate stencil that crosses a discontinuity therefore
 * contributes next to nothing, while equal indicators reproduce the ideal
 * weights and with them the full-order reconstruction.
 */
struct exahype2::fv::weno::JiangShu final {
  template <typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static void weights(
    const ComputePrecision* NOALIAS idealWeights,
    const ComputePrecision* NOALIAS smoothnessIndicators,
    ComputePrecision* NOALIAS       nonlinearWeights
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/weno/NonlinearWeights.cpph"
