// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_TOOLBOX_BLOCKSTRUCTURED_ENUMERATION_H_
#define _PEANO4_TOOLBOX_BLOCKSTRUCTURED_ENUMERATION_H_


#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"


namespace toolbox {
  namespace blockstructured {
    /**
     * @param overlap Overlap of one patch into the other. If you have a
     *   halo of one cell around each patch, then this parameter is 1.
     */
    int serialisePatchIndexInOverlap(
      const tarch::la::Vector<Dimensions,int>& overlapCell,
      int                                      numberOfDoFsPerAxisInPatch,
      int                                      overlap,
      int                                      normal
    );
  }
}


#endif
