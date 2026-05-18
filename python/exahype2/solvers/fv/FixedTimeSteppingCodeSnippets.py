# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.FixedTimeSteppingCodeSnippets


class FixedTimeSteppingCodeSnippets(exahype2.solvers.FixedTimeSteppingCodeSnippets):
    def __init__(self, normalised_time_step_size):
        self._normalised_time_step_size = normalised_time_step_size

    def create_print_time_step_implementation(self):
        return """
  logInfo("t" << tarch::align << getMinTimeStamp() << " / " << MinTerminalTime);
  logInfo("dt" << tarch::align << (getMinTimeStepSize() < std::numeric_limits<double>::max() / 10.0 ? getMinTimeStepSize() : getTimeStepSize()));
  logInfo("h_{min}" << tarch::align << getMinVolumeSize() << " (volume size)");
  logInfo("h_{max}" << tarch::align << getMaxVolumeSize() << " (volume size)");
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
