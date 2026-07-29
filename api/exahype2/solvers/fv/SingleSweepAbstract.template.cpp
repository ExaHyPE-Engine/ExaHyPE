// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "repositories/SolverRepository.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Lock.h"

std::bitset<DIMENSIONS> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::PeriodicBC = {{NAMESPACE | join("::")}}::PeriodicBC;

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _solverState(SolverState::GridConstruction),
  _minTimeStamp(0.0),
  _maxTimeStamp(0.0),
  _minTimeStampThisTimeStep(0.0),
  _maxTimeStampThisTimeStep(0.0),
  // This way, we still compute the patch size without an overflow
  _minVolumeH(std::numeric_limits<double>::max() / NumberOfFiniteVolumesPerAxisPerPatch / 2.0),
  _maxVolumeH(0.0),
  _minVolumeHThisTimeStep(std::numeric_limits<double>::max() / NumberOfFiniteVolumesPerAxisPerPatch / 2.0),
  _maxVolumeHThisTimeStep(0.0),
  _minTimeStepSize(std::numeric_limits<double>::max()),
  _maxTimeStepSize(0.0),
  _minTimeStepSizeThisTimeStep(std::numeric_limits<double>::max()),
  _maxTimeStepSizeThisTimeStep(0.0) {
  {{CONSTRUCTOR_IMPLEMENTATION}}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::suspendSolversForOneGridSweep() {
  _solverState = SolverState::Suspended;
}

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getNumberOfDoFs() const {
  return BasePowerExponent<NumberOfFiniteVolumesPerAxisPerPatch, DIMENSIONS>::value * (NumberOfUnknowns + NumberOfAuxiliaryVariables);
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinPatchSize() const {
  return getMinCellSize() * NumberOfFiniteVolumesPerAxisPerPatch;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxPatchSize() const {
  return getMaxCellSize() * NumberOfFiniteVolumesPerAxisPerPatch;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinCellSize() const {
  return _minVolumeH;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxCellSize() const {
  return _maxVolumeH;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStamp() const {
  return _minTimeStampThisTimeStep < std::numeric_limits<double>::max() / 10.0 ? _minTimeStampThisTimeStep : _minTimeStamp;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStamp() const {
  return _maxTimeStampThisTimeStep > std::numeric_limits<double>::min() * 10.0 ? _maxTimeStampThisTimeStep : _maxTimeStamp;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStepSize() const {
  return _minTimeStepSizeThisTimeStep < std::numeric_limits<double>::max() / 10.0 ? _minTimeStepSizeThisTimeStep : _minTimeStepSize;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStepSize() const {
  return _maxTimeStepSizeThisTimeStep > std::numeric_limits<double>::min() * 10.0 ? _maxTimeStepSizeThisTimeStep : _maxTimeStepSize;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::update(double timeStepSize, double timeStamp, double minVolumeSize, double maxVolumeSize) {
  tarch::multicore::Lock lock(_semaphore);

  if (tarch::la::greater(timeStepSize, 0.0)) {
    assertion(timeStepSize < std::numeric_limits<double>::max() / 10.0, timeStepSize);
    assertion(timeStepSize >= 0.0, timeStepSize);
    _minTimeStepSizeThisTimeStep = std::min(timeStepSize, _minTimeStepSizeThisTimeStep);
    _maxTimeStepSizeThisTimeStep = std::max(timeStepSize, _maxTimeStepSizeThisTimeStep);
    assertion(tarch::la::smallerEquals(_minTimeStepSizeThisTimeStep, _maxTimeStepSizeThisTimeStep), _minTimeStepSizeThisTimeStep, _maxTimeStepSizeThisTimeStep);

    _localMinTimeStampThisTimeStep = std::min(_localMinTimeStampThisTimeStep, timeStamp);
    _localMaxTimeStampThisTimeStep = std::max(_localMaxTimeStampThisTimeStep, timeStamp);
  }

  assertion(timeStamp < std::numeric_limits<double>::max() / 10.0, timeStamp);
  assertion(timeStamp >= 0.0, timeStamp);
  _minTimeStampThisTimeStep = std::min(timeStamp, _minTimeStampThisTimeStep);
  _maxTimeStampThisTimeStep = std::max(timeStamp, _maxTimeStampThisTimeStep);
  assertion(tarch::la::smallerEquals(_minTimeStampThisTimeStep, _maxTimeStampThisTimeStep), _minTimeStamp, _maxTimeStamp, _minTimeStampThisTimeStep, _maxTimeStampThisTimeStep, timeStepSize, timeStamp, minVolumeSize, maxVolumeSize);

  assertion(maxVolumeSize < std::numeric_limits<double>::max() / NumberOfFiniteVolumesPerAxisPerPatch, maxVolumeSize);
  assertion(minVolumeSize > 0.0, minVolumeSize);
  _minVolumeHThisTimeStep = std::min(_minVolumeHThisTimeStep, minVolumeSize / NumberOfFiniteVolumesPerAxisPerPatch);
  _maxVolumeHThisTimeStep = std::max(_maxVolumeHThisTimeStep, maxVolumeSize / NumberOfFiniteVolumesPerAxisPerPatch);
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::SolverState {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getSolverState() const {
  return _solverState;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridConstructionStep() {
  assertion(_solverState == SolverState::GridConstruction);
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridConstructionStep() {
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridInitialisationStep() {
  // Initialisation re-enters after every completed mesh update of its
  // initialise-drain loop, so the state may already sit at GridInitialisation.
  assertion(_solverState == SolverState::GridConstruction or _solverState == SolverState::GridInitialisation);
  _solverState = SolverState::GridInitialisation;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridInitialisationStep() {
  // From here on every completed sweep has sent a face-data batch, which the
  // following sweep's boundary exchange consumes.
  _hasStoredFaceData = true;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startTimeStep() {
  if (_solverState == SolverState::Suspended) {
    // Resume solving without resetting the per-time-step accumulators below, so
    // that a suspended solver does not nullify its eigenvalue / time step sizes.
    _solverState = SolverState::TimeStep;
    return;
  }

  _solverState = SolverState::TimeStep;
  _meshRefinementRanSinceLastTimeStep = false;

  {{START_TIME_STEP_IMPLEMENTATION}}

  _minTimeStampThisTimeStep = std::numeric_limits<double>::max();
  _maxTimeStampThisTimeStep = std::numeric_limits<double>::min();

  _minTimeStepSizeThisTimeStep = std::numeric_limits<double>::max();
  _maxTimeStepSizeThisTimeStep = std::numeric_limits<double>::min();

  _maxVolumeHThisTimeStep = 0.0;
  _minVolumeHThisTimeStep = std::numeric_limits<double>::max() / NumberOfFiniteVolumesPerAxisPerPatch / 2.0;

  _localMinTimeStampThisTimeStep = std::numeric_limits<double>::max();
  _localMaxTimeStampThisTimeStep = std::numeric_limits<double>::min();
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
  if (isLastGridSweepOfTimeStep()) {
    // The upper half holds the negated maxima, so one minimum reduction yields both
    // extrema. Negating an IEEE double is exact, so this matches a maximum bit for bit.
    const double localExtrema[8] = {
       _minTimeStampThisTimeStep,
       _minVolumeHThisTimeStep,
       _minTimeStepSizeThisTimeStep,
       _localMinTimeStampThisTimeStep,
      -_maxTimeStampThisTimeStep,
      -_maxVolumeHThisTimeStep,
      -_maxTimeStepSizeThisTimeStep,
      -_localMaxTimeStampThisTimeStep
    };
    double         globalExtrema[8] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
    const MPI_Comm communicator     = tarch::mpi::Rank::getInstance().getCommunicator();

    tarch::mpi::allreduce(
      localExtrema,
      globalExtrema,
      8, MPI_DOUBLE,
      MPI_MIN,
      communicator
    );

    _minTimeStampThisTimeStep      = globalExtrema[0];
    _minVolumeHThisTimeStep        = globalExtrema[1];
    _minTimeStepSizeThisTimeStep   = globalExtrema[2];
    _localMinTimeStampThisTimeStep = globalExtrema[3];

    _maxTimeStampThisTimeStep      = -globalExtrema[4];
    _maxVolumeHThisTimeStep        = -globalExtrema[5];
    _maxTimeStepSizeThisTimeStep   = -globalExtrema[6];
    _localMaxTimeStampThisTimeStep = -globalExtrema[7];
  }

  if (_minTimeStampThisTimeStep >= std::numeric_limits<double>::max() / 10.0) {
    _minTimeStampThisTimeStep = 0.0;
  }
  if (_maxTimeStampThisTimeStep <= std::numeric_limits<double>::min() * 10.0) {
    _maxTimeStampThisTimeStep = _maxTimeStamp;
  }

  {{FINISH_TIME_STEP_IMPLEMENTATION}}

  if (isLastGridSweepOfTimeStep()) {
    _minTimeStamp    = _minTimeStampThisTimeStep;
    _maxTimeStamp    = _maxTimeStampThisTimeStep;
    _minTimeStepSize = _minTimeStepSizeThisTimeStep;
    _maxTimeStepSize = _maxTimeStepSizeThisTimeStep;
    _minVolumeH      = _minVolumeHThisTimeStep;
    _maxVolumeH      = _maxVolumeHThisTimeStep;
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startPlottingStep() {
  assertion(mayPlot() or tarch::hasNonCriticalAssertionBeenViolated());
  _solverState = SolverState::Plotting;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishPlottingStep() {}

std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "GridConstruction";
    case SolverState::GridInitialisation:
      return "GridInitialisation";
    case SolverState::TimeStep:
      return "TimeStep";
    case SolverState::Plotting:
      return "Plotting";
    case SolverState::Suspended:
      return "Suspended";
  }
  return "<undef>";
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mayPlot() const {
  return
    _solverState == SolverState::GridInitialisation
    or
    // The solver rests in TimeStep between time steps, so we may plot then.
    isLastGridSweepOfTimeStep();
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isFirstGridSweepOfTimeStep() const {
  return _solverState == SolverState::TimeStep;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isLastGridSweepOfTimeStep() const {
  // The single-sweep solver does all its work in the TimeStep state.
  return _solverState == SolverState::TimeStep;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSimulation() {
  {{START_SIMULATION_IMPLEMENTATION}}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishSimulation() {
  {{FINISH_SIMULATION_IMPLEMENTATION}}
}

{{ABSTRACT_SOLVER_USER_DEFINITIONS}}
