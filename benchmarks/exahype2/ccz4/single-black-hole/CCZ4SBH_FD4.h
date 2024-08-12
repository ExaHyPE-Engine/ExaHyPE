// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#pragma once

#include "AbstractCCZ4SBH_FD4.h"

#include "MulticoreOrchestration.h"

#include "tarch/logging/Log.h"


namespace benchmarks {
  namespace exahype2 {
    namespace ccz4 {
      class CCZ4SBH_FD4;

      extern TP::TwoPunctures* twoPunctures;

      void prepareTwoPunctures();
    }
  }
}


class benchmarks::exahype2::ccz4::CCZ4SBH_FD4: public benchmarks::exahype2::ccz4::AbstractCCZ4SBH_FD4 {
  private:
    static tarch::logging::Log   _log;

  public:
    /**
     * Initialise the two punctures object if required
     */
    CCZ4SBH_FD4();
    
    /**
     * Refinement criterion
     *
     * ExaHypE2 is guided by a maximum and minimum mesh (patch) size.
     * All (dynamic) AMR is constrained by these values, i.e. if your
     * mesh is coarser than the maximum mesh size, ExaHyPE 2 will
     * automatically refine. If you try to refine further than the
     * minimum mesh size, ExaHyPE 2 will ignore any refinement.
     *
     * Consequently, you are fine if you work with a regular mesh:
     * You set the maximum mesh size, and you leave everything else
     * to Peano 4/ExaHyPE 2. If you want to have an adaptive mesh,
     * use this routine to implement the refinement pattern.
     *
     * @param Q This is the (current) solution. The data is not set
     *  to a valid value throughout grid construction. That is: If
     *  t equals 0.0, you cannot assume that Q is properly initialised.
     *  Therefore, Q usually is only evaluated by dynamic AMR codes
     *  which make the solution follow
     */
    virtual ::exahype2::RefinementCommand refinementCriterion(
      const double * __restrict__ Q, // Q[59+0],
      const tarch::la::Vector<Dimensions,double>&  meshCellCentre,
      const tarch::la::Vector<Dimensions,double>&  meshCellH,
      double                                       t
    ) override;
    

    virtual void initialCondition(
      double * __restrict__ Q,
      const tarch::la::Vector<Dimensions,double>&  meshCellCentre,
      const tarch::la::Vector<Dimensions,double>&  meshCellH,
      bool                                         gridIsConstructed
    ) override;
    
    
    /**
     * Start a new time step
     *
     * First, we call the superclass' routine. This way, attributes such as
     * isFirstGridSweepOfTimeStep() are correct. The time step size dumped
     * is wrong. We might amend it later.
     *
     * If we are in the first time step, we know that all of the solvers have
     * successfully restricted their admissible time step size. Therefore, we
     * synchronise all these time step sizes.
     *
     */
    virtual void startTimeStep(
      double globalMinTimeStamp,
      double globalMaxTimeStamp,
      double globalMinTimeStepSize,
      double globalMaxTimeStepSize
    ) override;
};

