// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"

namespace exahype2::fv::reconstruction::weno {
  /**
   * A weighting maps the linear weights and the oscillation indicators of the
   * admissible candidate stencils of one volume onto the normalised nonlinear
   * weights that combine the candidate reconstructions. It is stateless with
   * static members only. A weighting sees only these arrays, so a scheme that
   * selects stencils differently, such as TENO, plugs in without a change to
   * the reconstruction.
   */
  template <typename Weighting>
  concept NonlinearWeighting = std::is_empty_v<Weighting>;

  template <int Exponent>
  struct JiangShu;
} // namespace exahype2::fv::reconstruction::weno

TARGET_DECLARE
/**
 * The weights of Jiang and Shu (1996): every linear weight is divided by the
 * @p Exponent-th power of its regularised oscillation indicator, and the
 * results are normalised. Jiang and Shu use the exponent 2, the ADER-WENO
 * schemes of Dumbser et al. the exponent 8. The powers are taken relative to
 * the smallest regularised indicator, which leaves the weights unchanged but
 * keeps them finite in single precision.
 */
template <int Exponent>
struct exahype2::fv::reconstruction::weno::JiangShu final {
  static_assert(Exponent >= 1, "The Jiang-Shu weights need a positive exponent.");

  template <typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static void weights(
    int                             numberOfStencils,
    const ComputePrecision* NOALIAS linearWeights,
    const ComputePrecision* NOALIAS oscillationIndicators,
    ComputePrecision* NOALIAS       nonlinearWeights
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/reconstruction/weno/NonlinearWeights.cpph"
