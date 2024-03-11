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
  _tolerance(tolerance),
  _initialGlobalResidualMaxNorm(0.0),
  _initialGlobalResidualEukledianNormSquared(0.0),
  _initialGlobalResidualHNormSquared(0.0),
  _previousGlobalResidualMaxNorm(0.0),
  _previousGlobalResidualEukledianNormSquared(0.0),
  _previousGlobalResidualHNormSquared(0.0) {
}


bool mghype::matrixfree::solvers::Solver::terminationCriterionHolds() {
  int residualReduced = 0;
  int residualGrows   = 0;

  if ( tarch::la::equals(_initialGlobalResidualMaxNorm,0.0) ) {
    _initialGlobalResidualMaxNorm = _globalResidualMaxNorm;
  }
  else if (_previousGlobalResidualMaxNorm * 1.2 < _globalResidualMaxNorm) {
    logWarning( "terminationCriterionHolds()", "max residual grows from " << _previousGlobalResidualMaxNorm << " to " << _globalResidualMaxNorm );
    residualGrows++;
  }
  else if (_globalResidualMaxNorm / _initialGlobalResidualMaxNorm < _tolerance) {
    residualReduced++;
  }

  if ( tarch::la::equals(_initialGlobalResidualEukledianNormSquared,0.0) ) {
    _initialGlobalResidualEukledianNormSquared = _globalResidualEukledianNormSquared;
  }
  else if (_previousGlobalResidualEukledianNormSquared * 1.2 < _globalResidualEukledianNormSquared ) {
    logWarning( "terminationCriterionHolds()", "Eukledian residual squared grows from " << _previousGlobalResidualEukledianNormSquared << " to " << _globalResidualEukledianNormSquared );
    residualGrows++;
  }
  else if (_globalResidualEukledianNormSquared / _initialGlobalResidualEukledianNormSquared < _tolerance) {
    residualReduced++;
  }

  if ( tarch::la::equals(_initialGlobalResidualHNormSquared,0.0) ) {
    _initialGlobalResidualHNormSquared = _globalResidualHNormSquared;
  }
  else if (_previousGlobalResidualHNormSquared * 1.2 < _globalResidualHNormSquared ) {
    logWarning( "terminationCriterionHolds()", "residual in h-norm squared grows from " << _previousGlobalResidualHNormSquared << " to " << _globalResidualHNormSquared );
    residualGrows++;
  }
  else if (_globalResidualHNormSquared / _initialGlobalResidualHNormSquared < _tolerance ) {
    residualReduced++;
  }

  _previousGlobalResidualMaxNorm              = _globalResidualMaxNorm;
  _previousGlobalResidualEukledianNormSquared = _globalResidualEukledianNormSquared;
  _previousGlobalResidualHNormSquared         = _globalResidualHNormSquared;

  return residualGrows==3 or residualReduced==3;
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
