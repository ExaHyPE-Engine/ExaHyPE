// This file is part of the Peano's multigrid extension. For conditions of
// distribution and use, please see the copyright notice at
// www.peano-framework.org
#pragma once


#include <string>

#include "tarch/la/Vector.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"

#include "peano4/utils/Globals.h"

#include "SolverStatistics.h"


namespace mghype {
  namespace matrixfree {
    namespace solvers {
      class Solver;
    }
  }
}


/**
 * Abstract base class for all solvers
 */
class mghype::matrixfree::solvers::Solver: public mghype::matrixfree::solvers::SolverStatistics {
  public:
    /**
     * Construct the solver
     *
     * @param name Name of solver as used by toString()
     * @param tolerance Tolerance where solver should terminate. If you pass
     *   in 0.1, then the solver will terminate as soon as all residuals have
     *   been reduced to 10% of their initial value.
     */
    Solver(const std::string&  name, double tolerance);

    virtual std::string toString() const;

    /**
     * End the traversal
     *
     * Has to be implemented in subclass. Most solvers synchronise the
     * global statistics, and then print some output.
     */
    virtual void beginMeshSweep() = 0;

    /**
     * Begin the traversal
     *
     * Most solvers invoke clearGlobalResidualAndSolutionUpdate().
     */
    virtual void endMeshSweep() = 0;
  protected:

    static tarch::logging::Log _log;

    /**
     * Name of solver
     *
     * Usually only used for debuggin purposes
     */
    std::string _name;
};

