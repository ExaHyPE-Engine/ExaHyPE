# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.FixedTimeSteppingCodeSnippets


class FixedTimeSteppingCodeSnippets(exahype2.solvers.FixedTimeSteppingCodeSnippets):
  """
  Code snippet generator for fixed time stepping in the Runge-Kutta schemes
  """
  def __init__(self, normalised_time_step_size):
    self._normalised_time_step_size = normalised_time_step_size

  def create_start_time_step_implementation(self):
    """
    The outcome is used before we actually roll over the accumulation variables
    and other stuff.
    """
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
    logInfo("startTimeStep(...)", "dt           = " << getTimeStepSize());
    logInfo("startTimeStep(...)", "h_{min}      = " << _minVolumeHThisTimeStep << " (volume size)");
    logInfo("startTimeStep(...)", "h_{max}      = " << _maxVolumeHThisTimeStep << " (volume size)");
  }
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
