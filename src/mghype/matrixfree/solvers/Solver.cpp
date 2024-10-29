#include "Solver.h"

#include "tarch/la/ScalarOperations.h"
#include "tarch/multicore/Lock.h"
#include "tarch/mpi/Rank.h"
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
  _previousGlobalResidualHNormSquared(0.0),
  _globalDeltaUMaxNorm(0.0),
  _globalDeltaUEukledianNormSquared(0.0),
  _globalDeltaUHNormSquared(0.0),
  _minH(std::numeric_limits<double>::max()),
  _maxH(0.0) {
  const std::string      prefix = "Abstract";
  std::string::size_type index  = _name.find( prefix );
  if (index!=std::string::npos) {
    _name.erase(index, prefix.length());
  }
}


double mghype::matrixfree::solvers::Solver::getGlobalResidualMaxNorm() const {
  return _globalResidualMaxNorm;
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

  _previousGlobalResidualMaxNorm              = _globalResidualMaxNorm;
  _previousGlobalResidualEukledianNormSquared = _globalResidualEukledianNormSquared;
  _previousGlobalResidualHNormSquared         = _globalResidualHNormSquared;

  logDebug( "terminationCriterionHolds()", "residualReducedUnderThreshold=" << residualReducedUnderThreshold );
  static int numberOfResidualGrowths = 0;
  const int numberOfChecks = 1; // reset to 3 for all-norms check and uncomment the blocks above... 
  const int residualGrowthsLimit = 50; // stop if the residual norm increases over several consecutive iterations

  if (residualGrows==numberOfChecks) numberOfResidualGrowths++;

  bool output = (residualReducedUnderThreshold==numberOfChecks or numberOfResidualGrowths > residualGrowthsLimit);

  if (residualReducedUnderThreshold==numberOfChecks)
    logInfo( "terminationCriterionHolds()", "ONLY CHECKING MAX-NORM: terminating because residualReducedUnderThreshold==" << residualReducedUnderThreshold );
    
  if (numberOfResidualGrowths > residualGrowthsLimit)
    logInfo( "terminationCriterionHolds()", "terminating because numberOfResidualGrowths==" << numberOfResidualGrowths );

  return output;
}


void mghype::matrixfree::solvers::Solver::clearGlobalResidualAndSolutionUpdate() {
  tarch::multicore::Lock lock( _semaphore );

  _globalResidualMaxNorm              = 0.0;
  _globalResidualEukledianNormSquared = 0.0;
  _globalResidualHNormSquared         = 0.0;

  _globalDeltaUMaxNorm              = 0.0;
  _globalDeltaUEukledianNormSquared = 0.0;
  _globalDeltaUHNormSquared         = 0.0;

  // Commenting out as we will set this during init and leave it alone...
  // _minH = std::numeric_limits<double>::max();
  // _maxH = 0;
}


void mghype::matrixfree::solvers::Solver::updateGlobalResidual(
  double residual,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  tarch::multicore::Lock lock( _semaphore );

  _globalResidualMaxNorm               = std::max( _globalResidualMaxNorm, std::abs(residual) );
  _globalResidualEukledianNormSquared += residual * residual;
  _globalResidualHNormSquared         += residual * residual * tarch::la::volume(h);
}


void mghype::matrixfree::solvers::Solver::updateGlobalSolutionUpdates(
  double du,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  tarch::multicore::Lock lock( _semaphore );

  _globalDeltaUMaxNorm               = std::max( _globalDeltaUMaxNorm, std::abs(du) );
  _globalDeltaUEukledianNormSquared += du * du;
  _globalDeltaUHNormSquared         += du * du * tarch::la::volume(h);
}


void mghype::matrixfree::solvers::Solver::updateMinMaxMeshSize(
  const tarch::la::Vector<Dimensions,double>&  h
) {
  _minH = std::min( _minH, tarch::la::min(h) );
  _maxH = std::max( _maxH, tarch::la::max(h) );
}


std::string mghype::matrixfree::solvers::Solver::toString() const {
  std::ostringstream msg;
  msg << _name
      << ": |r|_max=" << _globalResidualMaxNorm
      << ", |r|_2=" << std::sqrt( _globalResidualEukledianNormSquared )
      << ", |r|_h=" << std::sqrt( _globalResidualHNormSquared )
      << ", |du|_max=" << _globalDeltaUMaxNorm
      << ", |du|_2=" << std::sqrt( _globalDeltaUEukledianNormSquared )
      << ", |du|_h=" << std::sqrt( _globalDeltaUHNormSquared )
      << ", h_min=" << _minH
      << ", h_max=" << _maxH;
  return msg.str();
}


void mghype::matrixfree::solvers::Solver::synchroniseGlobalResidualAndSolutionUpdate() {
  #if defined(Parallel)
  double residualMax = _globalResidualMaxNorm;
  double residual2   = _globalResidualEukledianNormSquared;
  double residualH   = _globalResidualHNormSquared;
  double duMax       = _globalDeltaUMaxNorm;
  double du2         = _globalDeltaUEukledianNormSquared;
  double duH         = _globalDeltaUHNormSquared;
  double hMin        = _minH;
  double hMax        = _maxH;

  MPI_Reduce( &residualMax, &_globalResidualMaxNorm,             1, MPI_DOUBLE, MPI_MAX, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );
  MPI_Reduce( &residual2,   &_globalResidualEukledianNormSquared, 1, MPI_DOUBLE, MPI_SUM, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );
  MPI_Reduce( &residualH,   &_globalResidualHNormSquared,        1, MPI_DOUBLE, MPI_SUM, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );

  MPI_Reduce( &duMax, &_globalDeltaUMaxNorm,             1, MPI_DOUBLE, MPI_MAX, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );
  MPI_Reduce( &du2,   &_globalDeltaUEukledianNormSquared, 1, MPI_DOUBLE, MPI_SUM, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );
  MPI_Reduce( &duH,   &_globalDeltaUHNormSquared,        1, MPI_DOUBLE, MPI_SUM, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );

  MPI_Reduce( &hMin, &_minH,             1, MPI_DOUBLE, MPI_MIN, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );
  MPI_Reduce( &hMax, &_maxH,             1, MPI_DOUBLE, MPI_MAX, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );
  #endif
}
