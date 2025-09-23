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
  if (tarch::la::greater(getMaxH(ofCurrentlyRunningGridSweep), 0.0)) {
    logInfo("printTimeStep()", "Solver {{SOLVER_NAME}}:");
    logDebug("printTimeStep()", "Phase  = " << toString(_solverState));
    logInfo("printTimeStep()", "t       = " << getMinTimeStamp(ofCurrentlyRunningGridSweep) << " / " << MinTerminalTime);
    logInfo("printTimeStep()", "dt      = " << getTimeStepSize());
    logInfo("printTimeStep()", "h_{min} = " << getMinH(ofCurrentlyRunningGridSweep) << " (internodal distance)");
    logInfo("printTimeStep()", "h_{max} = " << getMaxH(ofCurrentlyRunningGridSweep) << " (internodal distance)");
  }
"""

    def create_start_time_step_implementation(self):
        return """
"""

    def create_finish_time_step_implementation(self):
        return (
            """
    if (isLastGridSweepOfTimeStep()) {
      assertion(_minH >= 0.0);
      assertion(MaxAdmissibleH > 0.0);
      if (_minHThisTimeStep <= MaxAdmissibleH) {
        _timeStepSize = """
            + str(self._normalised_time_step_size)
            + """ * _minHThisTimeStep / MaxAdmissibleH;
      } else {
        _timeStepSize = 0.0;
      }
    }
"""
        )
