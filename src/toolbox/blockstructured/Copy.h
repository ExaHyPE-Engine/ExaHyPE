// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _PEANO4_TOOLBOX_BLOCKSTRUCTURED_COPY_H_
#define _PEANO4_TOOLBOX_BLOCKSTRUCTURED_COPY_H_


namespace toolbox {
  namespace blockstructured {
    /**
     * This routine assumes that we have two patches of the same numberOfDoFsPerAxisInPatch.
     * We run over the these patches. Each element therein holds a vector of unknowns, i.e.
     * the whole patch is an AoS. We pick the sourceIndex from each of the
     * numberOfDoFsPerAxisInPatch x numberOfDoFsPerAxisInPatch x numberOfDoFsPerAxisInPatch
     * entries and copy it over to destinationIndex in the image.
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


#endif
