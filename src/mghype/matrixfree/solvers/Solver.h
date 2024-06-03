// This file is part of the Peano's PETSc extension. For conditions of
// distribution and use, please see the copyright notice at
// www.peano-framework.org
#pragma once


#include <string>
#include "tarch/logging/Log.h"
#include "tarch/multicore/BooleanSemaphore.h"


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
      double residualMaxNorm,
      double residualEukledianNormSquared,
      double residualHNormSquared,
      double h
    );

    virtual std::string toString() const;

    virtual void beginMeshSweep() = 0;
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

    /**
     * Semaphore for global residual values
     */
    tarch::multicore::BooleanSemaphore  _semaphore;

    /**
     * Most solvers call this one in beginMeshSweep().
     */
    void clearGlobalResidual();
};

