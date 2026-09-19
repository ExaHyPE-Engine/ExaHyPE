// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "repositories/SolverRepository.h"
#include "peano4/Constants.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/accelerator/Atomic.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/instrumentation/Warning.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"

std::bitset<DIMENSIONS> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::PeriodicBoundaries = {{NAMESPACE | join("::")}}::PeriodicBoundaries;

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {
  {{CONSTRUCTOR_IMPLEMENTATION}}
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() = default;

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::suspendSolversForOneGridSweep() {
  _solverState = SolverState::Suspended;
}

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getNumberOfDoFs() const {
  return peano4::power(NumberOfFiniteVolumesPerAxisPerPatch, DIMENSIONS) * (NumberOfUnknowns + NumberOfAuxiliaryVariables);
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
  return _minTimeStampThisTimeStep != std::numeric_limits<double>::infinity()
    ? _minTimeStampThisTimeStep
    : _minTimeStamp;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStamp() const {
  // Only the minimum starts at infinity, so it says whether a patch has reported this time step.
  return _minTimeStampThisTimeStep != std::numeric_limits<double>::infinity()
    ? _maxTimeStampThisTimeStep
    : _maxTimeStamp;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStepSize() const {
  return _minTimeStepSizeThisTimeStep != std::numeric_limits<double>::infinity()
    ? _minTimeStepSizeThisTimeStep
    : _minTimeStepSize;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStepSize() const {
  // Only the minimum starts at infinity, so it says whether a patch has reported this time step.
  return _minTimeStepSizeThisTimeStep != std::numeric_limits<double>::infinity()
    ? _maxTimeStepSizeThisTimeStep
    : _maxTimeStepSize;
}

thread_local typename {{NAMESPACE | join("::")}}::{{CLASSNAME}}::LocalUpdates {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_localUpdates;

/**
 * Adds one patch's time step size, time stamp and patch size to the extrema of the current
 * traversal.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::update(
  const double timeStepSize,
  const double timeStamp,
  const double minPatchSize,
  const double maxPatchSize
) {
  const auto observedTimeStepStatistics =
    ::exahype2::NonNegativeStatistics{timeStepSize, timeStepSize};
  if (not observedTimeStepStatistics.hasPhysicalValues()) {
    tarch::triggerNonCriticalAssertion(
      __FILE__,
      __LINE__,
      "The observed time step size is finite and non-negative.",
      "Time step size bits="
        + std::to_string(
          observedTimeStepStatistics.invalidObservationBitsOrIdentity()
        )
    );
  }

  _localUpdates.timeStepSize = ::exahype2::NonNegativeStatistics::accumulate(_localUpdates.timeStepSize, timeStepSize);

  updateTimeStampAndPatchSize(timeStamp, minPatchSize, maxPatchSize);
}

/**
 * Accumulates a patch's time stamp and size extrema without a time step.
 *
 * A cell that skips its update this sweep still names its stamp and extent, so
 * the global statistics cover every cell of the mesh.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::updateTimeStampAndPatchSize(
  const double timeStamp,
  const double minPatchSize,
  const double maxPatchSize
) {
  assertion(std::isfinite(timeStamp) and timeStamp >= 0.0, timeStamp);
  _localUpdates.localMinTimeStamp = std::min(_localUpdates.localMinTimeStamp, timeStamp);
  _localUpdates.localMaxTimeStamp = std::max(_localUpdates.localMaxTimeStamp, timeStamp);
  _localUpdates.minTimeStamp      = std::min(_localUpdates.minTimeStamp, timeStamp);
  _localUpdates.maxTimeStamp      = std::max(_localUpdates.maxTimeStamp, timeStamp);
  assertion(std::isfinite(minPatchSize) and minPatchSize > 0.0, minPatchSize, maxPatchSize);
  assertion(std::isfinite(maxPatchSize) and maxPatchSize >= minPatchSize, minPatchSize, maxPatchSize);
  _localUpdates.minVolumeH = std::min(_localUpdates.minVolumeH, minPatchSize / NumberOfFiniteVolumesPerAxisPerPatch);
  _localUpdates.maxVolumeH = std::max(_localUpdates.maxVolumeH, maxPatchSize / NumberOfFiniteVolumesPerAxisPerPatch);
  _localUpdates.empty      = false;
}

/**
 * Reduces the extrema update() accumulated in this traversal into the solver. One call per
 * tree at the end of a traversal keeps the per-patch update free of atomic operations.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::reduceLocalUpdates() {
  if (_localUpdates.empty) {
    return;
  }

  // An invalid observation overwrites both extrema, and a later atomic minimum
  // or maximum keeps at least one of them, so the rank reduction reads the
  // invalid observation.
  if (_localUpdates.timeStepSize.hasInvalidObservation()) {
    tarch::accelerator::storeAtomic(_minTimeStepSizeThisTimeStep, _localUpdates.timeStepSize.minimum);
    tarch::accelerator::storeAtomic(_maxTimeStepSizeThisTimeStep, _localUpdates.timeStepSize.maximum);
  } else {
    tarch::accelerator::updateAtomicMin(_minTimeStepSizeThisTimeStep, _localUpdates.timeStepSize.minimum);
    tarch::accelerator::updateAtomicMax(_maxTimeStepSizeThisTimeStep, _localUpdates.timeStepSize.maximum);
  }
  tarch::accelerator::updateAtomicMin(_localMinTimeStampThisTimeStep, _localUpdates.localMinTimeStamp);
  tarch::accelerator::updateAtomicMax(_localMaxTimeStampThisTimeStep, _localUpdates.localMaxTimeStamp);
  tarch::accelerator::updateAtomicMin(_minTimeStampThisTimeStep, _localUpdates.minTimeStamp);
  tarch::accelerator::updateAtomicMax(_maxTimeStampThisTimeStep, _localUpdates.maxTimeStamp);
  tarch::accelerator::updateAtomicMin(_minVolumeHThisTimeStep, _localUpdates.minVolumeH);
  tarch::accelerator::updateAtomicMax(_maxVolumeHThisTimeStep, _localUpdates.maxVolumeH);

  _localUpdates = LocalUpdates{};
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
    // that a suspended solver does not nullify its eigenvalue and time step sizes.
    _solverState = SolverState::TimeStep;
    return;
  }

  _solverState = SolverState::TimeStep;
  _meshRefinementRanSinceLastTimeStep = false;

  {{START_TIME_STEP_IMPLEMENTATION}}

  _minTimeStampThisTimeStep = std::numeric_limits<double>::infinity();
  _maxTimeStampThisTimeStep = 0.0;

  _minTimeStepSizeThisTimeStep = std::numeric_limits<double>::infinity();
  _maxTimeStepSizeThisTimeStep = 0.0;

  _maxVolumeHThisTimeStep = 0.0;
  _minVolumeHThisTimeStep = std::numeric_limits<double>::infinity();

  _localMinTimeStampThisTimeStep = std::numeric_limits<double>::infinity();
  _localMaxTimeStampThisTimeStep = 0.0;
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

    const auto localTimeStepStatistics = ::exahype2::NonNegativeStatistics{
      _minTimeStepSizeThisTimeStep,
      _maxTimeStepSizeThisTimeStep
    };
    const auto hasInvalidLocalTimeStepObservation =
      localTimeStepStatistics.hasInvalidObservation();
    const std::uint64_t localInvalidTimeStepObservation[2]{
      hasInvalidLocalTimeStepObservation ? 0U : 1U,
      localTimeStepStatistics.invalidObservationBitsOrIdentity()
    };
    std::uint64_t globalInvalidTimeStepObservation[2]{};

    tarch::mpi::allreduce(
      localExtrema,
      globalExtrema,
      8, MPI_DOUBLE,
      MPI_MIN,
      communicator
    );
    tarch::mpi::allreduce(
      localInvalidTimeStepObservation,
      globalInvalidTimeStepObservation,
      2,
      MPI_UINT64_T,
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
    if (globalInvalidTimeStepObservation[0] == 0U) {
      const auto invalidTimeStepStatistics =
        ::exahype2::NonNegativeStatistics::fromInvalidObservationBits(
          globalInvalidTimeStepObservation[1]
        );
      _minTimeStepSizeThisTimeStep = invalidTimeStepStatistics.minimum;
      _maxTimeStepSizeThisTimeStep = invalidTimeStepStatistics.maximum;
    }
  }

  assertion(
    _minTimeStampThisTimeStep != std::numeric_limits<double>::infinity(),
    _minTimeStampThisTimeStep,
    _maxTimeStampThisTimeStep
  );
  assertion(
    _minVolumeHThisTimeStep != std::numeric_limits<double>::infinity(),
    _minVolumeHThisTimeStep,
    _maxVolumeHThisTimeStep
  );
  assertion(
    _minTimeStepSizeThisTimeStep != std::numeric_limits<double>::infinity(),
    _minTimeStepSizeThisTimeStep,
    _maxTimeStepSizeThisTimeStep
  );

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

std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(
  const SolverState state
) {
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
  // The solver remains in TimeStep between time steps, so plotting is valid there.
  return _solverState == SolverState::GridInitialisation or isLastGridSweepOfTimeStep();
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isFirstGridSweepOfTimeStep() const {
  return _solverState == SolverState::TimeStep;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isLastGridSweepOfTimeStep() const {
  return _solverState == SolverState::TimeStep;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSimulation() {
  {{START_SIMULATION_IMPLEMENTATION}}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishSimulation() {
  {{FINISH_SIMULATION_IMPLEMENTATION}}
}

{{ABSTRACT_SOLVER_USER_DEFINITIONS}}
