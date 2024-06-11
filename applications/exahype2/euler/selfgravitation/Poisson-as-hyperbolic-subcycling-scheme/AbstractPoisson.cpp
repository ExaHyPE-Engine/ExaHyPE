#include <algorithm>


#include "AbstractPoisson.h"
#include "Constants.h"


#include "tarch/multicore/Lock.h"
#include "tarch/NonCriticalAssertions.h"


tarch::logging::Log   applications::exahype2::euler::selfgravitation::AbstractPoisson::AbstractPoisson::_log( "applications::exahype2::euler::selfgravitation::AbstractPoisson::AbstractPoisson" );

std::bitset<Dimensions> applications::exahype2::euler::selfgravitation::AbstractPoisson::AbstractPoisson::PeriodicBC = applications::exahype2::euler::selfgravitation::PeriodicBC;


applications::exahype2::euler::selfgravitation::AbstractPoisson::AbstractPoisson():
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


void applications::exahype2::euler::selfgravitation::AbstractPoisson::suspendSolversForOneGridSweep() {
  _solverState = SolverState::Suspended;
}


double applications::exahype2::euler::selfgravitation::AbstractPoisson::getMaxMeshSize() const {
  return getMaxPatchSize();
}


double applications::exahype2::euler::selfgravitation::AbstractPoisson::getMinMeshSize() const {
  return getMinPatchSize();
}


double applications::exahype2::euler::selfgravitation::AbstractPoisson::getMinPatchSize(bool ofCurrentlyRunningGridSweep) const {
  return getMinVolumeSize(ofCurrentlyRunningGridSweep) * NumberOfFiniteVolumesPerAxisPerPatch;
}


double applications::exahype2::euler::selfgravitation::AbstractPoisson::getMaxPatchSize(bool ofCurrentlyRunningGridSweep) const {
  return getMaxVolumeSize(ofCurrentlyRunningGridSweep) * NumberOfFiniteVolumesPerAxisPerPatch;
}


double applications::exahype2::euler::selfgravitation::AbstractPoisson::getMinVolumeSize(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _minVolumeHThisTimeStep : _minVolumeH;
}


double applications::exahype2::euler::selfgravitation::AbstractPoisson::getMaxVolumeSize(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _maxVolumeHThisTimeStep : _maxVolumeH;
}


double applications::exahype2::euler::selfgravitation::AbstractPoisson::getMinTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _minTimeStampThisTimeStep : _minTimeStamp;
}


double applications::exahype2::euler::selfgravitation::AbstractPoisson::getMaxTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _maxTimeStampThisTimeStep : _maxTimeStamp;
}


double applications::exahype2::euler::selfgravitation::AbstractPoisson::getMinTimeStepSize() const {
  return _minTimeStepSize;
}


double applications::exahype2::euler::selfgravitation::AbstractPoisson::getMaxTimeStepSize() const {
  return _maxTimeStepSize;
}


void applications::exahype2::euler::selfgravitation::AbstractPoisson::update(double timeStepSize, double timeStamp, double patchSize) {
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

  assertion1(patchSize<std::numeric_limits<double>::max()/NumberOfFiniteVolumesPerAxisPerPatch,patchSize);
  assertion1(patchSize>0.0,patchSize);
  _maxVolumeHThisTimeStep = std::max(_maxVolumeHThisTimeStep,patchSize / NumberOfFiniteVolumesPerAxisPerPatch);
  _minVolumeHThisTimeStep = std::min(_minVolumeHThisTimeStep,patchSize / NumberOfFiniteVolumesPerAxisPerPatch);
}


applications::exahype2::euler::selfgravitation::AbstractPoisson::SolverState applications::exahype2::euler::selfgravitation::AbstractPoisson::getSolverState() const {
  return _solverState;
}



::exahype2::RefinementCommand applications::exahype2::euler::selfgravitation::AbstractPoisson::refinementCriterion(
  const double * __restrict__ Q, // Q[4+1],
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  return result;
  
}









void applications::exahype2::euler::selfgravitation::AbstractPoisson::startGridConstructionStep() {
  assertion( _solverState == SolverState::GridConstruction );
}


void applications::exahype2::euler::selfgravitation::AbstractPoisson::finishGridConstructionStep() {
}


void applications::exahype2::euler::selfgravitation::AbstractPoisson::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;
}


void applications::exahype2::euler::selfgravitation::AbstractPoisson::finishGridInitialisationStep() {
}


void applications::exahype2::euler::selfgravitation::AbstractPoisson::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  if ( _solverState==SolverState::GridInitialisation ) {
    _solverState     = SolverState::TimeStepAfterGridInitialisation;
  }
  else {
    assertion( _solverState!=SolverState::GridConstruction );
    _solverState     = SolverState::TimeStep;
  }

  
  if ( 
    tarch::mpi::Rank::getInstance().isGlobalMaster() 
    and
    (_maxVolumeH>0.0 or _maxVolumeHThisTimeStep>0.0)
    and
    isFirstGridSweepOfTimeStep()
  ) {
    logInfo( "startTimeStep()", "Solver Poisson:" );
    logInfo( "startTimeStep()", "t            = " << _minTimeStampThisTimeStep );
    logInfo( "startTimeStep()", "dt           = " << getAdmissibleTimeStepSize() );
    logInfo( "startTimeStep()", "h_{min}      = " << _minVolumeHThisTimeStep << " (volume size)");
    logInfo( "startTimeStep()", "h_{max}      = " << _maxVolumeHThisTimeStep << " (volume size)" );
    logInfo( "startTimeStep()", "lambda_{max} = " << _maxEigenvalue );
  }

  if ( isFirstGridSweepOfTimeStep() ) {
    _maxEigenvalue = 0.0;
  }

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

  _localMinTimeStampThisTimeStep = std::numeric_limits<double>::max();
  _localMaxTimeStampThisTimeStep = std::numeric_limits<double>::min();

  _patchUpdates    = 0;
}


void applications::exahype2::euler::selfgravitation::AbstractPoisson::finishTimeStep() {
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
      ::tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "_maxEigenvalue>=0", "invalid max eigenvalue: " + std::to_string(_maxEigenvalue) );
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
    _admissibleTimeStepSize = 0.001 * _minVolumeHThisTimeStep / _maxEigenvalue;
    if ( std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize) ) {
      ::tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize>0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize) );
    }
    if (tarch::la::smallerEquals(_admissibleTimeStepSize,0.0,1e-10) ) {
      logWarning( "finishTimeStep(...)", "degenerated time step size of " << std::to_string(_admissibleTimeStepSize) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug in the eigenvalue computation" );
    }
  }
}


void applications::exahype2::euler::selfgravitation::AbstractPoisson::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  if ( _solverState== SolverState::GridInitialisation) {
    _solverState = SolverState::PlottingAfterGridInitialisation;
  }
  else {
    _solverState = SolverState::Plotting;
  }
}


void applications::exahype2::euler::selfgravitation::AbstractPoisson::finishPlottingStep() {
}


std::string applications::exahype2::euler::selfgravitation::AbstractPoisson::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::TimeStep:
      return "time-step";
    case SolverState::Plotting:
      return "plotting";
    case SolverState::TimeStepAfterGridInitialisation:
      return "time-step-after-grid-initialisation";
    case SolverState::PlottingAfterGridInitialisation:
      return "plotting-after-grid-initialisation";
  }
  return "<undef>";
}


bool applications::exahype2::euler::selfgravitation::AbstractPoisson::mayPlot() const {
  return true;
}


bool applications::exahype2::euler::selfgravitation::AbstractPoisson::isFirstGridSweepOfTimeStep() const {
  return true;
}


bool applications::exahype2::euler::selfgravitation::AbstractPoisson::isLastGridSweepOfTimeStep() const {
  return true;
}

























void ::applications::exahype2::euler::selfgravitation::AbstractPoisson::setMaxEigenvalue( double eigenvalue ) {
  if ( tarch::la::greater( eigenvalue, 0.0 ) ) {
    tarch::multicore::Lock lock(_semaphore);
    _maxEigenvalue = std::max(_maxEigenvalue,eigenvalue);
  }
}    


double ::applications::exahype2::euler::selfgravitation::AbstractPoisson::getAdmissibleTimeStepSize() const {
  return _admissibleTimeStepSize;
}
    



void applications::exahype2::euler::selfgravitation::AbstractPoisson::startSimulation() {

}


void applications::exahype2::euler::selfgravitation::AbstractPoisson::finishSimulation() {

}
