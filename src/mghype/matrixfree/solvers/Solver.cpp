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
  const std::string      prefix = "Abstract";
  std::string::size_type index  = _name.find( prefix );
  if (index!=std::string::npos) {
    _name.erase(index, prefix.length());
  }
}


bool mghype::matrixfree::solvers::Solver::terminationCriterionHolds() {
  int residualReducedUnderThreshold = 0;
  int residualGrows   = 0;

  // The criterion holds if ALL three norms reach the tolerance 
  // or if the total numberOfResidualGrowths exceeds a limit

  if ( tarch::la::equals(_initialGlobalResidualMaxNorm,0.0) ) {
    _initialGlobalResidualMaxNorm = _globalResidualMaxNorm;
  }
  else if (_previousGlobalResidualMaxNorm * 1.2 < _globalResidualMaxNorm) {
    logWarning( "terminationCriterionHolds()", "max residual grows from " << _previousGlobalResidualMaxNorm << " to " << _globalResidualMaxNorm );
    residualGrows++;
  }
  else if (_globalResidualMaxNorm / _initialGlobalResidualMaxNorm < _tolerance) {
    residualReducedUnderThreshold++;
  }

  if ( tarch::la::equals(_initialGlobalResidualEukledianNormSquared,0.0) ) {
    _initialGlobalResidualEukledianNormSquared = _globalResidualEukledianNormSquared;
  }
  else if (_previousGlobalResidualEukledianNormSquared * 1.2 < _globalResidualEukledianNormSquared ) {
    logWarning( "terminationCriterionHolds()", "Eukledian residual squared grows from " << _previousGlobalResidualEukledianNormSquared << " to " << _globalResidualEukledianNormSquared );
    residualGrows++;
  }
  else if (_globalResidualEukledianNormSquared / _initialGlobalResidualEukledianNormSquared < _tolerance) {
    residualReducedUnderThreshold++;
  }

  if ( tarch::la::equals(_initialGlobalResidualHNormSquared,0.0) ) {
    _initialGlobalResidualHNormSquared = _globalResidualHNormSquared;
  }
  else if (_previousGlobalResidualHNormSquared * 1.2 < _globalResidualHNormSquared ) {
    logWarning( "terminationCriterionHolds()", "residual in h-norm squared grows from " << _previousGlobalResidualHNormSquared << " to " << _globalResidualHNormSquared );
    residualGrows++;
  }
  else if (_globalResidualHNormSquared / _initialGlobalResidualHNormSquared < _tolerance ) {
    residualReducedUnderThreshold++;
  }

  _previousGlobalResidualMaxNorm              = _globalResidualMaxNorm;
  _previousGlobalResidualEukledianNormSquared = _globalResidualEukledianNormSquared;
  _previousGlobalResidualHNormSquared         = _globalResidualHNormSquared;

  logDebug( "terminationCriterionHolds()", "residualReducedUnderThreshold=" << residualReducedUnderThreshold );
  static int numberOfResidualGrowths = 0;
  const int numberOfChecks = 3;

  if (residualGrows==numberOfChecks) numberOfResidualGrowths++;

  bool output = (residualReducedUnderThreshold==numberOfChecks or numberOfResidualGrowths > 5);

  if (not output) return false;

  if (residualReducedUnderThreshold==numberOfChecks)
    logInfo( "terminationCriterionHolds()", "terminating because residualReducedUnderThreshold==" << residualReducedUnderThreshold );
    
  if (numberOfResidualGrowths > 5)
    logInfo( "terminationCriterionHolds()", "terminating because numberOfResidualGrowths==" << numberOfResidualGrowths );

  return output;
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
