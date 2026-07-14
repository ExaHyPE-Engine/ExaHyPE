// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "Constants.h"

#include "repositories/SolverRepository.h"

#include "tarch/mpi/Rank.h"
#include "tarch/mpi/mpi.h"
#include "tarch/multicore/Lock.h"
#include "tarch/services/ServiceRepository.h"

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _solverState(SolverState::GridConstruction),
  _numberOfTroubledCells(0),
  _numberOfTroubledCellsThisTimeStep(0) {
  {{CONSTRUCTOR_IMPLEMENTATION}}
}

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::~{{CLASSNAME}}() {}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStamp() const {
  return repositories::{{REGULAR_SOLVER_INSTANCE}}.getMinTimeStamp();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStamp() const {
  return repositories::{{REGULAR_SOLVER_INSTANCE}}.getMaxTimeStamp();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStepSize() const {
  return repositories::{{REGULAR_SOLVER_INSTANCE}}.getMinTimeStepSize();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStepSize() const {
  return repositories::{{REGULAR_SOLVER_INSTANCE}}.getMaxTimeStepSize();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxPatchSize() const {
  return repositories::{{REGULAR_SOLVER_INSTANCE}}.getMaxPatchSize();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinPatchSize() const {
  return repositories::{{REGULAR_SOLVER_INSTANCE}}.getMinPatchSize();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxCellSize() const {
  return repositories::{{REGULAR_SOLVER_INSTANCE}}.getMaxCellSize();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinCellSize() const {
  return repositories::{{REGULAR_SOLVER_INSTANCE}}.getMinCellSize();
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
    case SolverState::Suspended:
    case SolverState::Plotting:
    case SolverState::RegularSolver:
      // If the plotting happened, the regular solver should starts its routine even if _numberOfTroubledCellsThisTimeStep != 0.
      // - It is done, because in that case the plotting state happened right after the limiter solver state, where the same check is not performed.
      // - Otherwise we are stuck in the limiter solver, since _numberOfTroubledCellsThisTimeStep is set to zero in the regular solver state after the limiter.
      _solverState = ((_numberOfTroubledCellsThisTimeStep == 0) or (_solverState == SolverState::Plotting) ?
        SolverState::RegularSolver : SolverState::LimiterStatusSpreadingToNeighbours);
      break;
    case SolverState::LimiterStatusSpreadingToNeighbours:
      _solverState = SolverState::LimiterStatusSpreadingToSecondNeighbours;
      break;
    case SolverState::LimiterStatusSpreadingToSecondNeighbours:
      _solverState = SolverState::LimiterSolver;
      break;
    case SolverState::LimiterSolver:
      // TODO: Potential bug if the limiter solver has more than one grid sweeps.
      // - It will change the state to the regular solver even if only the first sweep is finished.
      // - Potentially to fix it we can store isFinished status in each solver, which returns true as soon as all sweeps are done.
      _solverState = SolverState::RegularSolver;
      break;
  }

  _numberOfTroubledCells = _numberOfTroubledCellsThisTimeStep;
  if (_solverState == SolverState::RegularSolver) {
    _numberOfTroubledCellsThisTimeStep = 0;
  }

  {{START_TIME_STEP_IMPLEMENTATION}}

  if (_solverState == SolverState::RegularSolver) {
    repositories::{{LIMITER_SOLVER_INSTANCE}}.suspendSolversForOneGridSweep();
    _numberOfTroubledCellsThisTimeStep = 0;
  }

  if (_solverState == SolverState::LimiterSolver) {
    repositories::{{REGULAR_SOLVER_INSTANCE}}.suspendSolversForOneGridSweep();
  }

  if (_solverState == SolverState::LimiterStatusSpreadingToNeighbours or _solverState == SolverState::LimiterStatusSpreadingToSecondNeighbours) {
    repositories::{{LIMITER_SOLVER_INSTANCE}}.suspendSolversForOneGridSweep();
    repositories::{{REGULAR_SOLVER_INSTANCE}}.suspendSolversForOneGridSweep();
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
  if (isLastGridSweepOfTimeStep()) {
#ifdef PEANO_WITH_MPI
    int local = _numberOfTroubledCellsThisTimeStep;
    int global = 0;
    MPI_Comm communicator = tarch::mpi::Rank::getInstance().getCommunicator();
    auto progress = [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); };
    tarch::mpi::allreduce(
      &local, &global, 1, MPI_INT, MPI_SUM,
      communicator,
      progress
    );
    _numberOfTroubledCellsThisTimeStep = global;
#endif

    {{FINISH_TIME_STEP_IMPLEMENTATION}}
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startPlottingStep() {
  assertion(mayPlot());
  _solverState = SolverState::Plotting;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishPlottingStep() {}

std::string {{NAMESPACE | join("::")}}::{{CLASSNAME}}::toString(SolverState state) {
  switch (state) {
    case SolverState::GridConstruction:
      return "grid-construction";
    case SolverState::GridInitialisation:
      return "grid-initialisation";
    case SolverState::RegularSolver:
      return "regular-(non-limiting)-solver";
    case SolverState::LimiterStatusSpreadingToNeighbours:
      return "limiter-status-spreading-to-neighbours";
    case SolverState::LimiterStatusSpreadingToSecondNeighbours:
      return "limiter-status-spreading-to-second-neighbours";
    case SolverState::LimiterSolver:
      return "limiting-solver";
    case SolverState::Plotting:
      return "plotting";
    case SolverState::Suspended:
      return "suspended";
  }
  return "<undef>";
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mayPlot() const {
  return (
    (_solverState == SolverState::GridInitialisation)
    or
    (_solverState == SolverState::RegularSolver and _numberOfTroubledCellsThisTimeStep == 0)
    or
    (_solverState == SolverState::LimiterSolver)
  );
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isFirstGridSweepOfTimeStep() const {
  return (
    ( _solverState==SolverState::RegularSolver and repositories::{{REGULAR_SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep() )
    or
    ( _solverState==SolverState::LimiterSolver and repositories::{{LIMITER_SOLVER_INSTANCE}}.isFirstGridSweepOfTimeStep() )
  );
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isLastGridSweepOfTimeStep() const {
  return (
    ( _solverState==SolverState::RegularSolver and repositories::{{REGULAR_SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep() )
    or
    ( _solverState==SolverState::LimiterSolver and repositories::{{LIMITER_SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep() )
  );
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSimulation() {
  kernels::{{SOLVER_NAME}}::Quadrature<{{COMPUTE_PRECISION}}>::initQuadratureNodesAndWeights();
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishSimulation() {
  kernels::{{SOLVER_NAME}}::Quadrature<{{COMPUTE_PRECISION}}>::freeQuadratureNodesAndWeights();
}

void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::addTroubledCell() {
  tarch::multicore::Lock lock(_semaphore);
  ++_numberOfTroubledCellsThisTimeStep;
}

int {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getNumberOfTroubledCells() const {
  return _numberOfTroubledCells;
}

{% if ADMISSIBILITY_IMPLEMENTATION!="<empty>" and ADMISSIBILITY_IMPLEMENTATION!="<user-defined>" %}
bool {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::isPhysicallyAdmissible(
  const {{REGULAR_SOLVER_STORAGE_PRECISION}}* const Q,
  const tarch::la::Vector<DIMENSIONS, double>&      x,
  const tarch::la::Vector<DIMENSIONS, double>&      h,
  const double                                      timeStamp
) {
  {% if ADMISSIBILITY_IMPLEMENTATION!="<none>"%}
  {{ADMISSIBILITY_IMPLEMENTATION}}
  {% else %}
  return true;
  {% endif %}
}
{% endif %}

{{SOLVER_USER_DEFINITIONS}}
