# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.solvers.PDETerms import PDETerms
import jinja2


def create_abstract_solver_user_declarations_for_fixed_time_stepping():
    return """
private:
  double _timeStepSize;
public:
  double getTimeStepSize() const;
    """


def create_abstract_solver_user_definitions_for_fixed_time_stepping():
    return """
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getTimeStepSize() const {
  return _timeStepSize;
}
    """


def create_compute_time_step_size_for_fixed_time_stepping(_time_step_size):
    return """
  const double timeStepSize = """ + str(
        _time_step_size
    )


def create_start_time_step_implementation_for_fixed_time_stepping(use_enclave_tasking):
    """
    The outcome is used before we actually roll over the accumulation variables
    and other stuff.
    """
    predicate = """
    tarch::mpi::Rank::getInstance().isGlobalMaster()
    and
    _maxCellH > 0.0
    and
    isFirstGridSweepOfTimeStep()
  """

    if use_enclave_tasking:
        predicate += """and (_solverState == SolverState::Primary or _solverState == SolverState::PrimaryAfterGridInitialisation) """

    return (
        """
  if ("""
        + predicate
        + """) {
    logInfo("startTimeStep()", "Solver {{SOLVER_NAME}}:");
    logInfo("startTimeStep()", "t       = " << _minTimeStampThisTimeStep);
    logInfo("startTimeStep()", "dt      = " << getTimeStepSize());
    logInfo("startTimeStep()", "h_{min} = " << _minCellH);
    logInfo("startTimeStep()", "h_{max} = " << _maxCellH);
  }
"""
    )


def create_finish_time_step_implementation_for_fixed_time_stepping(
    normalised_time_step_size,
):
    return (
        """
  assertion(_minCellH >= 0.0);
  assertion(MaxAdmissibleCellH > 0.0);
  if (_minCellH <= MaxAdmissibleCellH) {
    _timeStepSize = """
        + str(normalised_time_step_size)
        + """ * _minCellH / MaxAdmissibleCellH;
  } else {
    _timeStepSize = 0.0;
  }
"""
    )


def create_abstract_solver_user_declarations_for_adaptive_time_stepping():
    return """
private:
  double _maxEigenvalue;
  double _admissibleTimeStepSize;
public:
  void setMaxEigenvalue(double eigenvalue);
  /**
   * @return Admissible time step size for the current sweep, i.e.
   *         return _admissibleTimeStepSize. This value always refers
   *         to the minimum mesh volume size. If you use subcycling,
   *         you have to scale it for cells that are not on the finest
   *         mesh resolution.
   */
  virtual double getAdmissibleTimeStepSize() const;
"""


def create_abstract_solver_user_definitions_for_adaptive_time_stepping():
    return """
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::setMaxEigenvalue(double eigenvalue) {
  if (tarch::la::greater(eigenvalue, 0.0)) {
    tarch::multicore::Lock lock(_semaphore);
    _maxEigenvalue = std::max(_maxEigenvalue,eigenvalue);
  }
}

double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getAdmissibleTimeStepSize() const {
  return _admissibleTimeStepSize;
}
"""


def create_compute_time_step_size_for_adaptive_time_stepping():
    return """
    double timeStepSize = repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
"""


def create_compute_new_time_step_size_for_adaptive_time_stepping():
    return """
    const double maxEigenvalue = kernels::aderdg::generic::c::maxScaledEigenvalue<
      {{SOLVER_NAME}}
      >(
        repositories::{{SOLVER_INSTANCE}},
        luh,
        marker.x(),
        marker.h(),
        timeStamp,
        timeStepSize
      );

    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue(maxEigenvalue);

    // This is a value set for stats reasons. We'll ignore it later as we
    // ask for a new valid time step size from getAdmissibleTimeStepSize().
    //const double newTimeStepSize = 0.0;
    const double newTimeStepSize =  repositories::{{SOLVER_INSTANCE}}.getMinCellSize(false)>0.0 ?
      repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize() * marker.h()(0) / repositories::{{SOLVER_INSTANCE}}.getMinCellSize(false) :
      0.0;
"""


def create_start_time_step_implementation_for_adaptive_time_stepping():
    predicate = """
    tarch::mpi::Rank::getInstance().isGlobalMaster()
    and
    _maxCellH > 0.0
    and
    isFirstGridSweepOfTimeStep()
  """

    statistics = (
        """
  if ("""
        + predicate
        + """) {
    logInfo("startTimeStep()", "Solver {{SOLVER_NAME}}:" );
    logInfo("startTimeStep()", "t            = " << _minTimeStamp);
    logInfo("startTimeStep()", "dt           = " << getAdmissibleTimeStepSize());
    logInfo("startTimeStep()", "h_{min}      = " << _minCellH);
    logInfo("startTimeStep()", "h_{max}      = " << _maxCellH);
    logInfo("startTimeStep()", "lambda_{max} = " << _maxEigenvalue);
  }
"""
    )

    clear_max_eigenvalue = """if (isFirstGridSweepOfTimeStep()) {
  _maxEigenvalue = 0.0;
}"""

    return statistics + clear_max_eigenvalue


def create_finish_time_step_implementation_for_adaptive_time_stepping():
    """
    This routine is inserted after we have reduced all global quantities. These
    are the quantities with the postfix ThisTimeStep.
    """
    compute_new_timestep = """

    #ifdef Parallel
    double newMaxEigenvalue = _maxEigenvalue;
    tarch::mpi::Rank::getInstance().allReduce(
          &newMaxEigenvalue,
          &_maxEigenvalue,
          1,
          MPI_DOUBLE,
          MPI_MAX,
          [&]() -> void { tarch::services::ServiceRepository::getInstance().receiveDanglingMessages(); }
          );
    #endif

    if (tarch::la::smaller(_maxEigenvalue, 0.0)) {
      ::tarch::triggerNonCriticalAssertion(__FILE__, __LINE__, "_maxEigenvalue >= 0", "invalid max eigenvalue: " + std::to_string(_maxEigenvalue));
      // Keep time step size invariant
      // _admissibleTimeStepSize = _admissibleTimeStepSize;
    } else if (tarch::la::equals(_maxEigenvalue, 0.0)) {
      ::tarch::triggerNonCriticalAssertion(__FILE__, __LINE__, "_maxEigenvalue > 0", "max eigenvalue has reached zero, which means that the simulation cannot successfully continue: " + std::to_string(_maxEigenvalue));
    } else {
      const double minVolumeSize = _minCellHThisTimeStep;
      // The max eigenvalue here is already the sum of the eigenvalues in each direction scaled with the volume size in that direction
      _admissibleTimeStepSize = std::min(MinTerminalTime - _minTimeStampThisTimeStep, CFL * PNPM / _maxEigenvalue);
      if (std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize)) {
        ::tarch::triggerNonCriticalAssertion(__FILE__, __LINE__, "_admissibleTimeStepSize > 0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize));
      }
      if (tarch::la::smallerEquals(_admissibleTimeStepSize,0.0,1e-10) ) {
        logWarning("finishTimeStep(...)", "degenerated time step size of " << std::to_string(_admissibleTimeStepSize) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug (h_volume=" << minVolumeSize << ")");
      }
    }
"""

    return compute_new_timestep



def create_constructor_implementation_for_adaptive_time_stepping():
    return "_admissibleTimeStepSize = 0.0;"
