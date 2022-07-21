// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_FD_BOUNDARY_CONDITIONS_H_
#define _EXAHYPE2_FD_BOUNDARY_CONDITIONS_H_



#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>


namespace exahype2 {
  namespace fd {
    /**
     * Apply boundary conditions. Works only for a halo size of 1.
     *
     * @param unknownsPlusAuxiliaryVariables Number of unknowns plus the number
     *   of auxiliary variables. The boundary treatment routine does not
     *   distinguish how the two types of quantities are handled, so it is
     *   reasonable to only pass in the sum.
     *
     * @param faceNumber Is usually taken from marker.getSelectedFaceNumber()
     *   and is thus a number between 0 and 2d-1.
     *
     */
    void applyBoundaryConditions(
      std::function< void(
        const double* __restrict__                   Qinside,
        double * __restrict__                        Qoutside,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal
      ) >   boundaryCondition,
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  patchSize,
      double                                       t,
      double                                       dt,
      int                                          numberOfGridCellsPerPatchPerAxis,
      int                                          overlap,
      int                                          unknownsPlusAuxiliaryVariables,
      int                                          faceNumber,
      double* __restrict__                         Q
    );
  }
}


#endif
