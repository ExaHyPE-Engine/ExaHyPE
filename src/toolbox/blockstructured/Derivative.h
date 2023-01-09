// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


#include "peano4/utils/Globals.h"
#include "tarch/la/Vector.h"


namespace toolbox {
  namespace blockstructured {
    /**
     * This routine assumes that we have two patches of the same numberOfDoFsPerAxisInPatch.
     * We run over the these patches. Each element therein holds a vector of unknowns, i.e.
     * the whole patch is an AoS. We pick the sourceIndex from each of the
     * numberOfDoFsPerAxisInPatch x numberOfDoFsPerAxisInPatch x numberOfDoFsPerAxisInPatch
     * entries and copy it over to destinationIndex in the image.
     *
     *
     * I use an upwind/downwind scheme to compute the gradient, i.e. no central differences.
     * Basically, I run over the left half of the patch and look one cell right (for the
     * x-derivative) and then I run over the right half of the patch and look one cell to the
     * left.
     *
     *
     * @param numberOfDoFsPerAxisInPatch Size per coordinate axis. Has to be added twice the
     *   halo layer size if you have a halo
     */
    void computeGradient(
      int                                numberOfDoFsPerAxisInPatch,
      const double* __restrict__         source,
      int                                sourceIndex,
      int                                sourceUnknowns,
      int                                sourceHalo,
      double* __restrict__               dest,
      const tarch::la::Vector<Dimensions,int>&   destIndex,
      int                                destUnknowns,
      int                                destHalo,
      const tarch::la::Vector<Dimensions,double>&  volumeH
    );


    double computeGradientAndReturnMaxDifference(
      int                                numberOfDoFsPerAxisInPatch,
      const double* __restrict__         source,
      int                                sourceIndex,
      int                                sourceUnknowns,
      int                                sourceHalo,
      double* __restrict__               dest,
      const tarch::la::Vector<Dimensions,int>&   destIndex,
      int                                destUnknowns,
      int                                destHalo,
      const tarch::la::Vector<Dimensions,double>&  volumeH
    );
  }
}


