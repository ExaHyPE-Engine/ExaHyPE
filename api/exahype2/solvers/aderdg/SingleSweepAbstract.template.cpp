// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "Constants.h"

#include "exahype2/NonNegativeStatistics.h"
{%- if SUBCELL_LIMITER %}
#include "exahype2/dg/LimiterStatus.h"
{%- endif %}
#include "runner/RuntimeParameters.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Lock.h"
#include "tarch/NonCriticalAssertions.h"

std::bitset<DIMENSIONS> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::PeriodicBC = {{NAMESPACE | join("::")}}::PeriodicBC;

{% if USE_POINT_SOURCE %}
double                  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::pointSourceLocation[NumberOfPointSources][DIMENSIONS]{};
{% endif %}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _solverState(SolverState::GridConstruction),
  _minTimeStamp(0.0),
  _maxTimeStamp(0.0),
  _minTimeStampThisTimeStep(std::numeric_limits<double>::infinity()),
  _maxTimeStampThisTimeStep(0.0),
  _localMinTimeStampThisTimeStep(std::numeric_limits<double>::infinity()),
  _localMaxTimeStampThisTimeStep(0.0),
  _minCellH(std::numeric_limits<double>::infinity()),
  _maxCellH(0.0),
  _minCellHThisTimeStep(std::numeric_limits<double>::infinity()),
  _maxCellHThisTimeStep(0.0),
  _hasHangingFacesThisTimeStep(false),
  _minTimeStepSize(std::numeric_limits<double>::infinity()),
  _maxTimeStepSize(0.0),
  _minTimeStepSizeThisTimeStep(std::numeric_limits<double>::infinity()),
  _maxTimeStepSizeThisTimeStep(0.0) {
  {{CONSTRUCTOR_IMPLEMENTATION}}
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {
  {{DESTRUCTOR_IMPLEMENTATION}}
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxPatchSize() const {
  return getMaxCellSize();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinPatchSize() const {
  return getMinCellSize();
}

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getNumberOfDoFs() const {
  return BasePowerExponent<Order + 1, DIMENSIONS>::value * (NumberOfUnknowns + NumberOfAuxiliaryVariables);
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
  return _minTimeStepSizeThisTimeStep != std::numeric_limits<double>::infinity()
    ? _maxTimeStepSizeThisTimeStep
    : _maxTimeStepSize;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::update(
  const double timeStepSize,
  const double timeStamp,
  const double minCellSize,
  const double maxCellSize,
  const bool   hasHangingFaces
) {
  const tarch::multicore::Lock lock(_semaphore);

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
  const auto timeStepStatistics = ::exahype2::NonNegativeStatistics::accumulate(
    {_minTimeStepSizeThisTimeStep, _maxTimeStepSizeThisTimeStep},
    timeStepSize
  );
  _minTimeStepSizeThisTimeStep = timeStepStatistics.minimum;
  _maxTimeStepSizeThisTimeStep = timeStepStatistics.maximum;
  _localMinTimeStampThisTimeStep = std::min(_localMinTimeStampThisTimeStep, timeStamp);
  _localMaxTimeStampThisTimeStep = std::max(_localMaxTimeStampThisTimeStep, timeStamp);

  assertion(timeStamp < std::numeric_limits<double>::max() / 10.0, timeStamp);
  assertion(timeStamp >= 0.0, timeStamp);
  _maxTimeStampThisTimeStep = std::max(timeStamp, _maxTimeStampThisTimeStep);
  _minTimeStampThisTimeStep = std::min(timeStamp, _minTimeStampThisTimeStep);
  assertion(tarch::la::smallerEquals(_minTimeStampThisTimeStep, _maxTimeStampThisTimeStep), _minTimeStamp, _maxTimeStamp, _minTimeStampThisTimeStep, _maxTimeStampThisTimeStep, timeStepSize, timeStamp, minCellSize, maxCellSize);

  assertion(maxCellSize < std::numeric_limits<double>::max() / 10.0, maxCellSize);
  assertion(minCellSize > 0.0, minCellSize);
  _maxCellHThisTimeStep = std::max(_maxCellHThisTimeStep, maxCellSize);
  _minCellHThisTimeStep = std::min(_minCellHThisTimeStep, minCellSize);
  _hasHangingFacesThisTimeStep = _hasHangingFacesThisTimeStep or hasHangingFaces;

  logDebug("Updated with dt=" << timeStepSize << ", t=" << timeStamp << ", h=" << minCellSize << " / " << maxCellSize << ": t(new)=" << _minTimeStampThisTimeStep);
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::SolverState {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getSolverState() const {
{%- if SUBCELL_LIMITER %}
  // The limiter sweep is the single source of truth; the Galerkin solver state is
  // projected from it for the plotting, prediction and precision paths that read it.
  switch (_limitingSweep.state) {
  case ::exahype2::dg::LimitingSolverState::GridConstruction:
    return SolverState::GridConstruction;
  case ::exahype2::dg::LimitingSolverState::GridInitialisation:
    return SolverState::GridInitialisation;
  case ::exahype2::dg::LimitingSolverState::Predict:
    return SolverState::InitialPrediction;
  case ::exahype2::dg::LimitingSolverState::PredictOnHangingCells:
    return SolverState::PredictionOnHangingCells;
  case ::exahype2::dg::LimitingSolverState::CorrectAndFlag:
  case ::exahype2::dg::LimitingSolverState::SpreadDomain:
  case ::exahype2::dg::LimitingSolverState::RecomputeSubcells:
    return SolverState::TimeStep;
  case ::exahype2::dg::LimitingSolverState::Plotting:
    return SolverState::Plotting;
  case ::exahype2::dg::LimitingSolverState::Suspended:
    return SolverState::Suspended;
  }
  return SolverState::TimeStep;
{%- else %}
  return _solverState;
{%- endif %}
}
{%- if SUBCELL_LIMITER %}

/**
 * Returns the active a-posteriori limiter sweep for action-set guards.
 */
::exahype2::dg::LimitingSolverState {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getLimitingSolverState() const {
  return _limitingSweep.state;
}

/**
 * Records a troubled cell atomically because correctors update the rank-local
 * count concurrently before its global reduction.
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::markCellAsTroubled() {
  _numberOfTroubledCellsThisTimeStep.fetch_add(1, std::memory_order_relaxed);
}

long {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getNumberOfTroubledCells() const {
  return _numberOfTroubledCellsThisTimeStep.load(std::memory_order_relaxed);
}

/**
 * Identifies the spreading sweep that completes the halo before recomputation.
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isFinalSubcellSpreadSweep() const {
  return _limitingSweep.state == ::exahype2::dg::LimitingSolverState::SpreadDomain
     and _limitingSweep.spreadRing + 1 == ::exahype2::dg::NumberOfSpreadRings;
}
{%- endif %}

{% if ADAPTIVE_PRECISION_IMPLEMENTATION!="<empty>" and ADAPTIVE_PRECISION_IMPLEMENTATION!="<user-defined>" and ADAPTIVE_PRECISION_IMPLEMENTATION!="<none>"%}
    exahype2::PrecisionCommand {{NAMESPACE | join("::")}}::{{CLASSNAME}}::precisionCriterion(
      const {{SOLUTION_STORAGE_PRECISION}}* const NOALIAS Q, // [{{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS, double>&        x,
      const tarch::la::Vector<DIMENSIONS, double>&        h,
      const double                                        t,
      const SolverState                                   currentState
    ) {
  {{ADAPTIVE_PRECISION_IMPLEMENTATION}}
}
{% endif %}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridConstructionStep() {
{%- if SUBCELL_LIMITER %}
  assertion(_limitingSweep.state == ::exahype2::dg::LimitingSolverState::GridConstruction);
{%- else %}
  assertion(_solverState == SolverState::GridConstruction);
{%- endif %}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridConstructionStep() {}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridInitialisationStep() {
{%- if SUBCELL_LIMITER %}
  _limitingSweep.state = ::exahype2::dg::LimitingSolverState::GridInitialisation;
{%- else %}
  _solverState = SolverState::GridInitialisation;
{%- endif %}

  {{START_GRID_INITIALISATION_IMPLEMENTATION}}

{% if USE_POINT_SOURCE %}
  initPointSourceLocations(pointSourceLocation);
{% endif %}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridInitialisationStep() {
  // From here on every completed sweep has sent a face-data batch, which the
  // following sweep's boundary exchange consumes.
  _hasStoredFaceData = true;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::suspendSolversForOneGridSweep() {
{%- if SUBCELL_LIMITER %}
  _limitingSweep.state = ::exahype2::dg::LimitingSolverState::Suspended;
{%- else %}
  _solverState = SolverState::Suspended;
{%- endif %}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startTimeStep() {
{%- if SUBCELL_LIMITER %}
  const auto resumesFromSuspension =
    _limitingSweep.state == ::exahype2::dg::LimitingSolverState::Suspended;
  _limitingSweep                   = ::exahype2::dg::advanceLimitingSweep(
    _limitingSweep,
    ::exahype2::dg::LimitingBranchDecision{
      .hasHangingFaces  = _hasHangingFacesThisTimeStep,
      .hasTroubledCells = _numberOfTroubledCellsThisTimeStep.load(std::memory_order_relaxed) != 0
    },
    ::exahype2::dg::NumberOfSpreadRings
  );
  if (::exahype2::dg::isFirstSweepOfTimeStep(_limitingSweep)) {
    // A new step opens at the predictor, so clear the troubled count the previous
    // step raised, even when this sweep resumes from a suspended one.
    _numberOfTroubledCellsThisTimeStep.store(0, std::memory_order_relaxed);
  }
  if (resumesFromSuspension) {
    // A suspended sweep carries no time step, so resume without nullifying the
    // accumulators the next real step still needs.
    return;
  }
{%- else %}
  switch (_solverState) {
  case SolverState::GridConstruction:
    _solverState = SolverState::GridInitialisation;
    break;
  case SolverState::GridInitialisation:
    _solverState = SolverState::InitialPrediction;
    break;
    // A suspended solver returns before the start-of-step implementation runs, so its
    // eigenvalues survive the sweep it sits out.
  case SolverState::Suspended:
{% if USE_SEPARATE_SWEEPS is sameas True %}
    _solverState = SolverState::InitialPrediction;
{% else %}
    // Resuming on an adaptive mesh still owes the prediction-on-hanging-cells sweep, so
    // this picks the same successor as a plot does.
    _solverState = (_minTimeStamp == 0)
      ? SolverState::InitialPrediction
      : ((_hasHangingFacesThisTimeStep or _meshRefinementRanSinceLastTimeStep)
        ? SolverState::PredictionOnHangingCells
        : SolverState::TimeStep);
{% endif %}
    return;
  case SolverState::Plotting:
{% if USE_SEPARATE_SWEEPS is sameas True %}
    _solverState = SolverState::InitialPrediction;
{% else %}
    // Plots run between a time-step sweep and its prediction-on-hanging-cells
    // sweep, so on an adaptive mesh that sweep still has to follow the plot.
    _solverState = (_minTimeStamp == 0)
      ? SolverState::InitialPrediction
      : ((_hasHangingFacesThisTimeStep or _meshRefinementRanSinceLastTimeStep)
        ? SolverState::PredictionOnHangingCells
        : SolverState::TimeStep);
{% endif %}
    break;
  case SolverState::InitialPrediction:
    _solverState = (_hasHangingFacesThisTimeStep or _meshRefinementRanSinceLastTimeStep)
      ? SolverState::PredictionOnHangingCells
      : SolverState::TimeStep;
    break;
  case SolverState::PredictionOnHangingCells:
    _solverState = SolverState::TimeStep;
    break;
  case SolverState::TimeStep:
{% if USE_SEPARATE_SWEEPS is sameas True %}
    _solverState = SolverState::InitialPrediction;
{% else %}
    _solverState = (_hasHangingFacesThisTimeStep or _meshRefinementRanSinceLastTimeStep)
      ? SolverState::PredictionOnHangingCells
      : SolverState::TimeStep;
{% endif %}
    break;
  }
{%- endif %}

  _meshRefinementRanSinceLastTimeStep = false;

  {{START_TIME_STEP_IMPLEMENTATION}}

  if (isFirstGridSweepOfTimeStep()) {
    _minTimeStampThisTimeStep = std::numeric_limits<double>::infinity();
    _maxTimeStampThisTimeStep = 0.0;

    _minTimeStepSizeThisTimeStep = std::numeric_limits<double>::infinity();
    _maxTimeStepSizeThisTimeStep = 0.0;

    _maxCellHThisTimeStep      = 0.0;
    _minCellHThisTimeStep      = std::numeric_limits<double>::infinity();
    _hasHangingFacesThisTimeStep = false;

    _localMinTimeStampThisTimeStep = std::numeric_limits<double>::infinity();
    _localMaxTimeStampThisTimeStep = 0.0;
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
{%- if SUBCELL_LIMITER %}
  // The corrector counts troubled cells on each rank. Sum the count before the
  // last-sweep test so every rank either grows the subcell domain or closes the
  // step together, which keeps the recomputed domain independent of the
  // decomposition.
  if (getLimitingSolverState() == ::exahype2::dg::LimitingSolverState::CorrectAndFlag) {
    const long localTroubledCells  = _numberOfTroubledCellsThisTimeStep.load(std::memory_order_relaxed);
    long       globalTroubledCells = 0;
    tarch::mpi::allreduce(
      &localTroubledCells,
      &globalTroubledCells,
      1,
      MPI_LONG,
      MPI_SUM,
      tarch::mpi::Rank::getInstance().getCommunicator()
    );
    _numberOfTroubledCellsThisTimeStep.store(globalTroubledCells, std::memory_order_relaxed);
  }
  if (isLastGridSweepOfTimeStep() or getLimitingSolverState() == ::exahype2::dg::LimitingSolverState::Predict) {
{%- else %}
  if (isLastGridSweepOfTimeStep() or _solverState == SolverState::InitialPrediction) {
{%- endif %}
    // The upper half holds the negated maxima, so one minimum reduction yields both
    // extrema. Negating an IEEE double is exact, so this matches a maximum bit for bit.
    // The hanging-face flag rides along as the last maximum.
    const double localExtrema[9] = {
       _minTimeStampThisTimeStep,
       _minCellHThisTimeStep,
       _minTimeStepSizeThisTimeStep,
       _localMinTimeStampThisTimeStep,
      -_maxTimeStampThisTimeStep,
      -_maxCellHThisTimeStep,
      -_maxTimeStepSizeThisTimeStep,
      -_localMaxTimeStampThisTimeStep,
      _hasHangingFacesThisTimeStep ? -1.0 : 0.0
    };
    double         globalExtrema[9] = {0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0};
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
      9, MPI_DOUBLE,
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
    _hasHangingFacesThisTimeStep   = globalExtrema[8] < 0.0;
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

    {{FINISH_TIME_STEP_IMPLEMENTATION}}

    if (isLastGridSweepOfTimeStep()) {
      _minTimeStamp    = _minTimeStampThisTimeStep;
      _maxTimeStamp    = _maxTimeStampThisTimeStep;
      _minTimeStepSize = _minTimeStepSizeThisTimeStep;
      _maxTimeStepSize = _maxTimeStepSizeThisTimeStep;
      _minCellH        = _minCellHThisTimeStep;
      _maxCellH        = _maxCellHThisTimeStep;
    }
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startPlottingStep() {
  assertion(mayPlot() or tarch::hasNonCriticalAssertionBeenViolated());
{%- if SUBCELL_LIMITER %}
  _limitingSweep.state = ::exahype2::dg::LimitingSolverState::Plotting;
{%- else %}
  _solverState = SolverState::Plotting;
{%- endif %}
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
    case SolverState::Plotting:
      return "Plotting";
    case SolverState::InitialPrediction:
      return "InitialPrediction";
    case SolverState::PredictionOnHangingCells:
      return "PredictionOnHangingCells";
    case SolverState::TimeStep:
      return "TimeStep";
    case SolverState::Suspended:
      return "Suspended";
    }
  return "<undef>";
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mayPlot() const {
{%- if SUBCELL_LIMITER %}
  return (
    _limitingSweep.state == ::exahype2::dg::LimitingSolverState::Suspended
    or
    _limitingSweep.state == ::exahype2::dg::LimitingSolverState::GridInitialisation
    or
    isLastGridSweepOfTimeStep()
  );
{%- else %}
  return (
    _solverState == SolverState::Suspended
    or
    _solverState == SolverState::GridInitialisation
    or
    isLastGridSweepOfTimeStep()
  );
{%- endif %}
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isFirstGridSweepOfTimeStep() const {
{%- if SUBCELL_LIMITER %}
  return ::exahype2::dg::isFirstSweepOfTimeStep(_limitingSweep);
{%- else %}
{% if USE_SEPARATE_SWEEPS is sameas True %}
  return _solverState == SolverState::InitialPrediction;
{% else %}
  return (
    _solverState == SolverState::TimeStep
    or
    _solverState == SolverState::InitialPrediction
  );
{% endif %}
{%- endif %}
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isLastGridSweepOfTimeStep() const {
{%- if SUBCELL_LIMITER %}
  return ::exahype2::dg::isLastSweepOfTimeStep(
    _limitingSweep,
    ::exahype2::dg::LimitingBranchDecision{
      .hasHangingFaces  = _hasHangingFacesThisTimeStep,
      .hasTroubledCells = _numberOfTroubledCellsThisTimeStep.load(std::memory_order_relaxed) != 0
    }
  );
{%- else %}
  return _solverState == SolverState::TimeStep;
{%- endif %}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSimulation() {
{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
  kernels::{{SOLVER_NAME}}::Quadrature<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::initQuadratureNodesAndWeights();
  kernels::{{SOLVER_NAME}}::DGMatrices<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::initDGMatrices();
{% endfor %}

  {{START_SIMULATION_IMPLEMENTATION}}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishSimulation() {
  {{FINISH_SIMULATION_IMPLEMENTATION}}

{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
  kernels::{{SOLVER_NAME}}::Quadrature<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::freeQuadratureNodesAndWeights();
  kernels::{{SOLVER_NAME}}::DGMatrices<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::freeDGMatrices();
{% endfor %}
}

{{ABSTRACT_SOLVER_USER_DEFINITIONS}}
