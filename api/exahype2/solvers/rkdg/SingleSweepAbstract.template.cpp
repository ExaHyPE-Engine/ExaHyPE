// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "Constants.h"

#include "peano4/Constants.h"
#include "tarch/NonCriticalAssertions.h"
#include "tarch/instrumentation/Debug.h"
#include "tarch/instrumentation/Info.h"
#include "tarch/instrumentation/Warning.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/accelerator/Atomic.h"

std::bitset<DIMENSIONS> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::PeriodicBoundaries = {{NAMESPACE | join("::")}}::PeriodicBoundaries;

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}() {
  {{CONSTRUCTOR_IMPLEMENTATION}}
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() = default;

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxPatchSize() const {
  return getMaxCellSize();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinPatchSize() const {
  return getMinCellSize();
}

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getNumberOfDoFs() const {
  return peano4::power(DGOrder + 1, DIMENSIONS) * (NumberOfUnknowns + NumberOfAuxiliaryVariables);
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::suspendSolversForOneGridSweep() {
  _solverState = SolverState::Suspended;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinCellSize() const {
  return _minCellH;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxCellSize() const {
  return _maxCellH;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStamp() const {
  return _minTimeStampThisTimeStep != std::numeric_limits<double>::infinity()
    ? _minTimeStampThisTimeStep
    : _minTimeStamp;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStamp() const {
  // Only the minimum starts at infinity, so it says whether a cell has reported this time step.
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
  // Only the minimum starts at infinity, so it says whether a cell has reported this time step.
  return _minTimeStepSizeThisTimeStep != std::numeric_limits<double>::infinity()
    ? _maxTimeStepSizeThisTimeStep
    : _maxTimeStepSize;
}

thread_local typename {{NAMESPACE | join("::")}}::{{CLASSNAME}}::LocalUpdates {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_localUpdates;

/**
 * Adds one cell's time step size, time stamp and cell size to the extrema of the current
 * traversal.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::update(
  const double timeStepSize,
  const double timeStamp,
  const double cellSize
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

  assertion(timeStamp<std::numeric_limits<double>::max()/10.0,timeStamp);
  assertion(timeStamp>=0.0,timeStamp);
  _localUpdates.localMinTimeStamp = std::min(_localUpdates.localMinTimeStamp, timeStamp);
  _localUpdates.localMaxTimeStamp = std::max(_localUpdates.localMaxTimeStamp, timeStamp);
  _localUpdates.minTimeStamp      = std::min(_localUpdates.minTimeStamp, timeStamp);
  _localUpdates.maxTimeStamp      = std::max(_localUpdates.maxTimeStamp, timeStamp);
  assertion( tarch::la::smallerEquals(_localUpdates.minTimeStamp,_localUpdates.maxTimeStamp), _minTimeStamp, _localUpdates.minTimeStamp, _localUpdates.maxTimeStamp, timeStepSize, timeStamp, cellSize );

  assertion(cellSize<std::numeric_limits<double>::max()/10.0,cellSize);
  assertion(cellSize>0.0,cellSize);
  _localUpdates.maxCellH = std::max(_localUpdates.maxCellH, cellSize);
  _localUpdates.minCellH = std::min(_localUpdates.minCellH, cellSize);
  _localUpdates.empty    = false;

  logDebug(
    "Updated with dt=" << timeStepSize << ", t=" << timeStamp << ", h=" << cellSize
                       << ": t(new)=" << _localUpdates.minTimeStamp
  );
}

/**
 * Reduces the extrema update() accumulated in this traversal into the solver. One call per
 * tree at the end of a traversal keeps the per-cell update free of atomic operations.
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
  tarch::accelerator::updateAtomicMin(_minCellHThisTimeStep, _localUpdates.minCellH);
  tarch::accelerator::updateAtomicMax(_maxCellHThisTimeStep, _localUpdates.maxCellH);

  _localUpdates = LocalUpdates{};
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::SolverState {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getSolverState() const {
  return _solverState;
}

/**
 * An override calls this base class operation.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridConstructionStep() {
  assertion( _solverState == SolverState::GridConstruction );
}

/**
 * An override calls this base class operation.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridConstructionStep() {
}

/**
 * An override calls this base class operation.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;
}

/**
 * An override calls this base class operation.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridInitialisationStep() {
  // From here on every completed sweep has sent a face-data batch, which the
  // following sweep's boundary exchange consumes.
  _hasStoredFaceData = true;
}

/**
 * An override calls this base class operation.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startTimeStep() {
  _previousStageTimeStepSize = getMinTimeStepSize();

  // Every rank must select the same sequence of refinement and solver sweeps.
  const double localCellSizeExtrema[3] = {
     _minCellHThisTimeStep,
    -_maxCellHThisTimeStep,
    _meshRefinementRanSinceLastTimeStep ? -1.0 : 0.0
  };
  double globalCellSizeExtrema[3] = {0.0, 0.0, 0.0};
  tarch::mpi::allreduce(
    localCellSizeExtrema,
    globalCellSizeExtrema,
    3, MPI_DOUBLE,
    MPI_MIN,
    tarch::mpi::Rank::getInstance().getCommunicator()
  );
  _usesDynamicMeshRefinementSweeps =
    globalCellSizeExtrema[0] < -globalCellSizeExtrema[1] or globalCellSizeExtrema[2] < 0.0;

  switch (_solverState) {
    case SolverState::GridConstruction:
      _solverState = SolverState::GridInitialisation;
      break;
    case SolverState::GridInitialisation:
      _solverState = SolverState::TimeStepAfterGridInitialisation;
      break;
    case SolverState::Suspended:
      _solverStateAfterDynamicMeshRefinement = SolverState::RungeKuttaStep0;
      _solverState = _usesDynamicMeshRefinementSweeps
        ? SolverState::DynamicMeshRefinement
        : SolverState::RungeKuttaStep0;
      break;
    case SolverState::Plotting:
      _solverStateAfterDynamicMeshRefinement = SolverState::RungeKuttaStep0;
      _solverState = _usesDynamicMeshRefinementSweeps
        ? SolverState::DynamicMeshRefinement
        : SolverState::RungeKuttaStep0;
      break;
    case SolverState::PlottingAfterGridInitialisation:
      _solverState = SolverState::TimeStepAfterGridInitialisation;
      break;
    case SolverState::TimeStepAfterGridInitialisation:
      _solverStateAfterDynamicMeshRefinement = SolverState::RungeKuttaStep{{1 if RK_STEPS>1 else 0}};
      _solverState = _usesDynamicMeshRefinementSweeps
        ? SolverState::DynamicMeshRefinement
        : SolverState::RungeKuttaStep{{1 if RK_STEPS>1 else 0}};
      break;
  {% for item in range(RK_STEPS) %}
    case SolverState::RungeKuttaStep{{item}}:
      _solverStateAfterDynamicMeshRefinement = SolverState::RungeKuttaStep{{(item+1) % RK_STEPS}};
      _solverState = _usesDynamicMeshRefinementSweeps
        ? SolverState::DynamicMeshRefinement
        : SolverState::RungeKuttaStep{{(item+1) % RK_STEPS}};
      break;
  {%- endfor %}
    case SolverState::DynamicMeshRefinement:
      _solverState = _solverStateAfterDynamicMeshRefinement;
      break;
  }

  _meshRefinementRanSinceLastTimeStep = false;

  {{START_TIME_STEP_IMPLEMENTATION}}

  if (
    isFirstGridSweepOfTimeStep()
    or _solverState==SolverState::TimeStepAfterGridInitialisation
  ) {
    _minTimeStampThisTimeStep = std::numeric_limits<double>::infinity();
    _maxTimeStampThisTimeStep = 0.0;

    _minTimeStepSizeThisTimeStep = std::numeric_limits<double>::infinity();
    _maxTimeStepSizeThisTimeStep = 0.0;

    _maxCellHThisTimeStep      = 0.0;
    _minCellHThisTimeStep      = std::numeric_limits<double>::infinity();

    _localMinTimeStampThisTimeStep = std::numeric_limits<double>::infinity();
    _localMaxTimeStampThisTimeStep = 0.0;
  }
}

/**
 * An override calls this base class operation.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
  if ( isLastGridSweepOfTimeStep() ) {
    // The upper half holds the negated maxima, so one minimum reduction yields both
    // extrema. Negating an IEEE double is exact, so this matches a maximum bit for bit.
    const double localExtrema[8] = {
       _minTimeStampThisTimeStep,
       _minCellHThisTimeStep,
       _minTimeStepSizeThisTimeStep,
       _localMinTimeStampThisTimeStep,
      -_maxTimeStampThisTimeStep,
      -_maxCellHThisTimeStep,
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
    _minCellHThisTimeStep          = globalExtrema[1];
    _minTimeStepSizeThisTimeStep   = globalExtrema[2];
    _localMinTimeStampThisTimeStep = globalExtrema[3];

    _maxTimeStampThisTimeStep      = -globalExtrema[4];
    _maxCellHThisTimeStep          = -globalExtrema[5];
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

    assertion(
      _minTimeStampThisTimeStep != std::numeric_limits<double>::infinity(),
      _minTimeStampThisTimeStep,
      _maxTimeStampThisTimeStep
    );
    assertion(
      _minCellHThisTimeStep != std::numeric_limits<double>::infinity(),
      _minCellHThisTimeStep,
      _maxCellHThisTimeStep
    );
    assertion(
      _minTimeStepSizeThisTimeStep != std::numeric_limits<double>::infinity(),
      _minTimeStepSizeThisTimeStep,
      _maxTimeStepSizeThisTimeStep
    );

    _minTimeStamp    = _minTimeStampThisTimeStep;
    _maxTimeStamp    = _maxTimeStampThisTimeStep;
    _minTimeStepSize = _minTimeStepSizeThisTimeStep;
    _maxTimeStepSize = _maxTimeStepSizeThisTimeStep;
    _minCellH        = _minCellHThisTimeStep;
    _maxCellH        = _maxCellHThisTimeStep;
  }

  {{FINISH_TIME_STEP_IMPLEMENTATION}}
}

/**
 * An override calls this base class operation.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startPlottingStep() {
  assertion( mayPlot() or tarch::hasNonCriticalAssertionBeenViolated() );
  _solverState = _solverState==SolverState::GridInitialisation
    ? SolverState::PlottingAfterGridInitialisation
    : SolverState::Plotting;
}

/**
 * An override calls this base class operation.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishPlottingStep() {
}

std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(
  const SolverState state
) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::Suspended:
      return "suspended";
    case SolverState::Plotting:
      return "plotting";
    case SolverState::PlottingAfterGridInitialisation:
      return "plotting-after-grid-initialisation";
    case SolverState::TimeStepAfterGridInitialisation:
      return "time-step-after-grid-initialisation";
    case SolverState::DynamicMeshRefinement:
      return "dynamic-mesh-refinement";
    {% for item in range(RK_STEPS) %}
    case SolverState::RungeKuttaStep{{item}}:
      return "runge-kutta-step-{{item}}";
    {%- endfor %}
  }
  return "<undef>";
}

/**
 * A plot is consistent after the last Runge-Kutta stage, or while the solver
 * sits in a state that leaves the solution untouched.
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mayPlot() const {
  return isLastGridSweepOfTimeStep()
      or _solverState==SolverState::GridInitialisation
      or _solverState==SolverState::PlottingAfterGridInitialisation
      or _solverState==SolverState::Plotting;
}

/**
 * Returns the Runge-Kutta stage, or -1 outside a Runge-Kutta sweep.
 */
int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRkStage() const {
  const int stage = static_cast<int>(_solverState) - static_cast<int>(SolverState::RungeKuttaStep0);
  return stage>=0 and stage<{{RK_STEPS}} ? stage : -1;
}

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getRkStageAfterDynamicMeshRefinement() const {
  const int stage = static_cast<int>(_solverStateAfterDynamicMeshRefinement)
    - static_cast<int>(SolverState::RungeKuttaStep0);
  return stage>=0 and stage<{{RK_STEPS}} ? stage : -1;
}

/**
 * Returns the projected stage's time step size before cells start the next stage.
 */
double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getPreviousStageTimeStepSize() const {
  return _previousStageTimeStepSize;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isFirstGridSweepOfTimeStep() const {
  return getSolverState()==SolverState::RungeKuttaStep0;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isLastGridSweepOfTimeStep() const {
  return getSolverState()==SolverState::RungeKuttaStep{{RK_STEPS-1}};
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::usesDynamicMeshRefinementSweeps() const {
  return _usesDynamicMeshRefinementSweeps;
}

/**
 * An override calls this base class operation.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSimulation() {
  kernels::{{SOLVER_NAME}}::Quadrature<double>::initialise();
  kernels::{{SOLVER_NAME}}::DGMatrices<double>::initialise();
  kernels::{{SOLVER_NAME}}::ButcherTableau<double>::initialise();

  {{START_SIMULATION_IMPLEMENTATION}}
}

/**
 * An override calls this base class operation.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishSimulation() {
  {{FINISH_SIMULATION_IMPLEMENTATION}}

  kernels::{{SOLVER_NAME}}::Quadrature<double>::deallocate();
  kernels::{{SOLVER_NAME}}::DGMatrices<double>::deallocate();
  kernels::{{SOLVER_NAME}}::ButcherTableau<double>::deallocate();
}

{{ABSTRACT_SOLVER_USER_DEFINITIONS}}
