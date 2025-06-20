#include "SolverStatistics.h"

#include "tarch/multicore/Lock.h"
#include "tarch/mpi/Rank.h"


tarch::logging::Log mghype::matrixfree::solvers::SolverStatistics::_log( "mghype::matrixfree::solvers::SolverStatistics" );


mghype::matrixfree::solvers::SolverStatistics::SolverStatistics(double tolerance):
  _tolerance(tolerance) {
  clearGlobalPrecondResidualUpdate();
  clearGlobalResidualAndSolutionUpdate();
}


double mghype::matrixfree::solvers::SolverStatistics::getGlobalResidualMaxNorm() const {
  return _globalResidualMaxNorm;
}

std::array<double, 2> mghype::matrixfree::solvers::SolverStatistics::getGlobalPrecondResidualNorms() const {
  return {_globalPrecondResidualMaxNorm, _globalPrecondResidualEukledianNormSquared};
}


bool mghype::matrixfree::solvers::SolverStatistics::terminationCriterionHolds() {
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


void mghype::matrixfree::solvers::SolverStatistics::clearGlobalResidualAndSolutionUpdate() {
  _globalResidualMaxNorm              = 0.0;
  _globalResidualEukledianNormSquared = 0.0;
  _globalResidualHNormSquared         = 0.0;

  _globalDeltaUMaxNorm              = 0.0;
  _globalDeltaUEukledianNormSquared = 0.0;
  _globalDeltaUHNormSquared         = 0.0;

  _minH = std::numeric_limits<double>::max();
  _maxH = 0;
}

void mghype::matrixfree::solvers::SolverStatistics::clearGlobalPrecondResidualUpdate() {
  _globalPrecondResidualMaxNorm              = 0.0;
  _globalPrecondResidualEukledianNormSquared = 0.0;
}

void mghype::matrixfree::solvers::SolverStatistics::updateGlobalResidual(
  double residual,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  _globalResidualMaxNorm               = std::max( _globalResidualMaxNorm, std::abs(residual) );
  _globalResidualEukledianNormSquared += residual * residual;
  _globalResidualHNormSquared         += residual * residual * tarch::la::volume(h);
}

void mghype::matrixfree::solvers::SolverStatistics::updateGlobalPrecondResidual(
  double residual
) {
  _globalPrecondResidualMaxNorm               = std::max( _globalPrecondResidualMaxNorm, std::abs(residual) );
  _globalPrecondResidualEukledianNormSquared += residual * residual;
}


void mghype::matrixfree::solvers::SolverStatistics::updateGlobalSolutionUpdates(
  double du,
  const tarch::la::Vector<Dimensions,double>&  h
) {
  _globalDeltaUMaxNorm               = std::max( _globalDeltaUMaxNorm, std::abs(du) );
  _globalDeltaUEukledianNormSquared += du * du;
  _globalDeltaUHNormSquared         += du * du * tarch::la::volume(h);
}


void mghype::matrixfree::solvers::SolverStatistics::updateMinMaxMeshSize(
  const tarch::la::Vector<Dimensions,double>&  h
) {
  _minH = std::min( _minH, tarch::la::min(h) );
  _maxH = std::max( _maxH, tarch::la::max(h) );
}



std::string mghype::matrixfree::solvers::SolverStatistics::toString() const {
  std::ostringstream msg;
  msg << std::scientific << std::setprecision(16);
  msg << "  |r|_max=" << _globalResidualMaxNorm
      << ", |r|_2=" << std::sqrt( _globalResidualEukledianNormSquared )
      << ", |r|_h=" << std::sqrt( _globalResidualHNormSquared )
      << ", |du|_max=" << _globalDeltaUMaxNorm
      << ", |du|_2=" << std::sqrt( _globalDeltaUEukledianNormSquared )
      << ", |du|_h=" << std::sqrt( _globalDeltaUHNormSquared )
      << ", h_min=" << _minH
      << ", h_max=" << _maxH;
  return msg.str();
}


std::string mghype::matrixfree::solvers::SolverStatistics::reportPrecondNorms() const {
  std::ostringstream msg;
  msg << std::scientific << std::setprecision(16);
  msg << "  |du_cycle|_max=" << _globalPrecondResidualMaxNorm
      << ", |du_cycle|_2=" << std::sqrt( _globalPrecondResidualEukledianNormSquared )
      << ", h_min=" << _minH
      << ", h_max=" << _maxH;
  return msg.str();
}


void mghype::matrixfree::solvers::SolverStatistics::synchroniseGlobalResidualAndSolutionUpdate() {
  #if defined(Parallel)
  double residualMax = _globalResidualMaxNorm;
  double residual2   = _globalResidualEukledianNormSquared;
  double residualH   = _globalResidualHNormSquared;
  double duMax       = _globalDeltaUMaxNorm;
  double du2         = _globalDeltaUEukledianNormSquared;
  double duH         = _globalDeltaUHNormSquared;
  double hMin        = _minH;
  double hMax        = _maxH;

  MPI_Reduce( &hMin, &_minH,             1, MPI_DOUBLE, MPI_MIN, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );
  MPI_Reduce( &hMax, &_maxH,             1, MPI_DOUBLE, MPI_MAX, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );

  MPI_Reduce( &residualMax, &_globalResidualMaxNorm,              1, MPI_DOUBLE, MPI_MAX, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );
  MPI_Reduce( &residual2,   &_globalResidualEukledianNormSquared, 1, MPI_DOUBLE, MPI_SUM, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );
  MPI_Reduce( &residualH,   &_globalResidualHNormSquared,         1, MPI_DOUBLE, MPI_SUM, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );

  MPI_Reduce( &duMax, &_globalDeltaUMaxNorm,              1, MPI_DOUBLE, MPI_MAX, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );
  MPI_Reduce( &du2,   &_globalDeltaUEukledianNormSquared, 1, MPI_DOUBLE, MPI_SUM, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );
  MPI_Reduce( &duH,   &_globalDeltaUHNormSquared,         1, MPI_DOUBLE, MPI_SUM, tarch::mpi::Rank::getGlobalMasterRank(), tarch::mpi::Rank::getInstance().getCommunicator() );
  #endif
}


void mghype::matrixfree::solvers::SolverStatistics::merge( const SolverStatistics&  statistics ) {
  static tarch::multicore::BooleanSemaphore  _semaphore;
  tarch::multicore::Lock lock( _semaphore );

  _minH = std::min(_minH, statistics._minH);
  _maxH = std::max(_maxH, statistics._maxH);

  _globalResidualMaxNorm               = std::max(_globalResidualMaxNorm, statistics._globalResidualMaxNorm);
  _globalResidualEukledianNormSquared += statistics._globalResidualEukledianNormSquared;
  _globalResidualHNormSquared         += _globalResidualHNormSquared;

  _globalDeltaUMaxNorm                 = std::max(_globalDeltaUMaxNorm, statistics._globalDeltaUMaxNorm);
  _globalDeltaUEukledianNormSquared   += _globalDeltaUEukledianNormSquared;
  _globalDeltaUHNormSquared           += _globalDeltaUHNormSquared;
}

