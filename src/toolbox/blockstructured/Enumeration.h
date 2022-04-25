// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_TOOLBOX_BLOCKSTRUCTURED_ENUMERATION_H_
#define _PEANO4_TOOLBOX_BLOCKSTRUCTURED_ENUMERATION_H_


#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"


namespace toolbox {
  namespace blockstructured {
    /**
     * The volumes or elements within an overlap are always enumerated
     * lexicographically. This routine serialises this index.
     *
     * @param overlap Overlap of one patch into the other. If you have a
     *   halo of one cell around each patch, then this parameter is 1.
     */
    int serialisePatchIndexInOverlap(
      const tarch::la::Vector<Dimensions,int>& overlapCell,
      int                                      numberOfDoFsPerAxisInPatch,
      int                                      overlap,
      int                                      normal
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
    int serialiseMarkerIn3x3PatchAssembly(
      const tarch::la::Vector<Dimensions,int>& markerIndex,
      int                                      numberOfDoFsPerAxisInPatch
    );
  }
}


#endif
