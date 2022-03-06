// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FV_PATCH_UTILS_H_
#define _EXAHYPE2_FV_PATCH_UTILS_H_


#include <vector>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"


namespace exahype2 {
  namespace fv {
    /**
     * In ExaHyPE's Finite Volume setup, a cell hosts a patch of Finite Volumes.
     * When we iterate over these volumes, we typically have to know the centre
     * of the volume.
     *
     * @param x      Centre of the cell
     * @param h      Size of the cell
     * @param index  Index of Finite Volume (in lexicographic ordering)
     */
    tarch::la::Vector<2,double>  getVolumeCentre(
      const tarch::la::Vector<2,double>&  x,
      const tarch::la::Vector<2,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch,
      const tarch::la::Vector<2,int>&     index
    );

    tarch::la::Vector<3,double>  getVolumeCentre(
      const tarch::la::Vector<3,double>&  x,
      const tarch::la::Vector<3,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch,
      const tarch::la::Vector<3,int>&     index
    );

    /**
     * With GCC 10, it was impossible to return/copy the vector class. We
     * almost never need it however, as we work with cubes. This specialisation
     * thus does the job.
     *
     * @see getVolumeSize()
     */
    double  getVolumeLength(
      const tarch::la::Vector<2,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch
    );

    double  getVolumeLength(
      const tarch::la::Vector<3,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch
    );

    tarch::la::Vector<2,double>  getVolumeSize(
      const tarch::la::Vector<2,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch
    );

    tarch::la::Vector<3,double>  getVolumeSize(
      const tarch::la::Vector<3,double>&  h,
      int                                 numberOfVolumesPerAxisInPatch
    );
    /**
     * Helper routine that I need in the log statements. Applies to
     * AoS data only.
     *
     * @return Data of one volume as tuple.
     */
    std::string plotVolume(
      const double* __restrict__ Q,
      int    unknowns
    );

    /**
     * Just runs over the patch and ensures that no entry is non or infinite. In
     * ExaHyPE, we primarily work with split approaches. That is, diagonal halo
     * entries are never initialised properly: We can copy over the face-connected
     * data, but we lack information through the diagonals. This routine takes
     * this into account when it validates the entries.
     *
     * Assumes all data are held as AoS.
     *
     * @param location String that tells system from where this routine got called
     * @param minValues Is either a nullptr or it points to a double array with
     *   exactly unknowns+auxiliaryVariables entries
     */
    void validatePatch(
      const double* __restrict__ Q,
      int    unknowns,
      int    auxiliaryVariables,
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize,
      const std::string& location = "",
      bool   triggerNonCriticalAssertion = true,
      double* minValues = nullptr,
      double* maxValues = nullptr
    );

    /**
     * Plot patch.
     *
     * Usually used for debugging.
     *
     * Assumes all data are held as AoS.
     */
    std::string plotPatch(
      const double* __restrict__ Q,
      int    unknowns,
      int    auxiliaryVariables,
      int    numberOfVolumesPerAxisInPatch,
      int    haloSize
    );
  }
}

#endif
