#include <algorithm>


#include "AbstractCCZ4DG1RK1.h"
#include "Constants.h"


#include "tarch/multicore/Lock.h"
#include "exahype2/NonCriticalAssertions.h"


tarch::logging::Log   benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::AbstractCCZ4DG1RK1::_log( "benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::AbstractCCZ4DG1RK1" );

std::bitset<Dimensions> benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::AbstractCCZ4DG1RK1::PeriodicBC = benchmarks::exahype2::ccz4::PeriodicBC;


benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::AbstractCCZ4DG1RK1():
    
  _solverState(SolverState::GridConstruction),
  _minTimeStamp(0.0),
  _maxTimeStamp(0.0),
  _minTimeStampThisTimeStep(0.0),
  _maxTimeStampThisTimeStep(0.0),
  _minCellH(0.0),
  _maxCellH(0.0),
  _minCellHThisTimeStep(0.0),
  _maxCellHThisTimeStep(0.0),
  _minTimeStepSize(0.0),
  _maxTimeStepSize(0.0),
  _minTimeStepSizeThisTimeStep(0.0),
  _maxTimeStepSizeThisTimeStep(0.0),
  _cellUpdates(0) {
  _admissibleTimeStepSize = 0.0;
}


double benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::getMaxMeshSize() const {
  return getMaxCellSize();
}


double benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::getMinMeshSize() const {
  return getMinCellSize();
}


double benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::getMinCellSize(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _minCellHThisTimeStep : _minCellH;
}


double benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::getMaxCellSize(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _maxCellHThisTimeStep : _maxCellH;
}


double benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::getMinTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _minTimeStampThisTimeStep : _minTimeStamp;
}


double benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::getMaxTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _maxTimeStampThisTimeStep : _maxTimeStamp;
}


double benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::getMinTimeStepSize() const {
  return _minTimeStepSize;
}


double benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::getMaxTimeStepSize() const {
  return _maxTimeStepSize;
}


void benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::update(double timeStepSize, double timeStamp, double cellSize) {
  tarch::multicore::Lock lock(_semaphore);

  if ( tarch::la::greater(timeStepSize,0.0) ) {
    assertion1(timeStepSize<std::numeric_limits<double>::max()/10.0,timeStepSize);
    assertion1(timeStepSize>=0.0,timeStepSize);
    _minTimeStepSizeThisTimeStep = std::min(timeStepSize,_minTimeStepSizeThisTimeStep);
    _maxTimeStepSizeThisTimeStep = std::max(timeStepSize,_maxTimeStepSizeThisTimeStep);
    assertion2(_minTimeStepSizeThisTimeStep<=_maxTimeStepSizeThisTimeStep, _minTimeStepSizeThisTimeStep, _maxTimeStepSizeThisTimeStep );

    _cellUpdates++;

    _localMinTimeStampThisTimeStep = std::min(_localMinTimeStampThisTimeStep, timeStamp);
    _localMaxTimeStampThisTimeStep = std::max(_localMaxTimeStampThisTimeStep, timeStamp);
  }

  assertion1(timeStamp<std::numeric_limits<double>::max()/10.0,timeStamp);
  assertion1(timeStamp>=0.0,timeStamp);
  _maxTimeStampThisTimeStep = std::max(timeStamp,_maxTimeStampThisTimeStep);
  _minTimeStampThisTimeStep = std::min(timeStamp,_minTimeStampThisTimeStep);
  assertion6( tarch::la::smallerEquals(_minTimeStamp,_maxTimeStampThisTimeStep), _minTimeStamp, _minTimeStampThisTimeStep, _maxTimeStampThisTimeStep, timeStepSize, timeStamp, cellSize );

  assertion1(cellSize<std::numeric_limits<double>::max()/10.0,cellSize);
  assertion1(cellSize>0.0,cellSize);
  _maxCellHThisTimeStep = std::max(_maxCellHThisTimeStep,cellSize);
  _minCellHThisTimeStep = std::min(_minCellHThisTimeStep,cellSize);

  logDebug( "update(double,double,double)", "updated with dt=" << timeStepSize << ", t=" << timeStamp << ", h=" << cellSize << ": t(new)=" << _minTimeStampThisTimeStep );
}


benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::SolverState benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::getSolverState() const {
  return _solverState;
}



::exahype2::RefinementCommand benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::refinementCriterion(
  const double * __restrict__ Q,    // Q[59+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t
) {
  
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;


  return result;
  
}




void benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::initialCondition(
  double * __restrict__                        Q,     // Q[59+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  const tarch::la::Vector<Dimensions,int>&     index,
  bool                                         gridIsConstructred
) {
  
  for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0; 
  ::applications::exahype2::ccz4::gaugeWave(Q, x, 0);

}




void benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::boundaryConditions(
  const double * __restrict__                  Qinside,   // Qinside[59+0]
  double * __restrict__                        Qoutside,  // Qoutside[59+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  int                                          normal
) {
  
}



void benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::startGridConstructionStep() {
  assertion( _solverState == SolverState::GridConstruction );
}


void benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::finishGridConstructionStep() {
}


void benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;
}


void benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::finishGridInitialisationStep() {
}


void benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  switch (_solverState) {
    case SolverState::GridConstruction:
      _solverState = SolverState::GridInitialisation;
      break;
    case SolverState::GridInitialisation:
      _solverState = SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep0;
      break;
    case SolverState::Plotting:
      _solverState = SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep0;
      break;
  
    case SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep0:
      _solverState = SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep0;
      break;
    case SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep0:
      _solverState = SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep0;
      break;
  }

  
  if (
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxCellHThisTimeStep>0.0
    and
    isFirstGridSweepOfTimeStep()
  ) {
    logInfo( "startTimeStep()", "Solver CCZ4DG1RK1:" );
    logInfo( "startTimeStep()", "t            = " << _minTimeStampThisTimeStep );
    logInfo( "startTimeStep()", "dt           = " << getAdmissibleTimeStepSize() );
    logInfo( "startTimeStep()", "h_{min}      = " << _minCellHThisTimeStep );
    logInfo( "startTimeStep()", "h_{max}      = " << _maxCellHThisTimeStep );
    logInfo( "startTimeStep()", "lambda_{max} = " << _maxEigenvalue );
  }

  if (isFirstGridSweepOfTimeStep()) {
    _maxEigenvalue = 0.0;
  }

  if ( isFirstGridSweepOfTimeStep() ) {
    _minTimeStamp = _minTimeStampThisTimeStep;
    _maxTimeStamp = _maxTimeStampThisTimeStep;

    _minTimeStampThisTimeStep = std::numeric_limits<double>::max();
    _maxTimeStampThisTimeStep = std::numeric_limits<double>::min();

    _minTimeStepSize = _minTimeStepSizeThisTimeStep;
    _maxTimeStepSize = _maxTimeStepSizeThisTimeStep;

    _minTimeStepSizeThisTimeStep = std::numeric_limits<double>::max();
    _maxTimeStepSizeThisTimeStep = std::numeric_limits<double>::min();

    _minCellH = _minCellHThisTimeStep;
    _maxCellH = _maxCellHThisTimeStep;

    _maxCellHThisTimeStep      = 0.0;
    _minCellHThisTimeStep      = std::numeric_limits<double>::max();

    _localMinTimeStampThisTimeStep = std::numeric_limits<double>::max();
    _localMaxTimeStampThisTimeStep = std::numeric_limits<double>::min();

    _cellUpdates    = 0;
  }

  logInfo( "startTimeStep(...)", "phase " + toString(_solverState) );
}


void benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::finishTimeStep() {
  if ( isLastGridSweepOfTimeStep() ) {

  #ifdef Parallel
  double newMinTimeStamp    = _minTimeStampThisTimeStep;
  double newMaxTimeStamp    = _maxTimeStampThisTimeStep;
  double newMinCellH        = _minCellHThisTimeStep;
  double newMaxCellH        = _maxCellHThisTimeStep;
  double newMinTimeStepSize = _minTimeStepSizeThisTimeStep;
  double newMaxTimeStepSize = _maxTimeStepSizeThisTimeStep;

  double newLocalMinTimeStamp = _localMinTimeStampThisTimeStep;
  double newLocalMaxTimeStamp = _localMaxTimeStampThisTimeStep;

  int newCellUpdates = _cellUpdates;

  tarch::mpi::Rank::getInstance().allReduce(
      &newMinTimeStamp,
      &_minTimeStampThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MIN,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
  tarch::mpi::Rank::getInstance().allReduce(
      &newMaxTimeStamp,
      &_maxTimeStampThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MAX,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );

  tarch::mpi::Rank::getInstance().allReduce(
      &newMinCellH,
      &_minCellHThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MIN,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
  tarch::mpi::Rank::getInstance().allReduce(
      &newMaxCellH,
      &_maxCellHThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MAX,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );

  tarch::mpi::Rank::getInstance().allReduce(
      &newMinTimeStepSize,
      &_minTimeStepSizeThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MIN,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
  tarch::mpi::Rank::getInstance().allReduce(
      &newMaxTimeStepSize,
      &_maxTimeStepSizeThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MAX,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );

  tarch::mpi::Rank::getInstance().allReduce(
      &newLocalMinTimeStamp,
      &_localMinTimeStampThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MIN,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
  tarch::mpi::Rank::getInstance().allReduce(
      &newLocalMaxTimeStamp,
      &_localMaxTimeStampThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MAX,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );

  tarch::mpi::Rank::getInstance().allReduce(
      &newCellUpdates,
      &_cellUpdates,
      1, MPI_INT,
      MPI_SUM,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
  #endif

    if ( _minTimeStampThisTimeStep>=std::numeric_limits<double>::max()/10.0) {
      _minTimeStampThisTimeStep = 0.0;
    }
    if ( _maxTimeStampThisTimeStep>=std::numeric_limits<double>::max()/10.0 ) {
      _maxTimeStampThisTimeStep = 0.0;
    }
  }

  
  if ( isLastGridSweepOfTimeStep() ) {
    #ifdef Parallel
    double newMaxEigenvalue = _maxEigenvalue;
    tarch::mpi::Rank::getInstance().allReduce(
        &newMaxEigenvalue,
        &_maxEigenvalue,
        1,
        MPI_DOUBLE,
        MPI_MAX, 
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
        );
    #endif

    if ( tarch::la::smaller(_maxEigenvalue, 0.0 ) ) {
      ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_maxEigenvalue>=0", "invalid max eigenvalue: " + std::to_string(_maxEigenvalue) );
      // keep time step size invariant
      // _admissibleTimeStepSize = _admissibleTimeStepSize;
    }
    else if ( tarch::la::equals(_maxEigenvalue, 0.0 ) ) {
      logWarning( "finishTimeStep(...)", "maximum eigenvalue approaches 0.0. For nonlinear PDEs, this often means the PDE becomes stationary. It could also be a bug however" ); 
      _admissibleTimeStepSize = 0.0;
    }
  }

  if ( 
    isLastGridSweepOfTimeStep() 
    and 
    tarch::la::greater(_maxEigenvalue, 0.0 ) 
  ) {
    const     double waveSpeedDamping   = 1.0 / _maxEigenvalue;
    constexpr double CockburnShuDamping = 1.0 / (2.0*1-1.0);
    constexpr double meshDamping        = 1==0 ? 1.0 : 1.0 / (1 * 1);
    _admissibleTimeStepSize = 0.1 * _minCellHThisTimeStep * waveSpeedDamping * CockburnShuDamping * meshDamping;
    if ( std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize) ) {
      ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize>0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize) );
    }
    if (tarch::la::smallerEquals(_admissibleTimeStepSize,0.0,1e-10) ) {
      logWarning( "finishTimeStep(...)", "degenerated time step size of " << std::to_string(_admissibleTimeStepSize) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug in the eigenvalue computation" );
    }
  }
}


void benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  assertion( mayPlot()  or ::exahype2::hasNonCriticalAssertionBeenViolated() );
  _solverState = SolverState::Plotting;
}


void benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::finishPlottingStep() {
}


std::string benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::Plotting:
      return "plotting";
    
    case SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep0:
      return "project-on-faces-and-compute-volume-integral-of-step-0";
      break;
    case SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep0:
    return "solve-Riemann-problem-and-add-to-volume-integral-of-step-0";
  }
  return "<undef>";
}


bool benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::mayPlot() const {
  return isLastGridSweepOfTimeStep()
      or _solverState==SolverState::GridInitialisation
      or _solverState==SolverState::Plotting;
}


bool benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::isFirstGridSweepOfTimeStep() const {
  return _solverState==SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep0;
}


bool benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::isLastGridSweepOfTimeStep() const {
  return _solverState==SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep0;
}


void benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::startSimulation() {

}


void benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::finishSimulation() {

}


bool benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::cellCanUseStatelessPDETerms(
  const tarch::la::Vector<Dimensions,double>&  cellCentre,
  const tarch::la::Vector<Dimensions,double>&  cellH,
  double                                       t,
  double                                       dt
) const {
  return true;
}





double ::benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::maxEigenvalue(
  const double * __restrict__ Q, // Q[59+0],
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  int                                          normal
) {
  
  return ::applications::exahype2::ccz4::maxEigenvalue(Q, normal%Dimensions, CCZ4e, CCZ4ds, CCZ4GLMc, CCZ4GLMd );

}







void ::benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::nonconservativeProduct(
  const double * __restrict__                  Q, // Q[59+0],
  const double * __restrict__                  dQdx, // [59+0]
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  int                                          normal,
  double * __restrict__ BgradQ // BgradQ[59]
) {
  
  double dQdxSerialised[NumberOfUnknowns*3];
  for (int i=0; i<NumberOfUnknowns; i++) {
    dQdxSerialised[i+0*NumberOfUnknowns] = 0.0;
    dQdxSerialised[i+1*NumberOfUnknowns] = 0.0;
    dQdxSerialised[i+2*NumberOfUnknowns] = 0.0;

    dQdxSerialised[i+normal*NumberOfUnknowns] = dQdx[i];
  }
  ::applications::exahype2::ccz4::ncp(BgradQ, Q, dQdxSerialised, normal%Dimensions, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4mu);

}




void ::benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::sourceTerm(
  const double * __restrict__                  Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  double                                       t,
  double                                       dt,
  double * __restrict__                        S
) {
  
  
  memset(S, 0.0, NumberOfUnknowns*sizeof(double)); 
  ::applications::exahype2::ccz4::source(S,Q, CCZ4LapseType, CCZ4ds, CCZ4c, CCZ4e, CCZ4f, CCZ4bs, CCZ4sk, CCZ4xi, CCZ4itau, CCZ4eta, CCZ4k1, CCZ4k2, CCZ4k3);

  
}



















void ::benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::setMaxEigenvalue( double eigenvalue ) {
  if ( tarch::la::greater( eigenvalue, 0.0 ) ) {
    tarch::multicore::Lock lock(_semaphore);
    _maxEigenvalue = std::max(_maxEigenvalue,eigenvalue);
  }
}    


double ::benchmarks::exahype2::ccz4::AbstractCCZ4DG1RK1::getAdmissibleTimeStepSize() const {
  return _admissibleTimeStepSize;
}
    