// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"

namespace toolbox::blockstructured {
  /**
   * The volumes or elements within an overlap are always enumerated
   * lexicographically. This routine serialises this index.
   *
   * @param overlap Overlap of one patch into the other. If you have a
   *   halo of one cell around each patch, then this parameter is 1.
   */
  inline GPU_CALLABLE_METHOD int serialiseVoxelIndexInOverlap(
    const tarch::la::Vector<DIMENSIONS, int>& overlapCell,
    int                                       numberOfDoFsPerAxisInPatch,
    int                                       overlap,
    int                                       normal
  );

  /**
   * Patches along a face are basically organised as @f$ 3^{d-1} @f$
   * arrays, i.e. form a Cartesian 3x3 topology for a 3d setup. This
   * routine serialises the number of patches.
   */
  inline GPU_CALLABLE_METHOD int serialisePatchIndexInOverlap(
    const tarch::la::Vector<DIMENSIONS, int>& patchIndex,
    int                                       normal
  );

  /**
   * If you have a marker identifying one element within a 3x3 or
   * 3x3x3, respectively, set of patches, we sometimes have to serialise
   * this marker index. We use a lexicographic ordering here. The routine
   * returns the index of the first cell within the patch identified via
   * markerIndex
   *
   * @param markerIndex Typically returned by getRelativePositionWithinFatherCell().
   */
  inline GPU_CALLABLE_METHOD int serialiseMarkerIn3x3PatchAssembly(
    const tarch::la::Vector<DIMENSIONS, int>& markerIndex,
    int                                       numberOfDoFsPerAxisInPatch
  );
} // namespace toolbox::blockstructured

#include "Enumeration.cpph"
