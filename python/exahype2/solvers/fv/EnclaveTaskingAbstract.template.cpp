#include <algorithm>


#include "{{CLASSNAME}}.h"
#include "Constants.h"


#include "tarch/multicore/Lock.h"
#include "exahype2/NonCriticalAssertions.h"


tarch::logging::Log   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}" );

std::bitset<Dimensions> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}::PeriodicBC = {{NAMESPACE | join("::")}}::PeriodicBC;


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _solverState(SolverState::GridConstruction),
  _minTimeStamp(0.0),
  _maxTimeStamp(0.0),
  _minVolumeH(0.0),
  _maxVolumeH(0.0),
  _minVolumeHFromPreviousTimeStep(0.0),
  _maxVolumeHFromPreviousTimeStep(0.0),
  _minTimeStepSize(0.0),
  _maxTimeStepSize(0.0),
  _patchUpdates(0) {
  {{CONSTRUCTOR_IMPLEMENTATION}}
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxMeshSize() const {
  return getMaxPatchSize();
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinMeshSize() const {
  return getMinPatchSize();
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinPatchSize(bool currentTimeStep) const {
  return getMinVolumeSize(currentTimeStep) * NumberOfFiniteVolumesPerAxisPerPatch;
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxPatchSize(bool currentTimeStep) const {
  return getMaxVolumeSize(currentTimeStep) * NumberOfFiniteVolumesPerAxisPerPatch;
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinVolumeSize(bool currentTimeStep) const {
  return currentTimeStep ? _minVolumeH : _minVolumeHFromPreviousTimeStep;
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxVolumeSize(bool currentTimeStep) const {
  return currentTimeStep ? _maxVolumeH : _maxVolumeHFromPreviousTimeStep;
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStamp() const {
  return _minTimeStamp;
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStamp() const {
  return _maxTimeStamp;
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStepSize() const {
  return _minTimeStepSize;
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStepSize() const {
  return _maxTimeStepSize;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::update(double timeStepSize, double timeStamp, double patchSize) {
  tarch::multicore::Lock lock(_semaphore);

  if ( tarch::la::greater(timeStepSize,0.0) ) {
    assertion1(timeStepSize<std::numeric_limits<double>::max()/10.0,timeStepSize);
    assertion1(timeStepSize>=0.0,timeStepSize);
    _minTimeStepSize = std::min(timeStepSize,_minTimeStepSize);
    _maxTimeStepSize = std::max(timeStepSize,_maxTimeStepSize);
    assertion2(_minTimeStepSize<=_maxTimeStepSize, _minTimeStepSize, _maxTimeStepSize );
  }

  assertion1(timeStamp<std::numeric_limits<double>::max()/10.0,timeStamp);
  assertion1(timeStamp>=0.0,timeStamp);
  _maxTimeStamp = std::max(timeStamp,_maxTimeStamp);
  _minTimeStamp = std::min(timeStamp,_minTimeStamp);
  assertion2(_minTimeStamp<=_maxTimeStamp, _minTimeStamp, _maxTimeStamp );

  assertion1(patchSize<std::numeric_limits<double>::max()/10.0,patchSize);
  assertion1(patchSize>0.0,patchSize);
  _maxVolumeH = std::max(_maxVolumeH,patchSize / NumberOfFiniteVolumesPerAxisPerPatch);
  _minVolumeH = std::min(_minVolumeH,patchSize / NumberOfFiniteVolumesPerAxisPerPatch);

  _patchUpdates++;
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::SolverState {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getSolverState() const {
  return _solverState;
}


{% if REFINEMENT_CRITERION_IMPLEMENTATION!="<user-defined>" %}
::exahype2::RefinementCommand {{NAMESPACE | join("::")}}::{{CLASSNAME}}::refinementCriterion(
  const double * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t
) {
  {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<empty>" %}
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  if ( tarch::la::greater(volumeH,MaxVolumeH ) {
    result = ::exahype2::RefinementCommand::Refine;
  }

  return result;
  {% else %}
  {{REFINEMENT_CRITERION_IMPLEMENTATION}}
  {% endif %}
}
{% endif %}


{% if INITIAL_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initialCondition(
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  volumeCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  bool                                         gridIsConstructred
) {
  {{INITIAL_CONDITIONS_IMPLEMENTATION}}
}
{% endif %}


{% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::boundaryConditions(
  const double * __restrict__                  Qinside,   // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  double * __restrict__                        Qoutside,  // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  faceCentre,
  const tarch::la::Vector<Dimensions,double>&  volumeH,
  double                                       t,
  int                                          normal
) {
  {{BOUNDARY_CONDITIONS_IMPLEMENTATION}}
}
{% endif %}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridConstructionStep() {
  assertion( _solverState == SolverState::GridConstruction );
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridConstructionStep() {
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridInitialisationStep() {
  assertion( _solverState == SolverState::GridConstruction );
  _solverState = SolverState::GridInitialisation;
  logDebug( "startGridInitialisationStep(...)", "new state is " << toString(_solverState) );
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridInitialisationStep() {
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  {{START_TIME_STEP_IMPLEMENTATION}}

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
    logDebug( "startTimeStep(...)", "reset t and dt as state had been " << toString(_solverState) );
    _minTimeStamp    = std::numeric_limits<double>::max();
    _maxTimeStamp    = std::numeric_limits<double>::min();
    _minTimeStepSize = std::numeric_limits<double>::max();
    _maxTimeStepSize = std::numeric_limits<double>::min();

    _minVolumeHFromPreviousTimeStep = _minVolumeH;
    _maxVolumeHFromPreviousTimeStep = _maxVolumeH;

    _maxVolumeH      = 0.0;
    _minVolumeH      = std::numeric_limits<double>::max();
    _patchUpdates    = 0;

    _solverState = SolverState::Secondary;
  }
  else { // plot initial condition, e.g.
    _solverState = SolverState::Primary;
  }

  logDebug( "startTimeStep(...)", "new state is " << toString(_solverState) );
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
  #ifdef Parallel
  double newMinTimeStamp = _minTimeStamp;
  double newMaxTimeStamp = _maxTimeStamp;
  double newMinVolumeH   = _minVolumeH;
  double newMaxVolumeH   = _maxVolumeH;
  double newMinTimeStepSize = _minTimeStepSize;
  double newMaxTimeStepSize = _maxTimeStepSize;
  int    newPatchUpdates    = _patchUpdates;

  tarch::mpi::Rank::getInstance().allReduce(
      &newMinTimeStamp,
      &_minTimeStamp,
      1, MPI_DOUBLE,
      MPI_MIN,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
  tarch::mpi::Rank::getInstance().allReduce(
      &newMaxTimeStamp,
      &_maxTimeStamp,
      1, MPI_DOUBLE,
      MPI_MAX,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );

  tarch::mpi::Rank::getInstance().allReduce(
      &newMinVolumeH,
      &_minVolumeH,
      1, MPI_DOUBLE,
      MPI_MIN,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
  tarch::mpi::Rank::getInstance().allReduce(
      &newMaxVolumeH,
      &_maxVolumeH,
      1, MPI_DOUBLE,
      MPI_MAX,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );

  tarch::mpi::Rank::getInstance().allReduce(
      &newMinTimeStepSize,
      &_minTimeStepSize,
      1, MPI_DOUBLE,
      MPI_MIN,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
      );
  tarch::mpi::Rank::getInstance().allReduce(
      &newMaxTimeStepSize,
      &_maxTimeStepSize,
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

  {{FINISH_TIME_STEP_IMPLEMENTATION}}
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  if ( _solverState == SolverState::GridInitialisation ) {
    _solverState = SolverState::PlottingInitialCondition;
  }
  else {
    _solverState = SolverState::Plotting;
  }
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishPlottingStep() {
}


std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(SolverState state) {
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




{{ABSTRACT_SOLVER_USER_DEFINITIONS}}
