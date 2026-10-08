// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "exahype2/dg/ader/TeamWorkers.h"
#include "exahype2/fv/Patch.h"
#include "exahype2/fv/reconstruction/weno/NonlinearWeights.h"
#include "exahype2/fv/reconstruction/weno/Tables.h"
#include "peano4/Constants.h"
#include "tarch/Assertions.h"
#include "tarch/CompilerSpecificSettings.h"
#include "tarch/accelerator/accelerator.h"
#include "tarch/la/Vector.h"

namespace exahype2::fv::reconstruction {
  template <int Order, exahype2::fv::reconstruction::weno::NonlinearWeighting Weighting>
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
 *
 * reconstructPatch() yields the polynomials of all volumes of a patch at once with the results of
 * reconstructPolynomial(). Every line of a sweep then serves every volume of the patch that reads
 * it, and the caller provides @c PatchScratchSize entries.
 */
template <int Order, exahype2::fv::reconstruction::weno::NonlinearWeighting Weighting>
struct exahype2::fv::reconstruction::WENO final {
  using Tables = exahype2::fv::reconstruction::weno::ReconstructionTables<Order>;
  using Basis  = typename Tables::Basis;

  static constexpr int HaloSize = Order + 1;

  template <int NumberOfData>
  static constexpr int ScratchSize = 2 * peano4::power(2 * Order + 1, DIMENSIONS) * NumberOfData;

  template <int NumberOfVolumesPerAxis, int NumberOfData>
  static constexpr int getPatchBufferSize(const int sweptAxes) {
    int size = NumberOfData;
    for (int axis = 0; axis < DIMENSIONS; axis++) {
      size *= axis < sweptAxes ? NumberOfVolumesPerAxis * Tables::NumberOfNodes : NumberOfVolumesPerAxis + 2 * Order;
    }
    return size;
  }

  template <int NumberOfVolumesPerAxis, int NumberOfData>
  static constexpr int PatchScratchSize = [] {
    int size = 0;
    for (int sweptAxes = 0; sweptAxes < DIMENSIONS; sweptAxes++) {
      size += getPatchBufferSize<NumberOfVolumesPerAxis, NumberOfData>(sweptAxes);
    }
    return size;
  }();

  template <int NumberOfVolumesPerAxis, int NumberOfData, typename StoragePrecision, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static void reconstructPolynomial(
    const StoragePrecision* NOALIAS const     inputPatch,
    const StoragePrecision* NOALIAS const     faceInputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, int>& volumeIndex,
    int                                       sharedFace,
    ComputePrecision* NOALIAS const           nodalState,
    ComputePrecision* NOALIAS const           scratch
  );

  template <int NumberOfVolumesPerAxis, int NumberOfData, typename StoragePrecision, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static void reconstructPatch(
    const StoragePrecision* NOALIAS const  inputPatch,
    const StoragePrecision* NOALIAS const  faceInputPatches[peano4::FacesPerCell],
    ComputePrecision* NOALIAS const        nodalStates,
    ComputePrecision* NOALIAS const        scratch,
    const exahype2::dg::ader::TeamWorkers& workers = exahype2::dg::ader::TeamWorkers{}
  );

  template <int NumberOfVolumesPerAxis>
  INLINE_METHOD GPU_CALLABLE_METHOD static bool volumeIsAvailable(
    const tarch::la::Vector<DIMENSIONS, int>& volumeIndex
  );

  template <int NumberOfVolumesPerAxis, int NumberOfData, typename StoragePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static const StoragePrecision* volumeState(
    const StoragePrecision* NOALIAS const     inputPatch,
    const StoragePrecision* NOALIAS const     faceInputPatches[peano4::FacesPerCell],
    const tarch::la::Vector<DIMENSIONS, int>& volumeIndex
  );

  template <int NumberOfVolumesPerAxis, int NumberOfData, int EntriesPerItem, typename ComputePrecision>
  INLINE_METHOD GPU_CALLABLE_METHOD static void sweepPatch(
    int                                    sweptAxis,
    const ComputePrecision* NOALIAS const  input,
    ComputePrecision* NOALIAS const        output,
    const exahype2::dg::ader::TeamWorkers& workers
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
