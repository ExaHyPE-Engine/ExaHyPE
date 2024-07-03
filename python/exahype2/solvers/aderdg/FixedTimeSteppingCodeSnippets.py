# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.FixedTimeSteppingCodeSnippets


class FixedTimeSteppingCodeSnippets(exahype2.solvers.FixedTimeSteppingCodeSnippets):
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
      _maxCellH > 0.0
      and
      isFirstGridSweepOfTimeStep()
    ) {
      logInfo("startTimeStep(...)", "Solver {{SOLVER_NAME}}:" );
      logInfo("startTimeStep(...)", "t       = " << _minTimeStampThisTimeStep);
      logInfo("startTimeStep(...)", "dt      = " << getTimeStepSize());
      logInfo("startTimeStep(...)", "h_{min} = " << _minCellH);
      logInfo("startTimeStep(...)", "h_{max} = " << _maxCellH);
  }
"""

    def create_finish_time_step_implementation(self):
        return (
            """
    if (isLastGridSweepOfTimeStep()) {
      assertion(_minVolumeH >= 0.0);
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
