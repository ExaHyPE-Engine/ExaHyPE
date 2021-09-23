// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_PATCH_UTILS_H_
#define _EXAHYPE2_PATCH_UTILS_H_


#include <vector>

#include "tarch/la/Vector.h"
#include "tarch/multicore/multicore.h"

#include "peano4/grid/GridControlEvent.h"
#include "peano4/utils/Globals.h"


namespace exahype2 {
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
}

#endif
