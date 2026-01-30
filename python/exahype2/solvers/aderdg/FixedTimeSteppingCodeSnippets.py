# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.FixedTimeSteppingCodeSnippets


class FixedTimeSteppingCodeSnippets(exahype2.solvers.FixedTimeSteppingCodeSnippets):
    def __init__(self, normalised_time_step_size):
        self._normalised_time_step_size = normalised_time_step_size

    def create_print_time_step_implementation(self):
        return """
  if (tarch::la::greater(getMaxCellSize(ofCurrentlyRunningGridSweep), 0.0)) {
    logDebug("logTimeStep()", "Phase  = " << toString(_solverState));
    logInfo("logTimeStep()", "t       = " << getMinTimeStamp(ofCurrentlyRunningGridSweep) << " / " << MinTerminalTime);
    logInfo("logTimeStep()", "dt      = " << getTimeStepSize());
    logInfo("logTimeStep()", "h_{min} = " << getMinCellSize(ofCurrentlyRunningGridSweep) << " (patch size)");
    logInfo("logTimeStep()", "h_{max} = " << getMaxCellSize(ofCurrentlyRunningGridSweep) << " (patch size)");
  }
"""

    def create_start_time_step_implementation(self):
        return """
"""

    def create_finish_time_step_implementation(self):
        return (
            """
    if (isLastGridSweepOfTimeStep()) {
      assertion(_minCellH >= 0.0);
      assertion(MaxAdmissibleCellH > 0.0);
      if (_minCellH <= MaxAdmissibleCellH) {
        _timeStepSize  = """
            + str(self._normalised_time_step_size)
            + """ * _minCellH  / MaxAdmissibleCellH;
      } else {
        _timeStepSize = 0.0;
      }
    }
"""
        )
