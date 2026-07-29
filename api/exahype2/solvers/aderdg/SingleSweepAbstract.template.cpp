// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "Constants.h"

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
  _minTimeStampThisTimeStep(0.0),
  _maxTimeStampThisTimeStep(0.0),
  _minCellH(std::numeric_limits<double>::max()),
  _maxCellH(0.0),
  _minCellHThisTimeStep(0.0),
  _maxCellHThisTimeStep(0.0),
  _minTimeStepSize(std::numeric_limits<double>::max()),
  _maxTimeStepSize(0.0),
  _minTimeStepSizeThisTimeStep(std::numeric_limits<double>::max()),
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

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::update(double timeStepSize, double timeStamp, double minCellSize, double maxCellSize) {
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
  _maxTimeStampThisTimeStep = std::max(timeStamp, _maxTimeStampThisTimeStep);
  _minTimeStampThisTimeStep = std::min(timeStamp, _minTimeStampThisTimeStep);
  assertion(tarch::la::smallerEquals(_minTimeStampThisTimeStep, _maxTimeStampThisTimeStep), _minTimeStamp, _maxTimeStamp, _minTimeStampThisTimeStep, _maxTimeStampThisTimeStep, timeStepSize, timeStamp, minCellSize, maxCellSize);

  assertion(maxCellSize < std::numeric_limits<double>::max() / 10.0, maxCellSize);
  assertion(minCellSize > 0.0, minCellSize);
  _maxCellHThisTimeStep = std::max(_maxCellHThisTimeStep, maxCellSize);
  _minCellHThisTimeStep = std::min(_minCellHThisTimeStep, minCellSize);

  logDebug("Updated with dt=" << timeStepSize << ", t=" << timeStamp << ", h=" << minCellSize << " / " << maxCellSize << ": t(new)=" << _minTimeStampThisTimeStep);
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::SolverState {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getSolverState() const {
  return _solverState;
}

{% if ADAPTIVE_PRECISION_IMPLEMENTATION!="<empty>" and ADAPTIVE_PRECISION_IMPLEMENTATION!="<user-defined>" and ADAPTIVE_PRECISION_IMPLEMENTATION!="<none>"%}
    exahype2::PrecisionCommand {{NAMESPACE | join("::")}}::{{CLASSNAME}}::precisionCriterion(
      const {{SOLUTION_STORAGE_PRECISION}} * NOALIAS Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<DIMENSIONS,double>&         x,
      const tarch::la::Vector<DIMENSIONS,double>&         h,
      double                                              t,
      SolverState                                         currentState
    ) {
  {{ADAPTIVE_PRECISION_IMPLEMENTATION}}
}
{% endif %}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridConstructionStep() {
  assertion(_solverState == SolverState::GridConstruction);
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridConstructionStep() {}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;

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
  _solverState = SolverState::Suspended;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startTimeStep() {
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
      : ((_minCellHThisTimeStep < _maxCellHThisTimeStep or _meshRefinementRanSinceLastTimeStep)
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
      : ((_minCellHThisTimeStep < _maxCellHThisTimeStep or _meshRefinementRanSinceLastTimeStep)
        ? SolverState::PredictionOnHangingCells
        : SolverState::TimeStep);
{% endif %}
    break;
  case SolverState::InitialPrediction:
    // TODO: This assumes isotropic cells. On a regular rectangular global domain,
    // this can still satisfy _minCellHThisTimeStep < _maxCellHThisTimeStep, which triggers
    // the PredictionOnHangingCells state and can lead to wrong coupled limiter/FV updates.
    // This may also be the case with limiting and (static) AMR.
    _solverState = (_minCellHThisTimeStep < _maxCellHThisTimeStep or _meshRefinementRanSinceLastTimeStep)
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
    // TODO: Same as above.
    // The cell statistics stem from the previous time step, so a mesh that only
    // just became adaptive is caught through the mesh-refinement flag.
    _solverState = (_minCellHThisTimeStep < _maxCellHThisTimeStep or _meshRefinementRanSinceLastTimeStep)
      ? SolverState::PredictionOnHangingCells
      : SolverState::TimeStep;
{% endif %}
    break;
  }

  _meshRefinementRanSinceLastTimeStep = false;

  {{START_TIME_STEP_IMPLEMENTATION}}

  if (isFirstGridSweepOfTimeStep()) {
    _minTimeStampThisTimeStep = std::numeric_limits<double>::max();
    _maxTimeStampThisTimeStep = std::numeric_limits<double>::min();

    _minTimeStepSizeThisTimeStep = std::numeric_limits<double>::max();
    _maxTimeStepSizeThisTimeStep = std::numeric_limits<double>::min();

    _maxCellHThisTimeStep      = 0.0;
    _minCellHThisTimeStep      = std::numeric_limits<double>::max();

    _localMinTimeStampThisTimeStep = std::numeric_limits<double>::max();
    _localMaxTimeStampThisTimeStep = std::numeric_limits<double>::min();
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
  if (isLastGridSweepOfTimeStep() or _solverState == SolverState::InitialPrediction) {
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

    tarch::mpi::allreduce(
      localExtrema,
      globalExtrema,
      8, MPI_DOUBLE,
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
      _minCellH        = _minCellHThisTimeStep;
      _maxCellH        = _maxCellHThisTimeStep;
    }
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
  return (
    _solverState == SolverState::Suspended
    or
    _solverState == SolverState::GridInitialisation
    or
    isLastGridSweepOfTimeStep()
  );
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isFirstGridSweepOfTimeStep() const {
{% if USE_SEPARATE_SWEEPS is sameas True %}
  return _solverState == SolverState::InitialPrediction;
{% else %}
  return (
    _solverState == SolverState::TimeStep
    or
    _solverState == SolverState::InitialPrediction
  );
{% endif %}
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isLastGridSweepOfTimeStep() const {
  return _solverState == SolverState::TimeStep;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSimulation() {
  // Initialise nodes, weights and matrices for use of the kernels.
{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
  kernels::{{SOLVER_NAME}}::Quadrature<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::initQuadratureNodesAndWeights();
  kernels::{{SOLVER_NAME}}::DGMatrices<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::initDGMatrices();
{% endfor %}

  {{START_SIMULATION_IMPLEMENTATION}}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishSimulation() {
  {{FINISH_SIMULATION_IMPLEMENTATION}}

  // Free the generated nodes, weights and matrices.
{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
  kernels::{{SOLVER_NAME}}::Quadrature<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::freeQuadratureNodesAndWeights();
  kernels::{{SOLVER_NAME}}::DGMatrices<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::freeDGMatrices();
{% endfor %}
}

{{ABSTRACT_SOLVER_USER_DEFINITIONS}}
