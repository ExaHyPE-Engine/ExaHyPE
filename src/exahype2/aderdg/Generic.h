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
        double * __restrict__                        Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double * __restrict__ F
      ) >   flux,
      const tarch::la::Vector<Dimensions,double>&  cellCentre,
      const tarch::la::Vector<Dimensions,double>&  cellSize,
      double                                       t,
      double                                       dt,
      int                                          order,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      const double * __restrict__ quadraturePoints1d,
      const double * __restrict__ quadratureWeights1d,
      double * __restrict__ Qin,
      double * __restrict__ Qout
    );


    /**
     * @param Qin  Space-time polynomial from spaceTimePredictor_GaussLegendre_AoS2d()
     * @param Qout Space polynomial for future time step (integrated prediction)
     */
    void timeIntegration_GaussLegendre_AoS2d(
      const tarch::la::Vector<Dimensions,double>&  cellCentre,
      const tarch::la::Vector<Dimensions,double>&  cellSize,
      double                                       t,
      double                                       dt,
      int                                          order,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      const double * __restrict__ quadraturePoints1d,
      const double * __restrict__ quadratureWeights1d,
      double * __restrict__ Qin,
      double * __restrict__ Qout
    );


    /**
     * Take solution and project it onto boundary
     */
    void projectSpaceTimeSolutionOntoFace_GaussLegendre_AoS2d(
      const tarch::la::Vector<Dimensions,double>&  cellCentre,
      const tarch::la::Vector<Dimensions,double>&  cellSize,
      double                                       t,
      double                                       dt,
      int                                          order,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      const double * __restrict__ quadraturePoints1d,
      const double * __restrict__ quadratureWeights1d,
      double * __restrict__ Qin,
      double * __restrict__ QoutLeft,
      double * __restrict__ QoutBottom,
      double * __restrict__ QoutRight,
      double * __restrict__ QoutTop
    );


    /**
     * Takes a predicted solution and projects the solution plus the
     * flux onto the face. Generic extension of the previous routine
     *
     * @param Qin  Space-time polynomial
     */
    void projectSpaceTimeSolutionOntoFace_GaussLegendre_AoS2d(
      std::function< void(
        double * __restrict__                        Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double * __restrict__                        F
      ) >   flux,
      const tarch::la::Vector<Dimensions,double>&  cellCentre,
      const tarch::la::Vector<Dimensions,double>&  cellSize,
      double                                       t,
      double                                       dt,
      int                                          order,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      const double * __restrict__ quadraturePoints1d,
      const double * __restrict__ quadratureWeights1d,
      double * __restrict__ Qin,
      double * __restrict__ QoutLeft,
      double * __restrict__ QoutBottom,
      double * __restrict__ QoutRight,
      double * __restrict__ QoutTop,
      double * __restrict__ normalFluxLeft,
      double * __restrict__ normalFluxBottom,
      double * __restrict__ normalFluxRight,
      double * __restrict__ normalFluxTop
    );




    /**
     * Takes a predicted solution and projects the solution plus the
     * flux onto the face. Generic extension of the previous routine
     */
    void addSpaceTimeRiemannSolutionToPrediction_GaussLegendre_AoS2d(
      const tarch::la::Vector<Dimensions,double>&  cellCentre,
      const tarch::la::Vector<Dimensions,double>&  cellSize,
      double                                       t,
      double                                       dt,
      int                                          order,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      const double * __restrict__ quadraturePoints1d,
      const double * __restrict__ quadratureWeights1d,
      double * __restrict__ riemannSolutionLeft,
      double * __restrict__ riemannSolutionBottom,
      double * __restrict__ riemannSolutionRight,
      double * __restrict__ riemannSolutionTop,
      double * __restrict__ QNew
    );


    // @todo Have to think about this one
    void solveSpaceTimeRiemannProblem_GaussLegendre_AoS2d(
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
      const tarch::la::Vector<Dimensions,double>&  faceCentre,
      const tarch::la::Vector<Dimensions,double>&  faceSize,
      double                                       t,
      double                                       dt,
      int                                          order,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      const double * __restrict__ quadraturePoints1d,
      const double * __restrict__ quadratureWeights1d,
      int                         normal,
      double * __restrict__       Q,
      double * __restrict__       fluxAlongNormal,
      double * __restrict__       riemannSolution
    );
  }
}


#endif

