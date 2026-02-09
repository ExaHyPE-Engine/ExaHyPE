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
     * Delegate to benchmarks::exahype2::ccz4::refinementCriterion()
     * for time t=0, i.e. for the initial condition.
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

