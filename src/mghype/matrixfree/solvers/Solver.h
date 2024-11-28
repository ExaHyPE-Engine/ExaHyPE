// This file is part of the Peano's PETSc extension. For conditions of
// distribution and use, please see the copyright notice at
// www.peano-framework.org
#pragma once


#include <string>

#include "tarch/la/Vector.h"
#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"

#include "peano4/utils/Globals.h"


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
class mghype::matrixfree::solvers::Solver {
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

    virtual bool terminationCriterionHolds();

    void updateGlobalResidual(
      double residual,
      const tarch::la::Vector<Dimensions,double>&  h
    );
    void updateGlobalSolutionUpdates(
      double deltaU,
      const tarch::la::Vector<Dimensions,double>&  h
    );
    void updateMinMaxMeshSize(
      const tarch::la::Vector<Dimensions,double>&  h
    );

    double getGlobalResidualMaxNorm() const;

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

    /**
     * @see terminationCriterionHolds()
     */
    const double _tolerance;

    /**
     * Observed mesh width
     */
    double     _minH;

    /**
     * Observed mesh width
     */
    double     _maxH;

    double     _globalResidualMaxNorm;
    double     _globalResidualEukledianNormSquared;
    double     _globalResidualHNormSquared;

    double _initialGlobalResidualMaxNorm;
    double _initialGlobalResidualEukledianNormSquared;
    double _initialGlobalResidualHNormSquared;

    double _previousGlobalResidualMaxNorm;
    double _previousGlobalResidualEukledianNormSquared;
    double _previousGlobalResidualHNormSquared;

    double     _globalDeltaUMaxNorm;
    double     _globalDeltaUEukledianNormSquared;
    double     _globalDeltaUHNormSquared;

    /**
     * Semaphore for global residual values
     */
    tarch::multicore::BooleanSemaphore  _semaphore;

    /**
     * Clear the global mesh stats
     *
     * Most solvers call this one in beginMeshSweep().
     */
    void clearGlobalResidualAndSolutionUpdate();

    /**
     * Synchronise the global stats between MPI ranks
     *
     * Becomes nop if there is no MPI. The routine also takes care of the
     * minimum and maximum mesh size tracked.
     */
    void synchroniseGlobalResidualAndSolutionUpdate();
};

