# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.AdaptiveTimeSteppingCodeSnippets


class AdaptiveTimeSteppingCodeSnippets(
    exahype2.solvers.AdaptiveTimeSteppingCodeSnippets
):
    def __init__(self, time_step_relaxation):
        self._time_step_relaxation = time_step_relaxation

    def create_start_time_step_implementation(self):
        return """
  if (isFirstGridSweepOfTimeStep()) {
    _admissibleTimeStepSize = _admissibleTimeStepSizeThisTimeStep;
    _maxEigenvalueThisTimeStep = 0.0;
  }
"""

    def create_finish_time_step_implementation(self):
        return (
            super(
                AdaptiveTimeSteppingCodeSnippets, self
            ).create_finish_time_step_implementation()
            + """
    if (
      isLastGridSweepOfTimeStep()
      and
      tarch::la::greater(_maxEigenvalueThisTimeStep, 0.0)
    ) {
      _admissibleTimeStepSizeThisTimeStep = """
            + str(self._time_step_relaxation)
            + """ * _minVolumeHThisTimeStep / _maxEigenvalueThisTimeStep;

      if (tarch::la::greater(_admissibleTimeStepSizeThisTimeStep, 0.0)) {
        if (_minTimeStepSizeThisTimeStep >= std::numeric_limits<double>::max() / 10.0) {
          _minTimeStepSizeThisTimeStep = _admissibleTimeStepSizeThisTimeStep;
        }
        if (_maxTimeStepSizeThisTimeStep <= std::numeric_limits<double>::min() * 10.0) {
          _maxTimeStepSizeThisTimeStep = _admissibleTimeStepSizeThisTimeStep;
        }
      }

      if (std::isnan(_admissibleTimeStepSizeThisTimeStep) or std::isinf(_admissibleTimeStepSizeThisTimeStep)) {
        tarch::triggerNonCriticalAssertion( __FILE__, __LINE__, "_admissibleTimeStepSizeThisTimeStep > 0", "Invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSizeThisTimeStep) );
      }
      if (tarch::la::smallerEquals(_admissibleTimeStepSizeThisTimeStep, 0.0, 1e-10)) {
        logWarning("Degenerated time step size of " << std::to_string(_admissibleTimeStepSizeThisTimeStep) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug in the eigenvalue computation");
      }
    }
"""
        )
