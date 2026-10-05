// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/Enumeration.h"
#include "exahype2/fv/weno/NonlinearWeights.h"
#include "exahype2/fv/weno/Tables.h"
#include "peano4/Constants.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::reconstruction {
  template <int Order, exahype2::fv::weno::NonlinearWeighting Weighting>
  struct WENO;
} // namespace exahype2::fv::reconstruction

TARGET_DECLARE
/**
 * Polynomial weighted essentially non-oscillatory reconstruction of order @p Order after Dumbser,
 * Zanotti, Hidalgo and Balsara (2013). It turns the volume averages around a volume into a nodal
 * polynomial on that volume, which the ADER time stepping evolves with the space-time predictor.
 *
 * The reconstruction sweeps the axes one after the other: The first sweep reconstructs along the
 * first axis for every volume of the window around the volume, every further sweep reconstructs
 * the nodal coefficients of the previous one along the next axis. On every line, each candidate
 * stencil determines a polynomial of full order, and the @c Weighting combines them by their
 * oscillation indicators.
 *
 * The halo of a patch holds no volume beyond an edge or a corner of the patch, and a stencil that
 * would read such a volume drops out of the combination. The two patches that share a face must
 * agree on its flux, so the polynomials that feed it read only the volumes both patches hold,
 * selected through the shared face. Dropping stencils leaves the order intact because every
 * stencil is of full order, so a patch needs at least 2 * Order volumes per axis. The
 * reconstruction serves the volumes of the patch and the first halo layer, whose stencils reach
 * Order volumes further.
 *
 * The polynomial is nodal in @c Basis, from which a time stepping derives its operators, and the
 * caller provides @c ScratchSize entries for the sweeps.
 */
template <int Order, exahype2::fv::weno::NonlinearWeighting Weighting>
struct exahype2::fv::reconstruction::WENO final {
  using Tables = exahype2::fv::weno::ReconstructionTables<Order>;
  using Basis  = typename Tables::Basis;

  static constexpr int HaloSize = Order + 1;

  template <int NumberOfData>
  static constexpr int ScratchSize = 2 * peano4::power(2 * Order + 1, DIMENSIONS) * NumberOfData;

  template <int NumberOfVolumesPerAxis, int NumberOfData, typename StoragePrecision, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static void reconstructPolynomial(
    const StoragePrecision* NOALIAS const     inputPatch,
    const StoragePrecision* NOALIAS const     faceInputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, int>& volumeIndex,
    int                                       sharedFace,
    ComputePrecision* NOALIAS const           nodalState,
    ComputePrecision* NOALIAS const           scratch
  );

  template <int NumberOfVolumesPerAxis>
  INLINE_METHOD GPU_CALLABLE_METHOD static bool volumeIsAvailable(const tarch::la::Vector<DIMENSIONS, int>& volumeIndex);

  template <int NumberOfVolumesPerAxis, int NumberOfData, typename StoragePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static const StoragePrecision* volumeState(
    const StoragePrecision* NOALIAS const     inputPatch,
    const StoragePrecision* NOALIAS const     faceInputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, int>& volumeIndex
  );

  template <int NumberOfData, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static void reconstructAlongLine(
    const Tables&                         tables,
    const ComputePrecision* NOALIAS const lineValues,
    const bool* NOALIAS const             lineIsAvailable,
    ComputePrecision* NOALIAS const       coefficients
  );
};
TARGET_DECLARE_END

#include "exahype2/fv/reconstruction/WENO.cpph"
