// This file is part of the ExaHyPE2 project. For conditions of distribution and
// use, please see the copyright notice at www.peano-framework.org
#include "{{CLASSNAME}}.h"

#include "repositories/SolverRepository.h"
#include "tarch/multicore/Lock.h"

tarch::logging::Log     {{NAMESPACE | join("::")}}::{{CLASSNAME}}::_log("{{NAMESPACE | join("::")}}::{{CLASSNAME}}");

std::bitset<DIMENSIONS> {{NAMESPACE | join("::")}}::{{CLASSNAME}}::PeriodicBC = {{NAMESPACE | join("::")}}::PeriodicBC;

{{NAMESPACE | join("::")}}::{{CLASSNAME}}::{{CLASSNAME}}():
  _solverState(SolverState::GridConstruction),
  _minTimeStamp(0.0),
  _maxTimeStamp(0.0),
  _minTimeStampThisTimeStep(0.0),
  _maxTimeStampThisTimeStep(0.0),
  // This way, we still compute the patch size without an overflow
  _minVolumeH(std::numeric_limits<double>::max() / NumberOfFiniteVolumesPerAxisPerPatch / 2.0),
  _maxVolumeH(0.0),
  _minVolumeHThisTimeStep(std::numeric_limits<double>::max() / NumberOfFiniteVolumesPerAxisPerPatch / 2.0),
  _maxVolumeHThisTimeStep(0.0),
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

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinPatchSize(bool ofCurrentlyRunningGridSweep) const {
  return getMinVolumeSize(ofCurrentlyRunningGridSweep) * NumberOfFiniteVolumesPerAxisPerPatch;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxPatchSize(bool ofCurrentlyRunningGridSweep) const {
  return getMaxVolumeSize(ofCurrentlyRunningGridSweep) * NumberOfFiniteVolumesPerAxisPerPatch;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMinVolumeSize(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _minVolumeHThisTimeStep : _minVolumeH;
}

double {{NAMESPACE | join("::")}}::{{CLASSNAME}}::getMaxVolumeSize(bool ofCurrentlyRunningGridSweep) const {
  return ofCurrentlyRunningGridSweep ? _maxVolumeHThisTimeStep : _maxVolumeH;
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

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::update(double timeStepSize, double timeStamp, double minVolumeSize, double maxVolumeSize) {
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

  assertion1(maxVolumeSize < std::numeric_limits<double>::max() / NumberOfFiniteVolumesPerAxisPerPatch, maxVolumeSize);
  assertion1(minVolumeSize > 0.0, minVolumeSize);
  _minVolumeHThisTimeStep = std::min(_minVolumeHThisTimeStep, minVolumeSize / NumberOfFiniteVolumesPerAxisPerPatch);
  _maxVolumeHThisTimeStep = std::max(_maxVolumeHThisTimeStep, maxVolumeSize / NumberOfFiniteVolumesPerAxisPerPatch);
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

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startTimeStep() {
  switch(_solverState) {
  case SolverState::Suspended:
    // Checks if _solverState is in the Suspended state and return.
    // Prevents nullification of eigenvalues if the solver is suspended.

{% if SOLVERS_BEFORE_IN_COUPLING | length == 0 -%}
    // This solver can solve in the very first grid sweep of the TimeStep observer
    // as no solvers come before it in the coupling.
    // In this case, this just means that the eigenvalue is not set to 0
    // as the solver was suspended before or is already done with solving.
    _solverState = SolverState::IsSolving;
{% else -%}
    _solverState = SolverState::StillNeedsToSolve;
{%- endif %}
    return;
  case SolverState::GridInitialisation:
{% if SOLVERS_BEFORE_IN_COUPLING | length == 0 -%}
    // This solver can solve in the very first grid sweep of the TimeStep observer
    // as no solvers come before it in the coupling.
    _solverState = SolverState::IsSolvingAfterGridInitialisation;
    break;
{% else -%}
    _solverState = SolverState::StillNeedsToSolveAfterGridInitialisation;
    return;
{%- endif %}
  case SolverState::StillNeedsToSolveAfterGridInitialisation:
    {
      bool maySolve = true;
{% for item in SOLVERS_BEFORE_IN_COUPLING -%}
      maySolve &= repositories::{{item[0]}}.getSolverState() == Abstract{{item[1]}}::SolverState::HasSolved;
{%- endfor %}
      if (not maySolve) {
        // Do not change state
        _solverState = SolverState::StillNeedsToSolveAfterGridInitialisation;
        return;
      }
    }
    _solverState = SolverState::IsSolvingAfterGridInitialisation;
    break;
  case SolverState::StillNeedsToSolve:
    {
      bool maySolve = true;
{% for item in SOLVERS_BEFORE_IN_COUPLING -%}
      maySolve &= repositories::{{item[0]}}.getSolverState() == Abstract{{item[1]}}::SolverState::HasSolved;
{%- endfor %}
      if (not maySolve) {
        // Do not change state
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
    // Nothing to do anymore
    _solverState = SolverState::HasSolved;
    return;
  default:
    // All other solver states such as plotting should lead to this
    {
      bool maySolve = true;
{% for item in SOLVERS_BEFORE_IN_COUPLING -%}
      maySolve &= repositories::{{item[0]}}.getSolverState() == Abstract{{item[1]}}::SolverState::HasSolved;
{%- endfor %}
      if (not maySolve) {
        // Wait until allowed to solve
        _solverState = SolverState::StillNeedsToSolve;
        return;
      }
    }
    _solverState = SolverState::IsSolving;
    break;
  }

  {{START_TIME_STEP_IMPLEMENTATION}}

  _minTimeStamp = _minTimeStampThisTimeStep;
  _maxTimeStamp = _maxTimeStampThisTimeStep;

  _minTimeStampThisTimeStep = std::numeric_limits<double>::max();
  _maxTimeStampThisTimeStep = std::numeric_limits<double>::min();

  _minTimeStepSize = _minTimeStepSizeThisTimeStep;
  _maxTimeStepSize = _maxTimeStepSizeThisTimeStep;

  _minTimeStepSizeThisTimeStep = std::numeric_limits<double>::max();
  _maxTimeStepSizeThisTimeStep = std::numeric_limits<double>::min();

  _minVolumeH = _minVolumeHThisTimeStep;
  _maxVolumeH = _maxVolumeHThisTimeStep;

  _maxVolumeHThisTimeStep = 0.0;
  _minVolumeHThisTimeStep = std::numeric_limits<double>::max() / NumberOfFiniteVolumesPerAxisPerPatch / 2.0;

  _localMinTimeStampThisTimeStep = std::numeric_limits<double>::max();
  _localMaxTimeStampThisTimeStep = std::numeric_limits<double>::min();
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishTimeStep() {
#ifdef WITH_MPI
  if (isLastGridSweepOfTimeStep()) {
    double newMinTimeStamp = _minTimeStampThisTimeStep;
    double newMaxTimeStamp = _maxTimeStampThisTimeStep;
    double newMinVolumeH   = _minVolumeHThisTimeStep;
    double newMaxVolumeH   = _maxVolumeHThisTimeStep;
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
      &newMinVolumeH,
      &_minVolumeHThisTimeStep,
      1, MPI_DOUBLE,
      MPI_MIN,
      [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
    );
    tarch::mpi::Rank::getInstance().allReduce(
      &newMaxVolumeH,
      &_maxVolumeHThisTimeStep,
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

  // Reset solvers if last one has finished solving
  if
    (
      repositories::{{LAST_SOLVER[0]}}.getSolverState() == Abstract{{LAST_SOLVER[1]}}::SolverState::IsSolving
      or
      repositories::{{LAST_SOLVER[0]}}.getSolverState() == Abstract{{LAST_SOLVER[1]}}::SolverState::IsSolvingAfterGridInitialisation
    ) {
    _solverState = SolverState::StillNeedsToSolve;
    logDebug("finishTimeStep()", "Solver {{SOLVER_NAME}} reset due to solver {{LAST_SOLVER[1]}} being in its IsSolving state");
  }
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::printTimeStep(bool ofCurrentlyRunningGridSweep) {
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

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::mayPlot() const {
  return
    _solverState == SolverState::GridInitialisation
    or
    // After the last solver, all solvers revert back to StillNeedsToSolve so we may plot then
    _solverState == SolverState::StillNeedsToSolve or _solverState == SolverState::StillNeedsToSolveAfterGridInitialisation;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isFirstGridSweepOfTimeStep() const {
  return _solverState == SolverState::IsSolving or _solverState == SolverState::IsSolvingAfterGridInitialisation;
}

bool {{NAMESPACE | join("::")}}::{{CLASSNAME}}::isLastGridSweepOfTimeStep() const {
  // Last GridSweepOfTimeStep for this solver is during it's solving run
  return
    (_solverState != SolverState::Suspended)
    and
    (_solverState == SolverState::IsSolving or _solverState == SolverState::IsSolvingAfterGridInitialisation);
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::startSimulation() {
  {{START_SIMULATION_IMPLEMENTATION}}
}

void {{NAMESPACE | join("::")}}::{{CLASSNAME}}::finishSimulation() {
  {{FINISH_SIMULATION_IMPLEMENTATION}}
}

{{ABSTRACT_SOLVER_USER_DEFINITIONS}}
