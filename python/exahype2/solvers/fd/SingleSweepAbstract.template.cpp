// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "repositories/SolverRepository.h"

#include "tarch/multicore/Lock.h"
#include "tarch/services/ServiceRepository.h"

std::bitset<DIMENSIONS> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::PeriodicBC = {{NAMESPACE | join("::")}}::PeriodicBC;

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _solverState(SolverState::GridConstruction),
  _minTimeStamp(0.0),
  _maxTimeStamp(0.0),
  _minTimeStampThisTimeStep(0.0),
  _maxTimeStampThisTimeStep(0.0),
  // This way, we still compute the patch size without an overflow
  _minH(std::numeric_limits<double>::max() / NumberOfNodesPerAxisPerPatch / 2.0),
  _maxH(0.0),
  _minHThisTimeStep(std::numeric_limits<double>::max() / NumberOfNodesPerAxisPerPatch / 2.0),
  _maxHThisTimeStep(0.0),
  _minTimeStepSize(std::numeric_limits<double>::max()),
  _maxTimeStepSize(0.0),
  _minTimeStepSizeThisTimeStep(std::numeric_limits<double>::max()),
  _maxTimeStepSizeThisTimeStep(0.0) {

  {{CONSTRUCTOR_IMPLEMENTATION}}
}


void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::suspendSolversForOneGridSweep() {
  _solverState = SolverState::Suspended;
}


double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxMeshSize() const {
  return getMaxPatchSize();
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinMeshSize() const {
  return getMinPatchSize();
}

int {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getNumberOfDoFs() const {
  return BasePowerExponent<NumberOfNodesPerAxisPerPatch, DIMENSIONS>::value * (NumberOfUnknowns + NumberOfAuxiliaryVariables);
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinPatchSize() const {
  return getMinH() * NumberOfNodesPerAxisPerPatch;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxPatchSize() const {
  return getMaxH() * NumberOfNodesPerAxisPerPatch;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinH() const {
  return _minH;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxH() const {
  return _maxH;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStamp() const {
  return _minTimeStampThisTimeStep < std::numeric_limits<double>::max() / 10.0 ? _minTimeStampThisTimeStep : _minTimeStamp;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStamp() const {
  return _minTimeStampThisTimeStep < std::numeric_limits<double>::max() / 10.0 ? _maxTimeStampThisTimeStep : _maxTimeStamp;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinTimeStepSize() const {
  return _minTimeStepSize;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxTimeStepSize() const {
  return _maxTimeStepSize;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::update(double timeStepSize, double timeStamp, double minH, double maxH) {
  tarch::multicore::Lock lock(_semaphore);

  if (tarch::la::greater(timeStepSize, 0.0)) {
    assertion1(timeStepSize < std::numeric_limits<double>::max() / 10.0, timeStepSize);
    assertion1(timeStepSize >= 0.0, timeStepSize);
    _minTimeStepSizeThisTimeStep = std::min(timeStepSize, _minTimeStepSizeThisTimeStep);
    _maxTimeStepSizeThisTimeStep = std::max(timeStepSize, _maxTimeStepSizeThisTimeStep);
    assertion2(_minTimeStepSizeThisTimeStep <= _maxTimeStepSizeThisTimeStep, _minTimeStepSizeThisTimeStep, _maxTimeStepSizeThisTimeStep);

    _localMinTimeStampThisTimeStep = std::min(_localMinTimeStampThisTimeStep, timeStamp);
    _localMaxTimeStampThisTimeStep = std::max(_localMaxTimeStampThisTimeStep, timeStamp);
  }

  assertion1(timeStamp < std::numeric_limits<double>::max() / 10.0, timeStamp);
  assertion1(timeStamp >= 0.0, timeStamp);
  _minTimeStampThisTimeStep = std::min(timeStamp, _minTimeStampThisTimeStep);
  _maxTimeStampThisTimeStep = std::max(timeStamp, _maxTimeStampThisTimeStep);
  assertion2(_minTimeStampThisTimeStep <= _maxTimeStampThisTimeStep, _minTimeStampThisTimeStep, _maxTimeStampThisTimeStep);

  assertion1(maxH < std::numeric_limits<double>::max() / NumberOfNodesPerAxisPerPatch, maxH);
  assertion1(minH > 0.0, minH);
  _minHThisTimeStep = std::min(_minHThisTimeStep, minH / NumberOfNodesPerAxisPerPatch);
  _maxHThisTimeStep = std::max(_maxHThisTimeStep, maxH / NumberOfNodesPerAxisPerPatch);
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
  assertion(_solverState == SolverState::GridConstruction);
  _solverState = SolverState::GridInitialisation;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishGridInitialisationStep() {}

/**
 * @coupling: Solvers switch their state according to coupled solvers
 */
void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startTimeStep() {
  logDebug("old state of " << "{{SOLVER_NAME}}" << " is " << toString(_solverState));
  switch (_solverState) {
    case SolverState::Suspended:
      // Checks if _solverState is in the Suspended state and return.
      // Prevents nullification of eigenvalues if the solver is suspended

      {% if SOLVERS_BEFORE_IN_COUPLING | length == 0 -%}
      // this solver can solve in the very first grid sweep of the TimeStep observer
      // as no solvers come before it in the coupling
      // in this case, this just means that the eigenvalue is not set to 0
      // as the solver was suspended before or is already done with solving
      _solverState = SolverState::IsSolving;
      break;
      {% else -%}
      _solverState = SolverState::StillNeedsToSolve;
      return;
      {%- endif %}

    case SolverState::GridInitialisation:
      {% if SOLVERS_BEFORE_IN_COUPLING | length == 0 -%}
      // this solver can solve in the very first grid sweep of the TimeStep observer
      // as no solvers come before it in the coupling
      _solverState = SolverState::IsSolvingAfterGridInitialisation;
      break;
      {% else -%}
      _solverState = SolverState::StillNeedsToSolveAfterGridInitialisation;
      return;
      {%- endif %}

    case SolverState::StillNeedsToSolveAfterGridInitialisation:
      /** 
       * this might turn into a race condition if the solvers
       * that are being querried here only change their state
       * after this one so be careful with the order in which 
       * the solvers are passed into the project
      */
      {
        bool maySolve = true;
        {% for item in SOLVERS_BEFORE_IN_COUPLING -%}
        maySolve &= repositories::{{item[0]}}.getSolverState() == Abstract{{item[1]}}::SolverState::HasSolved;
        {%- endfor %}
        if (not maySolve) {
          // do not change state
          _solverState = SolverState::StillNeedsToSolveAfterGridInitialisation;
          return;  
        } 
      }
      _solverState = SolverState::IsSolvingAfterGridInitialisation;
      break;

    case SolverState::StillNeedsToSolve:
      /** 
       * this might turn into a race condition if the solvers
       * that are being querried here only change their state
       * after this one so be careful with the order in which 
       * the solvers are passed into the project
      */
      {
        bool maySolve = true;
        {% for item in SOLVERS_BEFORE_IN_COUPLING -%}
        maySolve &= repositories::{{item[0]}}.getSolverState() == Abstract{{item[1]}}::SolverState::HasSolved;
        {%- endfor %}
        if (not maySolve) {
          // do not change state
          _solverState = SolverState::StillNeedsToSolve;
          return;
        } 
      }

      _solverState = SolverState::IsSolving;
      break;
    
    case SolverState::IsSolvingAfterGridInitialisation:
    case SolverState::IsSolving:
      _solverState = SolverState::HasSolved;
      return;

    case SolverState::HasSolved:
      // nothing to do anymore
      _solverState = SolverState::HasSolved;
      return;
    
    default:
      // all other solver states such as plotting should lead to this
      
      /** 
       * this might turn into a race condition if the solvers
       * that are being querried here only change their state
       * after this one so be careful with the order in which 
       * the solvers are passed into the project
      */
      {
        bool maySolve = true;
        {% for item in SOLVERS_BEFORE_IN_COUPLING -%}
        maySolve &= repositories::{{item[0]}}.getSolverState() == Abstract{{item[1]}}::SolverState::HasSolved;
        {%- endfor %}
        if (not maySolve) {
          // wait until allowed to solve
          _solverState = SolverState::StillNeedsToSolve;
          return;
        } 
      }

      _solverState = SolverState::IsSolving;
      break;
  }

  {{START_TIME_STEP_IMPLEMENTATION}}

  _minTimeStampThisTimeStep = std::numeric_limits<double>::max();
  _maxTimeStampThisTimeStep = std::numeric_limits<double>::min();

  _minTimeStepSizeThisTimeStep = std::numeric_limits<double>::max();
  _maxTimeStepSizeThisTimeStep = std::numeric_limits<double>::min();

  _maxHThisTimeStep      = 0.0;
  _minHThisTimeStep      = std::numeric_limits<double>::max() / NumberOfNodesPerAxisPerPatch / 2.0;

  _localMinTimeStampThisTimeStep = std::numeric_limits<double>::max();
  _localMaxTimeStampThisTimeStep = std::numeric_limits<double>::min();
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
#ifdef WITH_MPI
  if (isLastGridSweepOfTimeStep()) {
    double newMinTimeStamp = _minTimeStampThisTimeStep;
    double newMaxTimeStamp = _maxTimeStampThisTimeStep;
    double newMinH   = _minHThisTimeStep;
    double newMaxH   = _maxHThisTimeStep;
    double newMinTimeStepSize = _minTimeStepSizeThisTimeStep;
    double newMaxTimeStepSize = _maxTimeStepSizeThisTimeStep;

    double newLocalMinTimeStamp = _localMinTimeStampThisTimeStep;
    double newLocalMaxTimeStamp = _localMaxTimeStampThisTimeStep;

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
      &newMinH,
      &_minHThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MIN,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );
    tarch::mpi::Rank::getInstance().allReduce(
      &newMaxH,
      &_maxHThisTimeStep,
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
  }
 #endif

  if (_minTimeStampThisTimeStep >= std::numeric_limits<double>::max() / 10.0) {
    _minTimeStampThisTimeStep = 0.0;
  }
  if (_maxTimeStampThisTimeStep >= std::numeric_limits<double>::max() / 10.0) {
    _maxTimeStampThisTimeStep = 0.0;
  }

  {{FINISH_TIME_STEP_IMPLEMENTATION}}

  if (isLastGridSweepOfTimeStep()) {
    _minTimeStamp    = _minTimeStampThisTimeStep;
    _maxTimeStamp    = _maxTimeStampThisTimeStep;
    _minTimeStepSize = _minTimeStepSizeThisTimeStep;
    _maxTimeStepSize = _maxTimeStepSizeThisTimeStep;
    _minH            = _minHThisTimeStep;
    _maxH            = _maxHThisTimeStep;
  }

  // reset solvers if last one has finished solving
  if 
    (
      repositories::{{LAST_SOLVER[0]}}.getSolverState() == Abstract{{LAST_SOLVER[1]}}::SolverState::IsSolving
      or
      repositories::{{LAST_SOLVER[0]}}.getSolverState() == Abstract{{LAST_SOLVER[1]}}::SolverState::IsSolvingAfterGridInitialisation
    ) {
    _solverState = SolverState::StillNeedsToSolve;
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::logTimeStep() {
  {{PRINT_TIME_STEP_IMPLEMENTATION}}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startPlottingStep() {
  if ( _solverState== SolverState::GridInitialisation) {
    _solverState = SolverState::PlottingAfterGridInitialisation;
  } else {
    assertion(mayPlot() or tarch::hasNonCriticalAssertionBeenViolated());
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
    case SolverState::StillNeedsToSolveAfterGridInitialisation:
      return "{{SOLVER_NAME}}-still-needs-to-solve-after-grid-initialisation";
    case SolverState::StillNeedsToSolve:
      return "{{SOLVER_NAME}}-still-needs-to-solve";
    case SolverState::IsSolvingAfterGridInitialisation:
      return "{{SOLVER_NAME}}-is-solving-after-grid-initialisation";
    case SolverState::IsSolving:
      return "{{SOLVER_NAME}}-is-solving";
    case SolverState::HasSolved:
      return "{{SOLVER_NAME}}-has-solved";
    case SolverState::Plotting:
      return "plotting";
    case SolverState::PlottingAfterGridInitialisation:
      return "plotting-after-grid-initialisation";
    case SolverState::Suspended:
      return "suspended";
  }
  return "<undef>";
}

/**
 * @coupling: Plotting is always possible either before any solver starts solving 
 * or after the last solver was in its IsSolving state and reset all solvers to StillNeedsToSolve in 
 * finishTimeStep()
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mayPlot() const {
  // after the last solver, all solvers revert back to StillNeedsToSolve so Peano may plot then
  return _solverState == SolverState::StillNeedsToSolve or _solverState == SolverState::StillNeedsToSolveAfterGridInitialisation; 
}

/**
 * @coupling: Each solver sees its IsSolving grid sweep as the first grid sweep of the time step
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isFirstGridSweepOfTimeStep() const {
  return _solverState == SolverState::IsSolving or _solverState == SolverState::IsSolvingAfterGridInitialisation;
}

/**
 * @coupling: Each solver sees its IsSolving grid sweep as the last grid sweep of the time step
 */
bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isLastGridSweepOfTimeStep() const {
  // last GridSweepOfTimeStep for this solver is during it's solving run
  return _solverState == SolverState::IsSolving or _solverState == SolverState::IsSolvingAfterGridInitialisation;
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSimulation() {
  {{START_SIMULATION_IMPLEMENTATION}}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishSimulation() {
  {{FINISH_SIMULATION_IMPLEMENTATION}}
}

{{ABSTRACT_SOLVER_USER_DEFINITIONS}}
