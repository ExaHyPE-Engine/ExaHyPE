// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "Constants.h"

#include <algorithm>
#include <cstdint>
#include <limits>

#include "exahype2/NonNegativeStatistics.h"
#include "runner/RuntimeParameters.h"
#include "tarch/instrumentation/Log.h"
#include "tarch/mpi/Collectives.h"
#include "tarch/mpi/Rank.h"
#include "tarch/multicore/Lock.h"
#include "tarch/NonCriticalAssertions.h"

std::bitset<DIMENSIONS> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::PeriodicBC = {{NAMESPACE | join("::")}}::PeriodicBC;

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  {{BASIS_INITIALIZERS | indent(2,True) }}
  _solverState(SolverState::GridConstruction),
  _solverStateBeforeDynamicMeshRefinement(SolverState::GridConstruction),
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
  _minTimeStepSize(std::numeric_limits<double>::infinity()),
  _maxTimeStepSize(0.0),
  _minTimeStepSizeThisTimeStep(std::numeric_limits<double>::infinity()),
  _maxTimeStepSizeThisTimeStep(0.0) {
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
  return BasePowerExponent<DGOrder + 1, DIMENSIONS>::value * (NumberOfUnknowns + NumberOfAuxiliaryVariables);
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
  const double cellSize
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

  assertion(
    timeStamp < std::numeric_limits<double>::max() / 10.0,
    timeStamp
  );
  assertion(timeStamp >= 0.0, timeStamp);
  _maxTimeStampThisTimeStep = std::max(timeStamp, _maxTimeStampThisTimeStep);
  _minTimeStampThisTimeStep = std::min(timeStamp, _minTimeStampThisTimeStep);
  assertion(
    tarch::la::smallerEquals(_minTimeStamp, _maxTimeStampThisTimeStep),
    _minTimeStamp,
    _minTimeStampThisTimeStep,
    _maxTimeStampThisTimeStep,
    timeStepSize,
    timeStamp,
    cellSize
  );

  assertion(
    cellSize < std::numeric_limits<double>::max() / 10.0,
    cellSize
  );
  assertion(cellSize > 0.0, cellSize);
  _maxCellHThisTimeStep = std::max(_maxCellHThisTimeStep, cellSize);
  _minCellHThisTimeStep = std::min(_minCellHThisTimeStep, cellSize);

  logDebug("Updated with dt=" << timeStepSize << ", t=" << timeStamp << ", h=" << cellSize << ": t(new)=" << _minTimeStampThisTimeStep);
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::SolverState {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getSolverState() const {
  return _solverState;
}

{% if REFINEMENT_CRITERION_IMPLEMENTATION!="<user-defined>" %}
::exahype2::RefinementCommand {{NAMESPACE | join("::")}}::{{CLASSNAME}}::refinementCriterion(
  const double * NOALIAS Q,    // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<DIMENSIONS,double>&  x,
  const tarch::la::Vector<DIMENSIONS,double>&  h,
  double                                       t
) {
  {% if REFINEMENT_CRITERION_IMPLEMENTATION=="<empty>" %}
  ::exahype2::RefinementCommand result = ::exahype2::RefinementCommand::Keep;

  return result;
  {% else %}
  {{REFINEMENT_CRITERION_IMPLEMENTATION}}
  {% endif %}
}
{% endif %}

{% if INITIAL_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::initialCondition(
  double * NOALIAS                        Q,     // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<DIMENSIONS,double>&  x,
  const tarch::la::Vector<DIMENSIONS,double>&  h
) {
  {{INITIAL_CONDITIONS_IMPLEMENTATION}}
}
{% endif %}

{% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<user-defined>" and BOUNDARY_CONDITIONS_IMPLEMENTATION!="<none>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::boundaryConditions(
  const double * NOALIAS                  Qinside,   // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  double * NOALIAS                        Qoutside,  // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<DIMENSIONS,double>&  x,
  double                                       t,
  int                                          normal
) {
  {% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<empty>" %}
  {{BOUNDARY_CONDITIONS_IMPLEMENTATION}}
  {% endif %}
}
{% endif %}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridConstructionStep() {
  assertion( _solverState == SolverState::GridConstruction );
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridConstructionStep() {
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridInitialisationStep() {
  // From here on every completed sweep has sent a face-data batch, which the
  // following sweep's boundary exchange consumes.
  _hasStoredFaceData = true;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startTimeStep() {
  switch (_solverState) {
    case SolverState::GridConstruction:
      _solverState = SolverState::GridInitialisation;
      break;
    case SolverState::GridInitialisation:
      _solverState = SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep0;
      break;
    case SolverState::DynamicMeshRefinement:
      // finishDynamicMeshRefinementStep() restores the state that the mesh-refinement
      // sweeps interrupted, so a time step can only start once that pairing has completed.
      assertion(false, toString(_solverState));
      break;
    case SolverState::Suspended:
      _solverState = SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep0;
      break;
    case SolverState::Plotting:
      _solverState = SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep0;
      break;
  {% for item in range(RK_STEPS-1) %}
    case SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep{{item}}:
      _solverState = SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{{item}};
      break;
    case SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{{item}}:
      _solverState = SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep{{item+1}};
      break;
  {%- endfor %}
    case SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep{{RK_STEPS-1}}:
      _solverState = SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{{RK_STEPS-1}};
      break;
    case SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{{RK_STEPS-1}}:
      _solverState = SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep0;
      break;
  }

  _meshRefinementRanSinceLastTimeStep = false;

  {{START_TIME_STEP_IMPLEMENTATION}}

  if ( isFirstGridSweepOfTimeStep() ) {
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

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startPlottingStep() {
  assertion( mayPlot()  or ::tarch::hasNonCriticalAssertionBeenViolated() );
  _solverState = SolverState::Plotting;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishPlottingStep() {
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startDynamicMeshRefinementStep() {
  ::exahype2::Solver::startDynamicMeshRefinementStep();
  _solverStateBeforeDynamicMeshRefinement = _solverState;
  _solverState                            = SolverState::DynamicMeshRefinement;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishDynamicMeshRefinementStep() {
  _solverState = _solverStateBeforeDynamicMeshRefinement;
}

std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(
  const SolverState state
) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::Plotting:
      return "plotting";
    case SolverState::DynamicMeshRefinement:
      return "dynamic-mesh-refinement";
    case SolverState::Suspended:
      return "suspended";
    {% for item in range(RK_STEPS) %}
    case SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep{{item}}:
      return "project-on-faces-and-compute-volume-integral-of-step-{{item}}";
    case SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{{item}}:
      return "solve-Riemann-problem-and-add-to-volume-integral-of-step-{{item}}";
    {%- endfor %}
  }
  return "<undef>";
}

/**
 * A separate-sweeps solver spreads one time step over two sweeps per Runge-Kutta step, so
 * a plot is only consistent once the last of them has run, or while the solver sits in a
 * state that leaves the solution untouched.
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mayPlot() const {
  return isLastGridSweepOfTimeStep()
      or _solverState==SolverState::GridInitialisation
      or _solverState==SolverState::Plotting;
}

/**
 * True in exactly one of the two sweeps per Runge-Kutta step, the one that opens step 0.
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isFirstGridSweepOfTimeStep() const {
  return _solverState==SolverState::ProjectOnFacesAndComputeVolumeIntegralOfStep0;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isLastGridSweepOfTimeStep() const {
  return _solverState==SolverState::SolveRiemannProblemAndAddToVolumeIntegralOfStep{{RK_STEPS-1}};
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSimulation() {
  {{START_SIMULATION_IMPLEMENTATION}}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishSimulation() {
  {{FINISH_SIMULATION_IMPLEMENTATION}}
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::cellCanUseStatelessPDETerms(
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellCentre,
  [[maybe_unused]] const tarch::la::Vector<DIMENSIONS, double>& cellH,
  [[maybe_unused]] const double timeStamp,
  [[maybe_unused]] const double timeStepSize
) const {
  return true;
}

{{ABSTRACT_SOLVER_USER_DEFINITIONS}}
