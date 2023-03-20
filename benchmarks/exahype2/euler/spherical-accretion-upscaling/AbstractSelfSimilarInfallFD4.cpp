#include <algorithm>


#include "AbstractSelfSimilarInfallFD4.h"
#include "Constants.h"

#if defined(UseSmartMPI)
#include "tasks/SelfSimilarInfallFD4EnclaveTask.h"
#include "tarch/multicore/smartScheduling.h"
#endif


#include "tarch/multicore/Lock.h"
#include "exahype2/NonCriticalAssertions.h"


tarch::logging::Log   benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::AbstractSelfSimilarInfallFD4::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::AbstractSelfSimilarInfallFD4" );

std::bitset<Dimensions> benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::AbstractSelfSimilarInfallFD4::PeriodicBC = benchmarks::exahype2::euler::sphericalaccretionupscaling::PeriodicBC;


benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::AbstractSelfSimilarInfallFD4():
  _solverState(SolverState::GridConstruction),
  _minTimeStamp(0.0),
  _maxTimeStamp(0.0),
  _minTimeStampThisTimeStep(0.0),
  _maxTimeStampThisTimeStep(0.0),
  // This way, we still compute the patch size without an overflow
  _minGridCellH(std::numeric_limits<double>::max()/NumberOfGridCellsPerPatchPerAxis/2.0),
  _maxGridCellH(0.0),
  _minGridCellHThisTimeStep(std::numeric_limits<double>::max()/NumberOfGridCellsPerPatchPerAxis/2.0),
  _maxGridCellHThisTimeStep(0.0),
  _minTimeStepSize(std::numeric_limits<double>::max()),
  _maxTimeStepSize(0.0),
  _minTimeStepSizeThisTimeStep(std::numeric_limits<double>::max()),
  _maxTimeStepSizeThisTimeStep(0.0),
  _patchUpdates(0) {
  _admissibleTimeStepSize = 0.0;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::getMaxMeshSize() const {
  return getMaxPatchSize();
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::getMinMeshSize() const {
  return getMinPatchSize();
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::getMinPatchSize(bool ofCurrentlyRunningGridSweep) const {
  return getMinGridCellSize(ofCurrentlyRunningGridSweep) * NumberOfGridCellsPerPatchPerAxis;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::getMaxPatchSize(bool ofCurrentlyRunningGridSweep) const {
  return getMaxGridCellSize(ofCurrentlyRunningGridSweep) * NumberOfGridCellsPerPatchPerAxis;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::getMinGridCellSize(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _minGridCellHThisTimeStep : _minGridCellH;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::getMaxGridCellSize(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _maxGridCellHThisTimeStep : _maxGridCellH;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::getMinTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _minTimeStampThisTimeStep : _minTimeStamp;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::getMaxTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _maxTimeStampThisTimeStep : _maxTimeStamp;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::getMinTimeStepSize() const {
  return _minTimeStepSize;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::getMaxTimeStepSize() const {
  return _maxTimeStepSize;
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::update(double timeStepSize, double timeStamp, double patchSize) {
  tarch::multicore::Lock lock(_semaphore);

  if ( tarch::la::greater(timeStepSize,0.0) ) {
    assertion1(timeStepSize<std::numeric_limits<double>::max()/10.0,timeStepSize);
    assertion1(timeStepSize>=0.0,timeStepSize);
    _minTimeStepSizeThisTimeStep = std::min(timeStepSize,_minTimeStepSizeThisTimeStep);
    _maxTimeStepSizeThisTimeStep = std::max(timeStepSize,_maxTimeStepSizeThisTimeStep);
    assertion2(_minTimeStepSizeThisTimeStep<=_maxTimeStepSizeThisTimeStep, _minTimeStepSizeThisTimeStep, _maxTimeStepSizeThisTimeStep );

    _patchUpdates++;

    _localMinTimeStampThisTimeStep = std::min(_localMinTimeStampThisTimeStep, timeStamp);
    _localMaxTimeStampThisTimeStep = std::max(_localMaxTimeStampThisTimeStep, timeStamp);
  }

  assertion1(timeStamp<std::numeric_limits<double>::max()/10.0,timeStamp);
  assertion1(timeStamp>=0.0,timeStamp);
  _maxTimeStampThisTimeStep = std::max(timeStamp,_maxTimeStampThisTimeStep);
  _minTimeStampThisTimeStep = std::min(timeStamp,_minTimeStampThisTimeStep);
  // This assertion makes sense, but if we reset the time stamp size (in couple systems) then
  // it is valid.
  // assertion6( tarch::la::smallerEquals(_minTimeStamp,_maxTimeStampThisTimeStep), _minTimeStamp, _minTimeStampThisTimeStep, _maxTimeStampThisTimeStep, timeStepSize, timeStamp, patchSize );

  assertion1(patchSize<std::numeric_limits<double>::max()/NumberOfGridCellsPerPatchPerAxis,patchSize);
  assertion1(patchSize>0.0,patchSize);
  _maxGridCellHThisTimeStep = std::max(_maxGridCellHThisTimeStep,patchSize / NumberOfGridCellsPerPatchPerAxis);
  _minGridCellHThisTimeStep = std::min(_minGridCellHThisTimeStep,patchSize / NumberOfGridCellsPerPatchPerAxis);

  logDebug( "update(double,double,double)", "updated with dt=" << timeStepSize << ", t=" << timeStamp << ", h=" << patchSize << ": t(new)=" << _minTimeStampThisTimeStep );
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::SolverState benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::getSolverState() const {
  return _solverState;
}











void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::startGridConstructionStep() {
  assertion( _solverState == SolverState::GridConstruction );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::finishGridConstructionStep() {
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::finishGridInitialisationStep() {
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  switch (_solverState) {
    case SolverState::GridConstruction:
      assertionMsg(false, "grid construction should lead into initialisation and not time stepping" );
      break;
  
    case SolverState::Plotting:
    case SolverState::PlottingAfterGridInitialisation:
      _solverState = SolverState::RungeKuttaPrimarySubStep0;
      break;
    case SolverState::GridInitialisation:
      _solverState = SolverState::RungeKuttaPrimarySubStep0AfterGridInitialisation;
      break;
    case SolverState::RungeKuttaPrimarySubStep0:
      _solverState = SolverState::RungeKuttaSecondarySubStep0;
      break;
    case SolverState::RungeKuttaSecondarySubStep0:
      _solverState = SolverState::RungeKuttaPrimarySubStep0;
      break;
    case SolverState::RungeKuttaPrimarySubStep0AfterGridInitialisation:
      _solverState = SolverState::RungeKuttaSecondarySubStep0;
      break;
  }

  
  if (
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxGridCellHThisTimeStep>0.0
    and
    isFirstGridSweepOfTimeStep()
  ) {
    logInfo( "startTimeStep()", "Solver SelfSimilarInfallFD4:" );
    logInfo( "startTimeStep()", "t            = " << _minTimeStampThisTimeStep );
    logInfo( "startTimeStep()", "dt           = " << getAdmissibleTimeStepSize() );
    logInfo( "startTimeStep()", "h_{min}      = " << _minGridCellHThisTimeStep << " (individual grid cell within a patch)");
    logInfo( "startTimeStep()", "h_{max}      = " << _maxGridCellHThisTimeStep << " (individual grid cell within a patch)" );
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

    _minGridCellH = _minGridCellHThisTimeStep;
    _maxGridCellH = _maxGridCellHThisTimeStep;

    _maxGridCellHThisTimeStep      = 0.0;
    _minGridCellHThisTimeStep      = std::numeric_limits<double>::max()/NumberOfGridCellsPerPatchPerAxis/2.0;

    _localMinTimeStampThisTimeStep = std::numeric_limits<double>::max();
    _localMaxTimeStampThisTimeStep = std::numeric_limits<double>::min();

    _patchUpdates    = 0;
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::finishTimeStep() {
  if ( isLastGridSweepOfTimeStep() ) {
    #ifdef Parallel
    double newMinTimeStamp = _minTimeStampThisTimeStep;
    double newMaxTimeStamp = _maxTimeStampThisTimeStep;
    double newMinGridCellH   = _minGridCellHThisTimeStep;
    double newMaxGridCellH   = _maxGridCellHThisTimeStep;
    double newMinTimeStepSize = _minTimeStepSizeThisTimeStep;
    double newMaxTimeStepSize = _maxTimeStepSizeThisTimeStep;

    double newLocalMinTimeStamp = _localMinTimeStampThisTimeStep;
    double newLocalMaxTimeStamp = _localMaxTimeStampThisTimeStep;

    int newPatchUpdates = _patchUpdates;

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
        &newMinGridCellH,
        &_minGridCellHThisTimeStep,
        1, MPI_DOUBLE,
        MPI_MIN,
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
        );
    tarch::mpi::Rank::getInstance().allReduce(
        &newMaxGridCellH,
        &_maxGridCellHThisTimeStep,
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
        &newPatchUpdates,
        &_patchUpdates,
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
    _admissibleTimeStepSize = 0.1 * _minGridCellHThisTimeStep / _maxEigenvalue / (2*1-1);
    if ( std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize) ) {
      ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize>0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize) );
    }
    if (tarch::la::smallerEquals(_admissibleTimeStepSize,0.0,1e-10) ) {
      logWarning( "finishTimeStep(...)", "degenerated time step size of " << std::to_string(_admissibleTimeStepSize) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug in the eigenvalue computation" );
    }
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  if ( _solverState==SolverState::GridInitialisation ) {
    _solverState = SolverState::PlottingAfterGridInitialisation;
  }
  else {
    _solverState = SolverState::Plotting;
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::finishPlottingStep() {
}


std::string benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::Plotting:
      return "plotting";
    case SolverState::PlottingAfterGridInitialisation:
      return "plotting-after-grid-initialisation";
    case SolverState::RungeKuttaPrimarySubStep0AfterGridInitialisation:
      return "runge-kutta-primary-step-0-after-grid-initialisation";
    
    case SolverState::RungeKuttaPrimarySubStep0:
      return "runge-kutta-primary-step-0";
      break;
    case SolverState::RungeKuttaSecondarySubStep0:
      return "runge-kutta-secondary-step-0";
      break;
  }
  return "<undef>";
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::mayPlot() const {
  return isLastGridSweepOfTimeStep()
      or _solverState==SolverState::GridInitialisation
      or _solverState==SolverState::Plotting;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::isFirstGridSweepOfTimeStep() const {
  return _solverState==SolverState::RungeKuttaPrimarySubStep0
      or _solverState==SolverState::RungeKuttaPrimarySubStep0AfterGridInitialisation;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::isLastGridSweepOfTimeStep() const {
  return _solverState==SolverState::RungeKuttaSecondarySubStep0;
}


























void ::benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::setMaxEigenvalue( double eigenvalue ) {
  if ( tarch::la::greater( eigenvalue, 0.0 ) ) {
    tarch::multicore::Lock lock(_semaphore);
    _maxEigenvalue = std::max(_maxEigenvalue,eigenvalue);
  }
}    


double ::benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::getAdmissibleTimeStepSize() const {
  return _admissibleTimeStepSize;
}
    



void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::startSimulation() {

}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::finishSimulation() {

}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFD4::patchCanUseStatelessPDETerms(
  const tarch::la::Vector<Dimensions,double>&  patchCentre,
  const tarch::la::Vector<Dimensions,double>&  patchH,
  double                                       t,
  double                                       dt
) const {
  return true;
}