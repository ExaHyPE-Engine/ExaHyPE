# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.FixedTimeSteppingCodeSnippets


class FixedTimeSteppingCodeSnippets(exahype2.solvers.FixedTimeSteppingCodeSnippets):
    def __init__(self, normalised_time_step_size):
        self._normalised_time_step_size = normalised_time_step_size

    def create_print_time_step_implementation(self):
        return """
  if (tarch::la::greater(getMaxCellSize(false), 0.0)) {
    logInfo("printTimeStep()", "Solver {{SOLVER_NAME}}:");
    logInfo("printTimeStep()", "t       = " << getMinTimeStamp(false) << " / " << MinTerminalTime);
    logInfo("printTimeStep()", "dt      = " << getTimeStepSize());
    logInfo("printTimeStep()", "h_{min} = " << getMinCellSize(false));
    logInfo("printTimeStep()", "h_{max} = " << getMaxCellSize(false));
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
