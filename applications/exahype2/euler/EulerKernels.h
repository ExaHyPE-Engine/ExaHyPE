// This file is part of the Peano project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _APPLICATIONS_EULER_EULER_KERNELS_H_
#define _APPLICATIONS_EULER_EULER_KERNELS_H_


#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


namespace applications {
  namespace exahype2 {
    namespace euler {
      /**
       * Apply Neumann BCs to all quantities. This includes the material
       * parameters (auxiliary ones).
       */
      void NeumannBoundaryConditions(
        const double * __restrict__                  Qinside,
        double * __restrict__                        Qoutside,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        int                                          normal,
        int                                          numberOfUnknowns,
        int                                          numberOfAuxiliaryVariables
      );


      /**
       * Dimension-generic implementation of the max eigenvalue computation
       *
       * We take the Q[numberOfUnknowns+numberOfAuxiliaryVariables] entries and
       * return one scalar over the numberOfUnknowns PDE equations.
       */
      double maxEigenvalue(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        int                                          numberOfUnknowns,
        int                                          numberOfAuxiliaryVariables
      );


      /**
       * Dimension-generic implementation of the flux computation
       *
       * We take the Q[numberOfUnknowns+numberOfAuxiliaryVariables] entries and
       * return F[numberOfUnknowns] fluxes.
       */
      void flux(
        const double * __restrict__                  Q,
        const tarch::la::Vector<Dimensions,double>&  faceCentre,
        const tarch::la::Vector<Dimensions,double>&  volumeH,
        double                                       t,
        double                                       dt,
        int                                          normal,
        int                                          numberOfUnknowns,
        int                                          numberOfAuxiliaryVariables,
        double * __restrict__                        F
      );
    }
  }
}

#endif
