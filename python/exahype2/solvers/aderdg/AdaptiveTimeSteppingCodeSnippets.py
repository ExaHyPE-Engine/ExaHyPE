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
      luh,
      marker.x(),
      marker.h(),
      timeStamp,
      timeStepSize
    );

    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue(maxEigenvalue);

    // This is a value set for stats reasons. We'll ignore it later as we
    // ask for a new valid time step size from getAdmissibleTimeStepSize().
    //const double newTimeStepSize = 0.0;
    const double newTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getMinCellSize(false) > 0.0 ?
      repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize() * marker.h()(0) / repositories::{{SOLVER_INSTANCE}}.getMinCellSize(false) :
      0.0;
"""

    def create_start_time_step_implementation(self):
        return """
      if (
        tarch::mpi::Rank::getInstance().isGlobalMaster()
        and
        _maxCellH > 0.0
        and
        isFirstGridSweepOfTimeStep()
      ) {
        logInfo("startTimeStep()", "Solver {{SOLVER_NAME}}:");
        logInfo("startTimeStep()", "t            = " << _minTimeStamp);
        logInfo("startTimeStep()", "dt           = " << getAdmissibleTimeStepSize());
        logInfo("startTimeStep()", "h_{min}      = " << _minCellH);
        logInfo("startTimeStep()", "h_{max}      = " << _maxCellH);
        logInfo("startTimeStep()", "lambda_{max} = " << _maxEigenvalue);
      }

      if (isFirstGridSweepOfTimeStep()) {
        _maxEigenvalue = 0.0;
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
        tarch::la::greater(_maxEigenvalue, 0.0)
      ) {
        const double minCellSize = _minCellHThisTimeStep;
        // The max eigenvalue here is already the sum of the eigenvalues in each direction scaled with the volume size in that direction.
        _admissibleTimeStepSize = std::min(MinTerminalTime - _minTimeStampThisTimeStep, CFL * PNPM / _maxEigenvalue);
        if (std::isnan(_admissibleTimeStepSize) or std::isinf(_admissibleTimeStepSize)) {
          ::tarch::triggerNonCriticalAssertion(__FILE__, __LINE__, "_admissibleTimeStepSize > 0", "invalid (NaN of inf) time step size: " + std::to_string(_admissibleTimeStepSize));
        }
        if (tarch::la::smallerEquals(_admissibleTimeStepSize, 0.0, 1e-10)) {
          logWarning("finishTimeStep(...)", "degenerated time step size of " << std::to_string(_admissibleTimeStepSize) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug (h=" << minCellSize << ")");
        }
      }
"""
        )
