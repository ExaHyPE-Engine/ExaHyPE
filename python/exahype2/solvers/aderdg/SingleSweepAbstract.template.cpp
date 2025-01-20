// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include <algorithm>

#include "Constants.h"

#include "tarch/multicore/Lock.h"
#include "tarch/NonCriticalAssertions.h"

tarch::logging::Log     {{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}::_log("{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}");

std::bitset<Dimensions> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}::PeriodicBC = {{NAMESPACE | join("::")}}::PeriodicBC;

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _solverState(SolverState::GridConstruction),
  _minTimeStamp(0.0),
  _maxTimeStamp(0.0),
  _minTimeStampThisTimeStep(0.0),
  _maxTimeStampThisTimeStep(0.0),
  _minCellH(0.0),
  _maxCellH(0.0),
  _minCellHThisTimeStep(0.0),
  _maxCellHThisTimeStep(0.0),
  _minTimeStepSize(std::numeric_limits<double>::max()),
  _maxTimeStepSize(0.0),
  _minTimeStepSizeThisTimeStep(std::numeric_limits<double>::max()),
  _maxTimeStepSizeThisTimeStep(0.0),
  _cellUpdates(0) {
  {{CONSTRUCTOR_IMPLEMENTATION}}

  // Initialise nodes, weights and matrices for use of the kernels.
  {% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
  kernels::{{SOLVER_NAME}}::Quadrature<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::initQuadratureNodesAndWeights();
  kernels::{{SOLVER_NAME}}::DGMatrices<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::initDGMatrices();
  {% endfor %}
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}(){
  {{DESTRUCTOR_IMPLEMENTATION}}

  // Free the generated nodes, weights and matrices.
  {% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}
  kernels::{{SOLVER_NAME}}::Quadrature<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::freeQuadratureNodesAndWeights();
  kernels::{{SOLVER_NAME}}::DGMatrices<{{COMPUTATION_PRECISIONS[PRECISION_NUM]}}>::freeDGMatrices();
  {% endfor %}
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxMeshSize() const {
  return getMaxCellSize();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinMeshSize() const {
  return getMinCellSize();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinCellSize(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _minCellHThisTimeStep : _minCellH;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxCellSize(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _maxCellHThisTimeStep : _maxCellH;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _minTimeStampThisTimeStep : _minTimeStamp;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _maxTimeStampThisTimeStep : _maxTimeStamp;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStepSize() const {
  return _minTimeStepSize;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStepSize() const {
  return _maxTimeStepSize;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::update(double timeStepSize, double timeStamp, double cellSize) {
  tarch::multicore::Lock lock(_semaphore);

  if ( tarch::la::greater(timeStepSize,0.0) ) {
    assertion1(timeStepSize<std::numeric_limits<double>::max()/10.0,timeStepSize);
    assertion1(timeStepSize>=0.0,timeStepSize);
    _minTimeStepSizeThisTimeStep = std::min(timeStepSize,_minTimeStepSizeThisTimeStep);
    _maxTimeStepSizeThisTimeStep = std::max(timeStepSize,_maxTimeStepSizeThisTimeStep);
    assertion2(_minTimeStepSizeThisTimeStep<=_maxTimeStepSizeThisTimeStep, _minTimeStepSizeThisTimeStep, _maxTimeStepSizeThisTimeStep );

    _cellUpdates++;

    _localMinTimeStampThisTimeStep = std::min(_localMinTimeStampThisTimeStep, timeStamp);
    _localMaxTimeStampThisTimeStep = std::max(_localMaxTimeStampThisTimeStep, timeStamp);
  }

  assertion1(timeStamp<std::numeric_limits<double>::max()/10.0,timeStamp);
  assertion1(timeStamp>=0.0,timeStamp);
  _maxTimeStampThisTimeStep = std::max(timeStamp,_maxTimeStampThisTimeStep);
  _minTimeStampThisTimeStep = std::min(timeStamp,_minTimeStampThisTimeStep);
  assertion6( tarch::la::smallerEquals(_minTimeStamp,_maxTimeStampThisTimeStep), _minTimeStamp, _minTimeStampThisTimeStep, _maxTimeStampThisTimeStep, timeStepSize, timeStamp, cellSize );

  assertion1(cellSize<std::numeric_limits<double>::max()/10.0,cellSize);
  assertion1(cellSize>0.0,cellSize);
  _maxCellHThisTimeStep = std::max(_maxCellHThisTimeStep,cellSize);
  _minCellHThisTimeStep = std::min(_minCellHThisTimeStep,cellSize);

  logDebug( "update(double,double,double)", "updated with dt=" << timeStepSize << ", t=" << timeStamp << ", h=" << cellSize << ": t(new)=" << _minTimeStampThisTimeStep );
}


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::SolverState {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getSolverState() const {
  return _solverState;
}


{% if REFINEMENT_CRITERION_IMPLEMENTATION!="<user-defined>" %}
::exahype2::RefinementCommand {{NAMESPACE | join("::")}}::{{CLASSNAME}}::refinementCriterion(
  const {{SOLUTION_STORAGE_PRECISION}} * __restrict__ Q,    // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
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
  {{SOLUTION_STORAGE_PRECISION}}* __restrict__  Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions, double>&  x,
  const tarch::la::Vector<Dimensions, double>&  h,
  //const tarch::la::Vector<Dimensions, int>&      index,
  bool                                          gridIsConstructed
) {
  {{INITIAL_CONDITIONS_IMPLEMENTATION}}
}
{% endif %}

{% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::boundaryConditions(
  const {{CORRECTOR_COMPUTATION_PRECISION}} * __restrict__                  Qinside, // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  {{CORRECTOR_COMPUTATION_PRECISION}} * __restrict__                        Qoutside, // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  x,
  const tarch::la::Vector<Dimensions,double>&  h,
  double                                       t,
  int                                          normal
) {
  {{BOUNDARY_CONDITIONS_IMPLEMENTATION}}
}
{% endif %}

{% if ADAPTIVE_PRECISION_IMPLEMENTATION!="<empty>" and ADAPTIVE_PRECISION_IMPLEMENTATION!="<user-defined>" and ADAPTIVE_PRECISION_IMPLEMENTATION!="<none>"%}
    exahype2::PrecisionCommand {{NAMESPACE | join("::")}}::{{CLASSNAME}}::precisionCriterion(
      const {{SOLUTION_STORAGE_PRECISION}} * __restrict__ Q, // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const tarch::la::Vector<Dimensions,double>&         x,
      const tarch::la::Vector<Dimensions,double>&         h,
      double                                              t,
      SolverState                                         currentState
    ) {
  {{ADAPTIVE_PRECISION_IMPLEMENTATION}}
}
{% endif %}

{% if RIEMANN_SOLVER_IMPLEMENTATION!="<empty>" and RIEMANN_SOLVER_IMPLEMENTATION!="<user-defined>" and RIEMANN_SOLVER_IMPLEMENTATION!="<none>"%}
    void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::riemannSolver(
      {{CORRECTOR_COMPUTATION_PRECISION}}* const FL, // FL[{{NUMBER_OF_UNKNOWNS}}
      {{CORRECTOR_COMPUTATION_PRECISION}}* const FR, // FR[{{NUMBER_OF_UNKNOWNS}}
      const {{CORRECTOR_COMPUTATION_PRECISION}}* const QL, // QL[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const {{CORRECTOR_COMPUTATION_PRECISION}}* const QR, // QR[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
      const double t,
      const double dt,
      const tarch::la::Vector<Dimensions, double>& x,
      const tarch::la::Vector<Dimensions, double>& h,
      const int direction,
      bool isBoundaryFace,
      int faceIndex) {
  {{RIEMANN_SOLVER_IMPLEMENTATION}}
}
{% endif %}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridConstructionStep() {
  assertion( _solverState == SolverState::GridConstruction );
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridConstructionStep() {}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridInitialisationStep() {
  if (_solverState==SolverState::GridInitialisation) {
    {{FINISH_TIME_STEP_IMPLEMENTATION}}
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::suspendSolversForOneGridSweep() {
  _solverState = SolverState::Suspended;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startTimeStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  switch (_solverState) {
    case SolverState::GridConstruction:
      _solverState = SolverState::GridInitialisation;
      break;
    case SolverState::GridInitialisation:
      _solverState = SolverState::InitialPrediction;
      break;
    case SolverState::Suspended:
    case SolverState::Plotting:
{% if USE_SEPARATE_SWEEPS is sameas True %}
      _solverState = SolverState::InitialPrediction;
{% else %}
      _solverState = (_minTimeStamp==0) ? SolverState::InitialPrediction : _solverState = SolverState::TimeStep;
{% endif %}
      break;
    case SolverState::InitialPrediction:
      _solverState = (_minCellHThisTimeStep < _maxCellHThisTimeStep) ?
        SolverState::PredictionOnHangingCells : _solverState = SolverState::TimeStep;
      break;
    case SolverState::PredictionOnHangingCells:
      _solverState = SolverState::TimeStep;
      break;
    case SolverState::TimeStep:
{% if USE_SEPARATE_SWEEPS is sameas True %}
      _solverState = SolverState::InitialPrediction;
{% else %}
      _solverState = (_minCellHThisTimeStep < _maxCellHThisTimeStep) ?
        SolverState::PredictionOnHangingCells : _solverState = SolverState::TimeStep;
{% endif %}
      break;
  }

  {{START_TIME_STEP_IMPLEMENTATION}}

  if (isFirstGridSweepOfTimeStep()) {
    _minTimeStamp = _minTimeStampThisTimeStep;
    _maxTimeStamp = _maxTimeStampThisTimeStep;

    _minTimeStampThisTimeStep = std::numeric_limits<double>::max();
    _maxTimeStampThisTimeStep = std::numeric_limits<double>::min();

    _minTimeStepSize = _minTimeStepSizeThisTimeStep;
    _maxTimeStepSize = _maxTimeStepSizeThisTimeStep;

    _minTimeStepSizeThisTimeStep = std::numeric_limits<double>::max();
    _maxTimeStepSizeThisTimeStep = std::numeric_limits<double>::min();

    _minCellH = _minCellHThisTimeStep;
    _maxCellH = _maxCellHThisTimeStep;

    _maxCellHThisTimeStep      = 0.0;
    _minCellHThisTimeStep      = std::numeric_limits<double>::max();

    _localMinTimeStampThisTimeStep = std::numeric_limits<double>::max();
    _localMaxTimeStampThisTimeStep = std::numeric_limits<double>::min();

    _cellUpdates    = 0;
  }

  if (tarch::mpi::Rank::getInstance().isGlobalMaster()) {
    logInfo("startTimeStep(...)", "phase " + toString(_solverState));
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
  if (isLastGridSweepOfTimeStep()) {
    #ifdef Parallel
    double newMinTimeStamp    = _minTimeStampThisTimeStep;
    double newMaxTimeStamp    = _maxTimeStampThisTimeStep;
    double newMinCellH        = _minCellHThisTimeStep;
    double newMaxCellH        = _maxCellHThisTimeStep;
    double newMinTimeStepSize = _minTimeStepSizeThisTimeStep;
    double newMaxTimeStepSize = _maxTimeStepSizeThisTimeStep;

    double newLocalMinTimeStamp = _localMinTimeStampThisTimeStep;
    double newLocalMaxTimeStamp = _localMaxTimeStampThisTimeStep;

    int newCellUpdates = _cellUpdates;

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
        &newMinCellH,
        &_minCellHThisTimeStep,
        1, MPI_DOUBLE,
        MPI_MIN,
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
        );
    tarch::mpi::Rank::getInstance().allReduce(
        &newMaxCellH,
        &_maxCellHThisTimeStep,
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
        &newCellUpdates,
        &_cellUpdates,
        1, MPI_INT,
        MPI_SUM,
        [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
        );
    #endif

    if (_minTimeStampThisTimeStep >= std::numeric_limits<double>::max() / 10.0) {
      _minTimeStampThisTimeStep = 0.0;
    }
    if (_maxTimeStampThisTimeStep >= std::numeric_limits<double>::max() / 10.0) {
      _maxTimeStampThisTimeStep = 0.0;
    }

    {{FINISH_TIME_STEP_IMPLEMENTATION}}
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startPlottingStep(
  double globalMinTimeStamp,
  double globalMaxTimeStamp,
  double globalMinTimeStepSize,
  double globalMaxTimeStepSize
) {
  assertion( mayPlot() );
  _solverState = SolverState::Plotting;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishPlottingStep() {}

std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::Plotting:
      return "plotting";
    case SolverState::InitialPrediction:
      return "initial-prediction";
    case SolverState::PredictionOnHangingCells:
      return "prediction-on-hanging-cells";
    case SolverState::TimeStep:
      return "time-step";
    }
  return "<undef>";
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mayPlot() const {
  return _solverState==SolverState::GridInitialisation or isLastGridSweepOfTimeStep();
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isFirstGridSweepOfTimeStep() const {
{% if USE_SEPARATE_SWEEPS is sameas True %}
  return _solverState==SolverState::InitialPrediction;
{% else %}
  return ( _solverState==SolverState::TimeStep
        or _solverState==SolverState::InitialPrediction);
{% endif %}
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isLastGridSweepOfTimeStep() const {
  return _solverState==SolverState::TimeStep;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSimulation() {}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishSimulation() {}

{{ABSTRACT_SOLVER_USER_DEFINITIONS}}
