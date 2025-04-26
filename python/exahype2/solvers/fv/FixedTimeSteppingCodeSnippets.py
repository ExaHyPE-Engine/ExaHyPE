# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.FixedTimeSteppingCodeSnippets


class FixedTimeSteppingCodeSnippets(exahype2.solvers.FixedTimeSteppingCodeSnippets):
  """
  Code snippet generator for fixed time stepping in the Runge-Kutta schemes
  """
  def __init__(self, normalised_time_step_size):
    self._normalised_time_step_size = normalised_time_step_size

  def create_print_time_step_implementation(self):
    return """
  if (tarch::la::greater(getMaxVolumeSize(false), 0.0)) {
    logInfo("printTimeStep()", "Solver {{SOLVER_NAME}}:");
    logInfo("printTimeStep()", "t       = " << getMinTimeStamp(false) << " / " << MinTerminalTime);
    logInfo("printTimeStep()", "dt      = " << getTimeStepSize());
    logInfo("printTimeStep()", "h_{min} = " << getMinVolumeSize(false) << " (volume size)");
    logInfo("printTimeStep()", "h_{max} = " << getMaxVolumeSize(false) << " (volume size)");
  }
"""

  def create_start_time_step_implementation(self):
    return """
"""

  def create_finish_time_step_implementation(self):
    return """
    if (isLastGridSweepOfTimeStep()) {
      assertion(_minVolumeH >= 0.0);
      assertion(MaxAdmissibleVolumeH > 0.0);
      if (_minVolumeHThisTimeStep <= MaxAdmissibleVolumeH) {
        _timeStepSize  = """ + str(self._normalised_time_step_size) + """ * _minVolumeHThisTimeStep / MaxAdmissibleVolumeH;
      } else {
        _timeStepSize = 0.0;
      }
    }
"""
