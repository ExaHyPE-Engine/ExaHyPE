# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.AdaptiveTimeSteppingCodeSnippets


class AdaptiveTimeSteppingCodeSnippets(exahype2.solvers.AdaptiveTimeSteppingCodeSnippets):
  """
  Code snippet generator for fixed time stepping in the Runge-Kutta schemes

  adaptive_starting_timeStep_size: none, hard, soft
  """
  def __init__(self, time_step_relaxation, adaptive_starting_timeStep_size="none"):
    self._time_step_relaxation = time_step_relaxation
    self._adaptive_starting_timeStep_size = adaptive_starting_timeStep_size

  def create_start_time_step_implementation(self):
    return """
  if (
    tarch::mpi::Rank::getInstance().isGlobalMaster()
    and
    (_maxVolumeH > 0.0 or _maxVolumeHThisTimeStep > 0.0)
    and
    isFirstGridSweepOfTimeStep()
  ) {
    logInfo("startTimeStep(...)", "Solver {{SOLVER_NAME}}:");
    logInfo("startTimeStep(...)", "t            = " << _minTimeStampThisTimeStep);
    logInfo("startTimeStep(...)", "dt           = " << getAdmissibleTimeStepSize());
    logInfo("startTimeStep(...)", "h_{min}      = " << _minVolumeHThisTimeStep << " (volume size)");
    logInfo("startTimeStep(...)", "h_{max}      = " << _maxVolumeHThisTimeStep << " (volume size)");
    logInfo("startTimeStep(...)", "lambda_{max} = " << _maxEigenvalue);
  }

  if (isFirstGridSweepOfTimeStep()) {
    _maxEigenvalue = 0.0;
  }
"""

  def create_finish_time_step_implementation(self):
    """
    The superclass takes the admissible cell size and divides it by the
    maximum eigenvalue. The Finite Volume solvers however operate with
    patches, i.e. we have to divide by the volume count per axis.
    """

    temp_implementation=super(AdaptiveTimeSteppingCodeSnippets, self).create_finish_time_step_implementation() + """
    if (
      isLastGridSweepOfTimeStep()
      and
      tarch::la::greater(_maxEigenvalue, 0.0)
    ) {
      _admissibleTimeStepSize = """ + str(self._time_step_relaxation) + """ * _minVolumeHThisTimeStep / _maxEigenvalue;
"""

    if self._adaptive_starting_timeStep_size == "hard" or self._adaptive_starting_timeStep_size == "soft":
      temp_implementation +="""
    if (tarch::la::equals(_minTimeStampThisTimeStep, 0, 1e-8)) {_admissibleTimeStepSize *= 1e-3;}
    if (CheckpointTimeStamp != 0 and tarch::la::equals(_minTimeStampThisTimeStep, CheckpointTimeStamp, 1e-8)) {_admissibleTimeStepSize *= 1e-3;}
"""
      if self._adaptive_starting_timeStep_size == "soft":
        temp_implementation +="""
    if (_previousTimeStepSize != 0.0 and _previousTimeStepSize<_admissibleTimeStepSize) 
      {_admissibleTimeStepSize = 0.2*_admissibleTimeStepSize + 0.8*_previousTimeStepSize;}
    _previousTimeStepSize = _admissibleTimeStepSize;
"""

    temp_implementation +="""
      if (std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize)) {
        ::tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSize > 0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize) );
      }
      if (::tarch::la::smallerEquals(_admissibleTimeStepSize, 0.0, 1e-10)) {
        logWarning("finishTimeStep()", "degenerated time step size of " << std::to_string(_admissibleTimeStepSize) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug in the eigenvalue computation");
      }
    }
"""

    return temp_implementation

