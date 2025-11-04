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
  if (tarch::la::greater(getMaxVolumeSize(ofCurrentlyRunningGridSweep), 0.0)) {
    logInfo("printTimeStep()", "Solver {{SOLVER_NAME}}:");
    logDebug("printTimeStep()", "Phase  = " << toString(_solverState));
    logInfo("printTimeStep()", "t       = " << getMinTimeStamp(ofCurrentlyRunningGridSweep) << " / " << MinTerminalTime);
    logInfo("printTimeStep()", "dt      = " << getTimeStepSize());
    logInfo("printTimeStep()", "h_{min} = " << getMinVolumeSize(ofCurrentlyRunningGridSweep) << " (volume size)");
    logInfo("printTimeStep()", "h_{max} = " << getMaxVolumeSize(ofCurrentlyRunningGridSweep) << " (volume size)");
  }
"""

    def create_start_time_step_implementation(self):
        return """
"""

    def create_finish_time_step_implementation(self):
        return (
            """
    if (isLastGridSweepOfTimeStep()) {
      assertion(_minVolumeH >= 0.0);
      assertion(MaxAdmissibleVolumeH > 0.0);
      if (_minVolumeHThisTimeStep <= MaxAdmissibleVolumeH) {
        // TODO: Bug in FV: MaxAdmissibleVolumeH may change without _minVolumeHThisTimeStep changing depending on the patch size.
        // This leads to different time step sizes for the same grid.
        _timeStepSize = """
            + str(self._normalised_time_step_size)
            + """ * _minVolumeHThisTimeStep / MaxAdmissibleVolumeH;
      } else {
        _timeStepSize = 0.0;
      }
    }
"""
        )
