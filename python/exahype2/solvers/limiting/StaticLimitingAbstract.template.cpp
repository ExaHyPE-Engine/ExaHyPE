// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "Constants.h"
#include "repositories/SolverRepository.h"

#include "tarch/multicore/Lock.h"
#include "tarch/services/ServiceRepository.h"

#include <algorithm>

tarch::logging::Log {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log("{{NAMESPACE | join("::")}}::{{CLASSNAME}}");

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _solverState(SolverState::GridConstruction),
  _numberOfTroubledCells(0) {

  kernels::{{SOLVER_NAME}}::Quadrature<{{COMPUTE_PRECISION}}>::initQuadratureNodesAndWeights();

  {{CONSTRUCTOR_IMPLEMENTATION}}
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}(){
  kernels::{{SOLVER_NAME}}::Quadrature<{{COMPUTE_PRECISION}}>::freeQuadratureNodesAndWeights();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return repositories::{{REGULAR_SOLVER_INSTANCE}}.getMinTimeStamp();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStamp(bool ofCurrentlyRunningGridSweep) const {
  return repositories::{{REGULAR_SOLVER_INSTANCE}}.getMaxTimeStamp();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStepSize() const {
  return repositories::{{REGULAR_SOLVER_INSTANCE}}.getMinTimeStepSize();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStepSize() const {
  return repositories::{{REGULAR_SOLVER_INSTANCE}}.getMaxTimeStepSize();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxMeshSize() const {
  return repositories::{{REGULAR_SOLVER_INSTANCE}}.getMaxMeshSize();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinMeshSize() const {
  return repositories::{{REGULAR_SOLVER_INSTANCE}}.getMinMeshSize();
}

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getNumberOfDoFs() const {
  return 0;
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::SolverState {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getSolverState() const {
  return _solverState;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridConstructionStep() {
  assertion( _solverState == SolverState::GridConstruction );
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridConstructionStep() {}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startGridInitialisationStep() {
  _solverState = SolverState::GridInitialisation;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridInitialisationStep() {}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::suspendSolversForOneGridSweep() {
  _solverState = SolverState::Suspended;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startTimeStep() {
  switch (_solverState) {
    case SolverState::GridConstruction:
      _solverState = SolverState::GridInitialisation;
      break;
    case SolverState::GridInitialisation:
    case SolverState::PlottingAfterGridInitialisation:
      _solverState = SolverState::LimiterStatusSpreadingToNeighbours;
      repositories::{{LIMITER_SOLVER_INSTANCE}}.suspendSolversForOneGridSweep();
      repositories::{{REGULAR_SOLVER_INSTANCE}}.suspendSolversForOneGridSweep();
      break;
    case SolverState::LimiterStatusSpreadingToNeighbours:
      repositories::{{LIMITER_SOLVER_INSTANCE}}.suspendSolversForOneGridSweep();
    case SolverState::TimeStep:
    case SolverState::Plotting:
    case SolverState::Suspended:
      _solverState = SolverState::TimeStep;
      break;
  }

  {{START_TIME_STEP_IMPLEMENTATION}}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
#ifdef WITH_MPI
  if (_solverState == SolverState::LimiterStatusSpreadingToNeighbours) {
    int local = _numberOfTroubledCells;
    int global = 0;
    tarch::mpi::Rank::getInstance().allReduce(
      &local, &global, 1, MPI_INT, MPI_SUM,
      [&]{ tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );
    _numberOfTroubledCells = global;
  }
#endif

  if (isLastGridSweepOfTimeStep()) {
    {{FINISH_TIME_STEP_IMPLEMENTATION}}
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::printTimeStep(bool ofCurrentlyRunningGridSweep) {
  {{PRINT_TIME_STEP_IMPLEMENTATION}}

  if (getNumberOfTroubledCells() > 0) {
    logInfo("printTimeStep()", "Solver {{SOLVER_NAME}}:");
    logInfo("printTimeStep()", "Number of troubled cells: " << getNumberOfTroubledCells());
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startPlottingStep() {
  if ( _solverState == SolverState::GridInitialisation) {
    _solverState = SolverState::PlottingAfterGridInitialisation;
  } else {
    _solverState = SolverState::Plotting;
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishPlottingStep() {}

std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::PlottingAfterGridInitialisation:
      return "plotting-after-grid-initialisation";
    case SolverState::LimiterStatusSpreadingToNeighbours:
      return "limiter-status-spreading-to-neighbours";
    case SolverState::TimeStep:
      return "time-step";
    case SolverState::Plotting:
      return "plotting";
    case SolverState::Suspended:
      return "suspended";
  }
  return "<undef>";
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mayPlot() const {
  return _solverState!=SolverState::LimiterStatusSpreadingToNeighbours;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isFirstGridSweepOfTimeStep() const {
  return (
    _solverState != SolverState::LimiterStatusSpreadingToNeighbours
    and
    repositories::{{REGULAR_SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep()
    and
    repositories::{{LIMITER_SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep()
  );
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isLastGridSweepOfTimeStep() const {
  return (
    _solverState != SolverState::LimiterStatusSpreadingToNeighbours
    and
    repositories::{{REGULAR_SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
    and
    repositories::{{LIMITER_SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep()
  );
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSimulation() {}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishSimulation() {}

void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::addTroubledCell(){
  tarch::multicore::Lock lock(_semaphore);
  ++_numberOfTroubledCells;
}

int {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getNumberOfTroubledCells() const {
  return _numberOfTroubledCells;
}

{% if ADMISSIBILITY_IMPLEMENTATION!="<empty>" and ADMISSIBILITY_IMPLEMENTATION!="<user-defined>" %}
bool {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::isPhysicallyAdmissible(
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const Q,
  const tarch::la::Vector<DIMENSIONS, double>&      x,
  const tarch::la::Vector<DIMENSIONS, double>&      h,
  const double timeStamp
){
  {% if ADMISSIBILITY_IMPLEMENTATION!="<none>"%}
  {{ADMISSIBILITY_IMPLEMENTATION}}
  {% else %}
  return true;
  {% endif %}
}
{% endif %}

{{SOLVER_USER_DEFINITIONS}}
