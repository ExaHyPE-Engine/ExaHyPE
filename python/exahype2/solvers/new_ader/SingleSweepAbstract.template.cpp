#include <algorithm>


#include "{{CLASSNAME}}.h"
#include "Constants.h"


#include "tarch/multicore/Lock.h"
#include "exahype2/NonCriticalAssertions.h"


tarch::logging::Log   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}::_log( "{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}" );

std::bitset<Dimensions> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}::PeriodicBC = {{NAMESPACE | join("::")}}::PeriodicBC;

double**   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::weights                                 = kernels::{{POLYNOMIAL_TYPE}}::weights;
double**   {{NAMESPACE | join("::")}}::{{CLASSNAME}}::nodes                                   = kernels::{{POLYNOMIAL_TYPE}}::nodes;
double***  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::Kxi                                     = kernels::{{POLYNOMIAL_TYPE}}::Kxi;
double***  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::dudx                                    = kernels::{{POLYNOMIAL_TYPE}}::dudx;
double***  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::iK1                                     = kernels::{{POLYNOMIAL_TYPE}}::iK1;
double***  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::equidistantGridProjector                = kernels::{{POLYNOMIAL_TYPE}}::equidistantGridProjector;
double***  {{NAMESPACE | join("::")}}::{{CLASSNAME}}::FCoeff                                  = kernels::{{POLYNOMIAL_TYPE}}::FCoeff;
double**** {{NAMESPACE | join("::")}}::{{CLASSNAME}}::fineGridProjector                       = kernels::{{POLYNOMIAL_TYPE}}::fineGridProjector;
kernels::UnivariateFunction** {{NAMESPACE | join("::")}}::{{CLASSNAME}}::basisFunction                 = kernels::{{POLYNOMIAL_TYPE}}::basisFunction;
kernels::UnivariateFunction** {{NAMESPACE | join("::")}}::{{CLASSNAME}}::basisFunctionFirstDerivative  = kernels::{{POLYNOMIAL_TYPE}}::basisFunctionFirstDerivative;
kernels::UnivariateFunction** {{NAMESPACE | join("::")}}::{{CLASSNAME}}::basisFunctionSecondDerivative = kernels::{{POLYNOMIAL_TYPE}}::basisFunctionSecondDerivative;


{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
{{BASIS_INITIALIZERS | indent(2,True) }}
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

  {% if USE_KERNEL_GENERATOR==True %}
  //initialise nodes, weights and matrices for use of the kernels
  generated::kernels::AderDG::{{LINEARITY}}::initQuadratureNodesAndWeights();
  generated::kernels::AderDG::{{LINEARITY}}::initDGMatrices();
  {% endif %}

}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}(){

  {% if USE_KERNEL_GENERATOR==True %}
  //free the generated nodes, weights and matrices
  generated::kernels::AderDG::{{LINEARITY}}::freeQuadratureNodesAndWeights();
  generated::kernels::AderDG::{{LINEARITY}}::freeDGMatrices();
  {% endif %}
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
  const double * __restrict__ Q,    // Q[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}],
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
  double * __restrict__ Q,
  const tarch::la::Vector<Dimensions,double>&  x,
  bool                                         gridIsConstructred
) {
  {{INITIAL_CONDITIONS_IMPLEMENTATION}}
}
{% endif %}


{% if BOUNDARY_CONDITIONS_IMPLEMENTATION!="<user-defined>" %}
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::boundaryConditions(
  const double * __restrict__                  Qinside,   // Qinside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  double * __restrict__                        Qoutside,  // Qoutside[{{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}]
  const tarch::la::Vector<Dimensions,double>&  x,
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
  _solverState = SolverState::GridInitialisation;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridInitialisationStep() {
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
      _solverState = SolverState::Prediction;
      break;
    case SolverState::Plotting:
      _solverState = SolverState::Prediction;
      break;
    case SolverState::Prediction:
      _solverState = SolverState::Correction;
      break;
    case SolverState::Correction:
      _solverState = SolverState::Prediction;
      break;
  }

  {{START_TIME_STEP_IMPLEMENTATION}}

  if (_solverState==SolverState::Prediction) {
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

  logInfo( "startTimeStep(...)", "phase " + toString(_solverState) );

}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
	if ( _solverState==SolverState::Correction ) {

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
      &newMaxTimeSteSweepAbstract.template.cpp
			modified:   SingleSweeamp,
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

		if ( _minTimeStampThisTimeStep>=std::numeric_limits<double>::max()/10.0) {
			_minTimeStampThisTimeStep = 0.0;
		}
		if ( _maxTimeStampThisTimeStep>=std::numeric_limits<double>::max()/10.0 ) {
			_maxTimeStampThisTimeStep = 0.0;
		}
	}

  {{FINISH_TIME_STEP_IMPLEMENTATION}}
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


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishPlottingStep() {
}


std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::Plotting:
      return "plotting";
    case SolverState::Prediction:
      return "prediction";
      break;
    case SolverState::Correction:
			return "correction";
  }
  return "<undef>";
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mayPlot() const {
  return isLastGridSweepOfTimeStep()
      or _solverState==SolverState::GridInitialisation
      or _solverState==SolverState::Plotting;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isFirstGridSweepOfTimeStep() const {
  return _solverState==SolverState::Prediction;
}


bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isLastGridSweepOfTimeStep() const {
  return _solverState==SolverState::Correction;
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSimulation() {

}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishSimulation() {

}



{% for PRECISION_NUM in range(0,COMPUTATION_PRECISIONS|length) %}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::viscousFlux(const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const Q, const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const gradQ, {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}** const F) {
  // should not be called
  exit(1);
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::viscousEigenvalues(const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const Q, const int direction, {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const lambda) {
  //should not be called
  exit(1);
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::multiplyMaterialParameterMatrix(const {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}* const Q, {{COMPUTATION_PRECISIONS[PRECISION_NUM]}}** const rhs){
  //should not be called
  exit(1);
}
{% endfor %}


{{ABSTRACT_SOLVER_USER_DEFINITIONS}}
