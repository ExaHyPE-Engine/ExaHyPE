#include "Solver.h"

#include "tarch/la/ScalarOperations.h"
#include "tarch/multicore/Lock.h"
#include <cmath>


tarch::logging::Log mghype::matrixfree::solvers::Solver::_log( "mghype::matrixfree::solvers::Solver" );


mghype::matrixfree::solvers::Solver::Solver(
  const std::string&  name,
  double              tolerance
):
  _name(name),
  _tolerance(tolerance) {
}


bool mghype::matrixfree::solvers::Solver::terminationCriterionHolds() {
  static double initialGlobalResidualMaxNorm       = 0.0;
  static double initialGlobalResidualEukledianNormSquared = 0.0;
  static double initialGlobalResidualHNormSquared         = 0.0;

  bool result = false;

  if ( tarch::la::equals(initialGlobalResidualMaxNorm,0.0) ) {
    initialGlobalResidualMaxNorm = _globalResidualMaxNorm;
  }
  else if (initialGlobalResidualMaxNorm * 1.5 < _globalResidualMaxNorm) {
    logWarning( "terminationCriterionHolds()", "max residual grows" );
  }
  else {
    result |= _globalResidualMaxNorm / initialGlobalResidualMaxNorm < _tolerance;
  }

  if ( tarch::la::equals(initialGlobalResidualEukledianNormSquared,0.0) ) {
    initialGlobalResidualEukledianNormSquared = _globalResidualEukledianNormSquared;
  }
  else if (initialGlobalResidualEukledianNormSquared * 1.5 < _globalResidualEukledianNormSquared ) {
    logWarning( "terminationCriterionHolds()", "Eukledian residual grows" );
  }
  else {
    result |= _globalResidualEukledianNormSquared / initialGlobalResidualEukledianNormSquared < _tolerance;
  }

  if ( tarch::la::equals(initialGlobalResidualHNormSquared,0.0) ) {
    initialGlobalResidualHNormSquared = _globalResidualHNormSquared;
  }
  else if (initialGlobalResidualHNormSquared * 1.5 < _globalResidualHNormSquared ) {
    logWarning( "terminationCriterionHolds()", "residual in h-norm grows" );
  }
  else {
    result |= _globalResidualHNormSquared / initialGlobalResidualHNormSquared < _tolerance;
  }

  return result;
}


void mghype::matrixfree::solvers::Solver::clearGlobalResidual() {
  tarch::multicore::Lock lock( _semaphore );

  _globalResidualMaxNorm              = 0.0;
  _globalResidualEukledianNormSquared = 0.0;
  _globalResidualHNormSquared         = 0.0;

  _minH = std::numeric_limits<double>::max();
  _maxH = 0;
}


void mghype::matrixfree::solvers::Solver::updateGlobalResidual(
  double residualMaxNorm,
  double residualEukledianNormSquared,
  double residualHNormSquared,
  double h
) {
  tarch::multicore::Lock lock( _semaphore );

  _globalResidualMaxNorm        = std::max(_globalResidualMaxNorm, residualMaxNorm);
  _globalResidualEukledianNormSquared += residualEukledianNormSquared;
  _globalResidualHNormSquared         += residualHNormSquared;

  _minH = std::min( _minH, h );
  _maxH = std::max( _maxH, h );
}


std::string mghype::matrixfree::solvers::Solver::toString() const {
  std::ostringstream msg;
  msg << _name
      << ": |r|_max=" << _globalResidualMaxNorm
      << ", |r|_2=" << std::sqrt( _globalResidualEukledianNormSquared )
      << ", |r|_h=" << std::sqrt( _globalResidualHNormSquared )
      << ", h_min=" << _minH
      << ", h_max=" << _maxH;
  return msg.str();
}
