// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ADERDG_GENERIC_H_
#define _EXAHYPE2_ADERDG_GENERIC_H_



#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>
#include <string>


namespace exahype2 {
  namespace aderdg {
    /**
     * Just runs over the patch and ensures that no entry is non or infinite.
     *
     * @param location String that tells system from where this routine got called
     */
    void validatePatch(
      double* __restrict__ Q,
      int    unknowns,
      int    auxiliaryVariables,
      int    order,
	  const std::string& location = ""
    );

    void spacetimePatch(
      double* __restrict__ Q,
      int    unknowns,
      int    auxiliaryVariables,
      int    order,
      const std::string& location = ""
    );

    /**
     * @param Qout  Space-time polynomial
     */
    void spaceTimePredictor_GaussLegendre_AoS2d(
      std::function< void(
        double * __restrict__ QL,
        double * __restrict__ QR,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double * __restrict__ FL,
        double * __restrict__ FR
      ) >   splitRiemannSolve1d,
      const tarch::la::Vector<Dimensions,double>&  cellCentre,
      const tarch::la::Vector<Dimensions,double>&  cellSize,
      double                                       t,
      double                                       dt,
      int                                          order,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double * __restrict__ quadraturePoints1d,
      double * __restrict__ quadratureWeights1d,
      double * __restrict__ Qin,
      double * __restrict__ Qout
    );


    /**
     * @param Qout  Space-time polynomial
     */
    void projectSpaceTimeSolutionOntoFace_GaussLegendre_AoS2d(
      const tarch::la::Vector<Dimensions,double>&  cellCentre,
      const tarch::la::Vector<Dimensions,double>&  cellSize,
      double                                       t,
      double                                       dt,
      int                                          order,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double * __restrict__ quadraturePoints1d,
      double * __restrict__ quadratureWeights1d,
      double * __restrict__ Qin,
      double * __restrict__ Qout
    );

  }
}


#endif

