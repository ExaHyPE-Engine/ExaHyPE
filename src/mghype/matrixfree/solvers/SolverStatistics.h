// This file is part of the Peano's multigrid extension. For conditions of
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
      class SolverStatistics;
    }
  }
}


class mghype::matrixfree::solvers::SolverStatistics {
  public:
    /**
     * @param tolerance You can use the default parameter if you later on merge
     *   it into a proper class.
     */
    SolverStatistics(double tolerance = 0.0);

    bool terminationCriterionHolds();

    /**
     * Update the global residual
     *
     * Is typically to be called after we have assembled the complete residual.
     * For DG, this means the cell and face contributions from the residual.
     *
     * @param residual: Double
     *   Residual contribution from one cell. Has to be proper number, i.e. not
     *   nan or inf. We have a check in here that says that the residual should
     *   not be higher than 1e10. This is a totally arbitrary number, but
     *   sometimes helps for debugging - we have never seen such big residuals
     *   in proper runs.
     */
    void updateGlobalResidual(
      double residual,
      const tarch::la::Vector<Dimensions,double>&  h
    );

    /*
     *
     * @todo This name is not good. We should argue over a cycle
     * residual or something similar. But in any case, it is not
     * a good name.
     */
    void updateGlobalPrecondResidual(
      double residual
    );

    void updateGlobalSolutionUpdates(
      double deltaU,
      const tarch::la::Vector<Dimensions,double>&  h
    );

    void updateMinMaxMeshSize(
      const tarch::la::Vector<Dimensions,double>&  h
    );

    void clearGlobalPrecondResidualUpdate();

    double getGlobalResidualMaxNorm() const;
    std::array<double, 2> getGlobalPrecondResidualNorms() const;

    std::string toString() const;

    void merge( const SolverStatistics&  statistics );

    std::string reportPrecondNorms() const;

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
  protected:

    static tarch::logging::Log _log;

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

    double _globalPrecondResidualMaxNorm;
    double _globalPrecondResidualEukledianNormSquared;
};

