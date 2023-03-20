#include <algorithm>


#include "AbstractSelfSimilarInfallFV.h"
#include "Constants.h"

#if defined(UseSmartMPI)
#include "tasks/SelfSimilarInfallFVEnclaveTask.h"
#include "tarch/multicore/smartScheduling.h"
#endif

#include "tarch/multicore/multicore.h"
#include "tarch/multicore/Lock.h"
#include "exahype2/NonCriticalAssertions.h"


tarch::logging::Log   benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::AbstractSelfSimilarInfallFV::_log( "benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::AbstractSelfSimilarInfallFV" );

std::bitset<Dimensions> benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::AbstractSelfSimilarInfallFV::PeriodicBC = benchmarks::exahype2::euler::sphericalaccretionupscaling::PeriodicBC;


benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::AbstractSelfSimilarInfallFV():
  _solverState(SolverState::GridConstruction),
  _minTimeStamp(0.0),
  _maxTimeStamp(0.0),
  _minTimeStampThisTimeStep(0.0),
  _maxTimeStampThisTimeStep(0.0),
  // This way, we still compute the patch size without an overflow
  _minVolumeH(std::numeric_limits<double>::max()/NumberOfFiniteVolumesPerAxisPerPatch/2.0),
  _maxVolumeH(0.0),
  _minVolumeHThisTimeStep(std::numeric_limits<double>::max()/NumberOfFiniteVolumesPerAxisPerPatch/2.0),
  _maxVolumeHThisTimeStep(0.0),
  _minTimeStepSize(std::numeric_limits<double>::max()),
  _maxTimeStepSize(0.0),
  _minTimeStepSizeThisTimeStep(std::numeric_limits<double>::max()),
  _maxTimeStepSizeThisTimeStep(0.0),
  _patchUpdates(0) {
  _admissibleTimeStepSize = 0.0;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::getMaxMeshSize() const {
  return getMaxPatchSize();
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::getMinMeshSize() const {
  return getMinPatchSize();
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::getMinPatchSize(bool ofCurrentlyRunningGridSweep) const {
  return getMinVolumeSize(ofCurrentlyRunningGridSweep) * NumberOfFiniteVolumesPerAxisPerPatch;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::getMaxPatchSize(bool ofCurrentlyRunningGridSweep) const {
  return getMaxVolumeSize(ofCurrentlyRunningGridSweep) * NumberOfFiniteVolumesPerAxisPerPatch;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::getMinVolumeSize(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _minVolumeHThisTimeStep : _minVolumeH;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::getMaxVolumeSize(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _maxVolumeHThisTimeStep : _maxVolumeH;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::getMinTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _minTimeStampThisTimeStep : _minTimeStamp;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::getMaxTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _maxTimeStampThisTimeStep : _maxTimeStamp;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::getMinTimeStepSize() const {
  return _minTimeStepSize;
}


double benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::getMaxTimeStepSize() const {
  return _maxTimeStepSize;
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::update(double timeStepSize, double timeStamp, double patchSize) {
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
  _maxTimeStampThisTimeStep = std::max(_maxTimeStampThisTimeStep,timeStamp);
  _minTimeStampThisTimeStep = std::min(_minTimeStampThisTimeStep,timeStamp);
  assertion2(_minTimeStampThisTimeStep<=_maxTimeStampThisTimeStep, _minTimeStampThisTimeStep, _maxTimeStampThisTimeStep );

  assertion1(patchSize<std::numeric_limits<double>::max()/NumberOfFiniteVolumesPerAxisPerPatch,patchSize);
  assertion1(patchSize>0.0,patchSize);
  _maxVolumeHThisTimeStep = std::max(_maxVolumeHThisTimeStep,patchSize / NumberOfFiniteVolumesPerAxisPerPatch);
  _minVolumeHThisTimeStep = std::min(_minVolumeHThisTimeStep,patchSize / NumberOfFiniteVolumesPerAxisPerPatch);
}


benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::SolverState benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::getSolverState() const {
  return _solverState;
}











void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::startGridConstructionStep() {
  assertion( _solverState == SolverState::GridConstruction );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::finishGridConstructionStep() {
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::startGridInitialisationStep() {
  assertion( _solverState == SolverState::GridConstruction );
  _solverState = SolverState::GridInitialisation;
  logDebug( "startGridInitialisationStep(...)", "new state is " << toString(_solverState) );
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::finishGridInitialisationStep() {
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  logDebug( "startTimeStep(...)", "old state is " << toString(_solverState) );

  if (
    _solverState == SolverState::GridInitialisation
  ) {
    _solverState = SolverState::PrimaryAfterGridInitialisation;
  }
  else if (
    _solverState == SolverState::Primary
    or
    _solverState == SolverState::PrimaryAfterGridInitialisation
  ) {
    _solverState = SolverState::Secondary;
  }
  else { // plot initial condition, e.g.
    _solverState = SolverState::Primary;
  }

  
  if ( 
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    _maxVolumeH>0.0
    and
    isFirstGridSweepOfTimeStep()
  ) {
    logInfo( "startTimeStep()", "Solver SelfSimilarInfallFV:" );
    logInfo( "startTimeStep()", "t            = " << _minTimeStampThisTimeStep );
    logInfo( "startTimeStep()", "dt           = " << getAdmissibleTimeStepSize() );
    logInfo( "startTimeStep()", "h_{min}      = " << _minVolumeHThisTimeStep << " (volume size)");
    logInfo( "startTimeStep()", "h_{max}      = " << _maxVolumeHThisTimeStep << " (volume size)" );
    logInfo( "startTimeStep()", "lambda_{max} = " << _maxEigenvalue );
  }

  if ( isFirstGridSweepOfTimeStep() ) {
    _maxEigenvalue = 0.0;
  }

  if (
    _solverState == SolverState::Primary
    or
    _solverState == SolverState::PrimaryAfterGridInitialisation
  ) {
    logDebug( "startTimeStep(...)", "reset t and dt as state had been " << toString(_solverState) );

    _minTimeStamp = _minTimeStampThisTimeStep;
    _maxTimeStamp = _maxTimeStampThisTimeStep;

    _minTimeStampThisTimeStep = std::numeric_limits<double>::max();
    _maxTimeStampThisTimeStep = std::numeric_limits<double>::min();

    _minTimeStepSize = _minTimeStepSizeThisTimeStep;
    _maxTimeStepSize = _maxTimeStepSizeThisTimeStep;

    _minTimeStepSizeThisTimeStep = std::numeric_limits<double>::max();
    _maxTimeStepSizeThisTimeStep = std::numeric_limits<double>::min();

    _minVolumeH = _minVolumeHThisTimeStep;
    _maxVolumeH = _maxVolumeHThisTimeStep;

    _maxVolumeHThisTimeStep      = 0.0;
    _minVolumeHThisTimeStep      = std::numeric_limits<double>::max()/NumberOfFiniteVolumesPerAxisPerPatch/2.0;

    _patchUpdates    = 0;

    _localMinTimeStampThisTimeStep = std::numeric_limits<double>::max();
    _localMaxTimeStampThisTimeStep = std::numeric_limits<double>::min();
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::finishTimeStep() {
  if (_solverState==SolverState::Secondary) {
    #ifdef Parallel
    double newMinTimeStamp = _minTimeStampThisTimeStep;
    double newMaxTimeStamp = _maxTimeStampThisTimeStep;
    double newMinVolumeH   = _minVolumeHThisTimeStep;
    double newMaxVolumeH   = _maxVolumeHThisTimeStep;
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
      &newMinVolumeH,
      &_minVolumeHThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MIN,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
    tarch::mpi::Rank::getInstance().allReduce(
      &newMaxVolumeH,
      &_maxVolumeHThisTimeStep,
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
    _admissibleTimeStepSize = 0.1 * _minVolumeHThisTimeStep / _maxEigenvalue;
    if ( std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize) ) {
      ::exahype2::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize>0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize) );
    }
    if (tarch::la::smallerEquals(_admissibleTimeStepSize,0.0,1e-10) ) {
      logWarning( "finishTimeStep(...)", "degenerated time step size of " << std::to_string(_admissibleTimeStepSize) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug in the eigenvalue computation" );
    }
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  if ( _solverState == SolverState::GridInitialisation ) {
    _solverState = SolverState::PlottingInitialCondition;
  }
  else {
    assertion( mayPlot()  or ::exahype2::hasNonCriticalAssertionBeenViolated() );

    _solverState = SolverState::Plotting;
  }
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::finishPlottingStep() {
}


std::string benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::Primary:
      return "primary";
    case SolverState::Secondary:
      return "secondary";
    case SolverState::PlottingInitialCondition:
      return "plotting-initial-condition";
    case SolverState::PrimaryAfterGridInitialisation:
      return "primary-after-grid-initialisation";
    case SolverState::Plotting:
      return "plotting";
  }
  return "<undef>";
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::isFirstGridSweepOfTimeStep() const {
  return _solverState != SolverState::Secondary;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::isLastGridSweepOfTimeStep() const {
  return _solverState == SolverState::Secondary;
}


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::mayPlot() const {
  return _solverState == SolverState::Secondary;
}


























void ::benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::setMaxEigenvalue( double eigenvalue ) {
  if ( tarch::la::greater( eigenvalue, 0.0 ) ) {
    tarch::multicore::Lock lock(_semaphore);
    _maxEigenvalue = std::max(_maxEigenvalue,eigenvalue);
  }
}    


double ::benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::getAdmissibleTimeStepSize() const {
  return _admissibleTimeStepSize;
}
    


bool benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::patchCanUseStatelessPDETerms(
  const tarch::la::Vector<Dimensions,double>&  patchCentre,
  const tarch::la::Vector<Dimensions,double>&  patchH,
  double                                       t,
  double                                       dt
) const {
  return true;
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::startSimulation() {
  #if defined(UseSmartMPI)
  tarch::multicore::registerSmartMPITask(
    tasks::SelfSimilarInfallFVEnclaveTask::getEnclaveTaskTypeId(),
    [](smartmpi::ReceiverCallType type, int rank, int tag, MPI_Comm communicator) -> smartmpi::Task* {
      if (type==smartmpi::ReceiverCallType::ReceiveTask) {
        return tasks::SelfSimilarInfallFVEnclaveTask::receiveTask( rank, tag, communicator );
      } else if (type==smartmpi::ReceiverCallType::ReceivedTaskOutcomeForFowarding) {
        static constexpr bool isForwarding = true;
        return tasks::SelfSimilarInfallFVEnclaveTask::receiveOutcome( rank, tag, communicator, isForwarding);
      } else {
        assert(type==smartmpi::ReceiverCallType::ReceiveOutcome);
        static constexpr bool isForwarding = false;
        return tasks::SelfSimilarInfallFVEnclaveTask::receiveOutcome( rank, tag, communicator, isForwarding );
      }
    }
  );
  #endif
}


void benchmarks::exahype2::euler::sphericalaccretionupscaling::AbstractSelfSimilarInfallFV::finishSimulation() {

}