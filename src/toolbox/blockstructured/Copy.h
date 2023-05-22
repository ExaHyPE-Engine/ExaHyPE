// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once


namespace toolbox {
  namespace blockstructured {
    /**
     * Copy one unknown from one patch to the other
     *
     * This routine assumes that we have two patches of the same numberOfDoFsPerAxisInPatch.
     * We run over the these patches. Each element therein holds a vector of unknowns, i.e.
     * the whole patch is an AoS. We pick the sourceIndex from each of the
     * numberOfDoFsPerAxisInPatch x numberOfDoFsPerAxisInPatch x numberOfDoFsPerAxisInPatch
     * entries and copy it over to destinationIndex in the image. That is, the two patches
     * have to have the same number of unknowns, but they might have different unknowns.
     *
     * @param numberOfDoFsPerAxisInPatch Size per coordinate axis. Has to be added twice the
     *   halo layer size if you have a halo
     *
     * @param source Pointer to the source data. This is the whole input patch stored as AoS.
     *   It has the dimensions @f$ (numberOfDoFsPerAxisInPatch+2*sourceHalo)^Dimensions \cdot sourceUnknowns @f$.
     *
     * @param sourceIndex Which index in source is to be copied over to the destination field.
     *   Has to be greater equals to zero and has to be smaller than sourceUnknowns.
     *
     */
    void copyUnknown(
      int                                numberOfDoFsPerAxisInPatch,
      const double* __restrict__         source,
      int                                sourceIndex,
      int                                sourceUnknowns,
      int                                sourceHalo,
      double* __restrict__               dest,
      int                                destIndex,
      int                                destUnknowns,
      int                                destHalo
    );

    /**
     * Copy all unknowns
     */
    void copyUnknowns(
      int                                numberOfDoFsPerAxisInPatch,
      const double* __restrict__         source,
      int                                sourceHalo,
      double* __restrict__               dest,
      int                                destHalo,
      int                                unknowns
    );

    double copyUnknownAndComputeMaxDifference(
      int                                numberOfDoFsPerAxisInPatch,
      const double* __restrict__         source,
      int                                sourceIndex,
      int                                sourceUnknowns,
      int                                sourceHalo,
      double* __restrict__               dest,
      int                                destIndex,
      int                                destUnknowns,
      int                                destHalo
    );
  }
}


