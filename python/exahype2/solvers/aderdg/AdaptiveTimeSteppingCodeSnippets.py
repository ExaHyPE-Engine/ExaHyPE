# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.solvers.AdaptiveTimeSteppingCodeSnippets


class AdaptiveTimeSteppingCodeSnippets(
    exahype2.solvers.AdaptiveTimeSteppingCodeSnippets
):
    def __init__(self):
        pass

    def create_compute_new_time_step_size(self):
        return """
    const double maxEigenvalue = kernels::{{SOLVER_NAME}}::maxScaledEigenvalue(
      repositories::{{SOLVER_INSTANCE}},
      QNew,
      marker.getX(),
      marker.getH(),
      timeStamp,
      timeStepSize
    );

    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue(maxEigenvalue);
    const double newTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getMinCellSize(false) > 0.0 ?
      repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize() * tarch::la::min(marker.getH()) / repositories::{{SOLVER_INSTANCE}}.getMinCellSize(false) :
      0.0;
"""

    def create_print_time_step_implementation(self):
        return """
  if (tarch::la::greater(getMaxCellSize(ofCurrentlyRunningGridSweep), 0.0) and tarch::la::greater(getMaxEigenvalue(ofCurrentlyRunningGridSweep), 0.0)) {
    logInfo("printTimeStep()", "Solver {{SOLVER_NAME}}:");
    logDebug("printTimeStep()", "Phase       = " << toString(_solverState));
    logInfo("printTimeStep()", "t            = " << getMinTimeStamp(ofCurrentlyRunningGridSweep) << " / " << MinTerminalTime);
    logInfo("printTimeStep()", "dt           = " << getAdmissibleTimeStepSize(ofCurrentlyRunningGridSweep));
    logInfo("printTimeStep()", "h_{min}      = " << getMinCellSize(ofCurrentlyRunningGridSweep) << " (patch size)");
    logInfo("printTimeStep()", "h_{max}      = " << getMaxCellSize(ofCurrentlyRunningGridSweep) << " (patch size)");
    logInfo("printTimeStep()", "lambda_{max} = " << getMaxEigenvalue(ofCurrentlyRunningGridSweep));
  }
"""

    def create_start_time_step_implementation(self):
        return """
  if (isFirstGridSweepOfTimeStep()) {
    _maxEigenvalue = _maxEigenvalueThisTimeStep;
    _maxEigenvalueThisTimeStep = 0.0;
  }
"""

    def create_finish_time_step_implementation(self):
        """
        This routine is inserted after we have reduced all global quantities. These
        are the quantities with the postfix ThisTimeStep.
        """
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
        const double minCellSize = _minCellHThisTimeStep;
        _admissibleTimeStepSize = _admissibleTimeStepSizeThisTimeStep;
        // The max eigenvalue here is already the sum of the eigenvalues in each direction scaled with the volume size in that direction.
        _admissibleTimeStepSizeThisTimeStep = std::min(MinTerminalTime - _minTimeStampThisTimeStep, CFL * PNPM / _maxEigenvalueThisTimeStep);
        if (std::isnan(_admissibleTimeStepSizeThisTimeStep) or std::isinf(_admissibleTimeStepSizeThisTimeStep)) {
          tarch::triggerNonCriticalAssertion(__FILE__, __LINE__, "_admissibleTimeStepSizeThisTimeStep > 0", "Invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSizeThisTimeStep));
        }
        if (tarch::la::smallerEquals(_admissibleTimeStepSizeThisTimeStep, 0.0, 1e-10)) {
          logWarning("finishTimeStep()", "Degenerated time step size of " << std::to_string(_admissibleTimeStepSizeThisTimeStep) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug (h=" << minCellSize << ")");
        }
      }
"""
        )
