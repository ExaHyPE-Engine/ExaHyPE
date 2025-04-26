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

  def create_print_time_step_implementation(self):
    return """
  if (tarch::la::greater(getMaxVolumeSize(false), 0.0) and tarch::la::greater(getMaxEigenvalue(false), 0.0)) {
    logInfo("printTimeStep()", "Solver {{SOLVER_NAME}}:");
    logInfo("printTimeStep()", "t            = " << getMinTimeStamp(false) << " / " << MinTerminalTime);
    logInfo("printTimeStep()", "dt           = " << getAdmissibleTimeStepSize());
    logInfo("printTimeStep()", "h_{min}      = " << getMinVolumeSize(false) << " (volume size)");
    logInfo("printTimeStep()", "h_{max}      = " << getMaxVolumeSize(false) << " (volume size)");
    logInfo("printTimeStep()", "lambda_{max} = " << getMaxEigenvalue(false));
  }
"""

  def create_start_time_step_implementation(self):
    return """
  if (isFirstGridSweepOfTimeStep()) {
    _maxEigenvalue = _maxEigenvalueThisTimeStep;
    _maxEigenvalueThisTimeStep = 0.0;
  }
"""

  def create_finish_time_step_implementation(self):
    """
    The superclass takes the admissible cell size and divides it by the
    maximum eigenvalue. The Finite Volume solvers however operate with
    patches, i.e. we have to divide by the volume count per axis.
    """

    temp_implementation = super(AdaptiveTimeSteppingCodeSnippets, self).create_finish_time_step_implementation() + """
    if (isLastGridSweepOfTimeStep() and tarch::la::greater(_maxEigenvalueThisTimeStep, 0.0)) {
      _admissibleTimeStepSizeThisTimeStep = """ + str(self._time_step_relaxation) + """ * _minVolumeHThisTimeStep / _maxEigenvalueThisTimeStep;
"""

    if self._adaptive_starting_timeStep_size == "hard" or self._adaptive_starting_timeStep_size == "soft":
      temp_implementation +="""
    if (tarch::la::equals(_minTimeStampThisTimeStep, 0, 1e-8)) {
      _admissibleTimeStepSizeThisTimeStep *= 1e-3;
    }
    if (CheckpointTimeStamp != 0 and tarch::la::equals(_minTimeStampThisTimeStep, CheckpointTimeStamp, 1e-8)) {
      _admissibleTimeStepSizeThisTimeStep *= 1e-3;
    }
"""
      if self._adaptive_starting_timeStep_size == "soft":
        temp_implementation +="""
    if (_admissibleTimeStepSize != 0.0 and _admissibleTimeStepSize < _admissibleTimeStepSizeThisTimeStep) {
      _admissibleTimeStepSizeThisTimeStep = 0.2 * _admissibleTimeStepSizeThisTimeStep + 0.8 * _admissibleTimeStepSize;
    }
    _admissibleTimeStepSize = _admissibleTimeStepSizeThisTimeStep;
"""

    temp_implementation +="""
      if (std::isnan(_admissibleTimeStepSizeThisTimeStep) or std::isinf(_admissibleTimeStepSizeThisTimeStep)) {
        tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSizeThisTimeStep > 0", "Invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSizeThisTimeStep) );
      }
      if (tarch::la::smallerEquals(_admissibleTimeStepSizeThisTimeStep, 0.0, 1e-10)) {
        logWarning("finishTimeStep()", "Degenerated time step size of " << std::to_string(_admissibleTimeStepSizeThisTimeStep) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug in the eigenvalue computation");
      }
    }
"""

    return temp_implementation
