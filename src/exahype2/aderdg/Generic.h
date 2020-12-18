// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#ifndef _EXAHYPE2_ADERDG_GENERIC_H_
#define _EXAHYPE2_ADERDG_GENERIC_H_



#include "tarch/la/Vector.h"
#include "peano4/utils/Globals.h"


#include <functional>
#include <string>

// @todo: only 2D case considered atm
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
     * Spans a cell-local space-time solution, the space-time predictor.
     *  
     * @param[in]    flux                pointwise flux function
     * @param[in]    cellCentre          centre of the cell
     * @param[in]    cellSize            extents of the cell
     * @param[in]    t                   time stamp associated with old solution
     * @param[in]    dt                  time step size
     * @param[in]    order               polynomial order
     * @param[in]    unknowns            number of unknowns / evolved quantities
     * @param[in]    auxiliaryVariables  number of auxiliary variables (material parameters etc.)
     * @param[in]    quadraturePoints1d  @todo 
     * @param[in]    quadratureWeights1d @todo
     * @param[in]    Qin  the (time-independent, spatially-varying) old solution
     * @param[inout] Qout cell-local space-time predictor polynomial (evolution in the small)
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
     * @param Qout volume flux contribution to future time step (integrated predicted flux)
     * @param cellCentre
     * @param cellSize
     * @param t
     * @param dt
     * @param order
     * @param unknowns
     * @param auxiliaryVariables
     * @param quadraturePoints1d
     * @param quadratureWeights1d
     *
     * @param Qin  Space-time polynomial from spaceTimePredictor_GaussLegendre_AoS2d()
     * @param Qout new solution if (integrated predicted flux)
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
     * Take 2D prediction and project it onto a cell's four boundaries.
     *
     * @param cellCentre
     * @param cellSize
     * @param t
     * @param dt
     * @param order
     * @param unknowns
     * @param auxiliaryVariables
     * @param quadraturePoints1d
     * @param quadratureWeights1d
     * @param Qin  Space-time polynomial
     * @param QoutLeft
     * @param QoutBottom
     * @param QoutRight
     * @param QoutTop
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
     * @param flux
     * @param cellCentre
     * @param cellSize
     * @param t
     * @param dt
     * @param order
     * @param unknowns
     * @param auxiliaryVariables
     * @param quadraturePoints1d
     * @param quadratureWeights1d
     * @param Qin  Space-time polynomial
     * @param QoutLeft
     * @param QoutBottom
     * @param QoutRight
     * @param QoutTop
     * @param normalFluxLeft
     * @param normalFluxBottom
     * @param normalFluxRight
     * @param normalFluxTop
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
     *
     * @param cellCentre
     * @param cellSize
     * @param t
     * @param dt
     * @param order
     * @param unknowns
     * @param auxiliaryVariables
     * @param quadraturePoints1d
     * @param quadratureWeights1d
     * @param riemannSolutionLeft
     * @param riemannSolutionBottom
     * @param riemannSolutionRight
     * @param riemannSolutionTop
     * @param QNew
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


    /* @todo Have to think about this one
     * @param splitRiemannSolve1d
     * @param faceCentre
     * @param faceSize
     * @param t
     * @param dt
     * @param order
     * @param unknowns
     * @param auxiliaryVariables
     * @param quadraturePoints1d
     * @param quadratureWeights1d
     * @param normal
     * @param Q
     * @param fluxAlongNormal
     * @param riemannSolution
     */
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

// templates
#include "Generic.cpph"

#endif
