# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.FixedTimeSteppingCodeSnippets


class FixedTimeSteppingCodeSnippets(exahype2.solvers.FixedTimeSteppingCodeSnippets):
    def __init__(self, normalised_time_step_size):
        self._normalised_time_step_size = normalised_time_step_size

    def create_print_time_step_implementation(self):
        return """
  logInfo("t" << tarch::align << getMinTimeStamp() << " / " << ::exahype2::RuntimeParameters::getInstance().getMinTerminalTime());
  logInfo("dt" << tarch::align << (getMinTimeStepSize() < std::numeric_limits<double>::max() / 10.0 ? getMinTimeStepSize() : getTimeStepSize()));
  logInfo("h_{min}" << tarch::align << getMinCellSize() << " (patch size)");
  logInfo("h_{max}" << tarch::align << getMaxCellSize() << " (patch size)");
"""

    def create_start_time_step_implementation(self):
        return """
"""

    def create_finish_time_step_implementation(self):
        return (
            """
    if (isLastGridSweepOfTimeStep()) {
      const double maxAdmissiblePatchSize = ::exahype2::RuntimeParameters::getInstance().getMaxAdmissiblePatchSize(DomainSize);
      assertion(_minCellH >= 0.0);
      assertion(maxAdmissiblePatchSize > 0.0);
      if (_minCellHThisTimeStep <= maxAdmissiblePatchSize) {
        _timeStepSize  = """
            + str(self._normalised_time_step_size)
            + """ * _minCellHThisTimeStep  / maxAdmissiblePatchSize;
      } else {
        _timeStepSize = 0.0;
      }
    }
"""
        )
