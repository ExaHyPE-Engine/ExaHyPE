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
      const double maxAdmissibleCellSize = ::exahype2::RuntimeParameters::getInstance().getMaxAdmissibleCellSize(
        DomainSize,
        NumberOfFiniteVolumesPerAxisPerPatch
      );
      assertion(_minVolumeH >= 0.0);
      assertion(maxAdmissibleCellSize > 0.0);
      if (_minVolumeHThisTimeStep <= maxAdmissibleCellSize) {
        _timeStepSize = """
            + str(self._normalised_time_step_size)
            + """ * _minVolumeHThisTimeStep / maxAdmissibleCellSize;
      } else {
        _timeStepSize = 0.0;
      }
    }
"""
        )
