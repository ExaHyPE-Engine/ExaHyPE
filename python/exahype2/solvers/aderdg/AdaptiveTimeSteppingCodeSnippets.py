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
    const double newTimeStepSize = repositories::{{SOLVER_INSTANCE}}.getMinCellSize() > 0.0 ?
      repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize() * tarch::la::min(marker.getH()) / repositories::{{SOLVER_INSTANCE}}.getMinCellSize() :
      0.0;
"""

    def create_print_time_step_implementation(self):
        return """
  logInfo("t" << tarch::align << getMinTimeStamp() << " / " << ::exahype2::RuntimeParameters::getInstance().getMinTerminalTime());
  logInfo("dt" << tarch::align << (getMinTimeStepSize() < std::numeric_limits<double>::max() / 10.0 ? getMinTimeStepSize() : _admissibleTimeStepSizeThisTimeStep));
  logInfo("h_{min}" << tarch::align << getMinCellSize() << " (patch size)");
  logInfo("h_{max}" << tarch::align << getMaxCellSize() << " (patch size)");
  logInfo("lambda_{max}" << tarch::align << getMaxEigenvalue());
"""

    def create_start_time_step_implementation(self):
        return """
  if (isFirstGridSweepOfTimeStep()) {
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
        _admissibleTimeStepSize = _admissibleTimeStepSizeThisTimeStep;

        // The max eigenvalue here is already the sum of the eigenvalues in each direction scaled with the volume size in that direction.
        const auto dtCFL = CFL * PNPM / _maxEigenvalueThisTimeStep;

        if (not std::isfinite(dtCFL)) {
          tarch::triggerNonCriticalAssertion(__FILE__, __LINE__,
            "std::isfinite(dtCFL)",
            "Invalid (NaN or inf) time step size: " + std::to_string(dtCFL)
          );
        }

        if (tarch::la::smallerEquals(dtCFL, 0.0, 1e-10)) {
          logWarning("Degenerated time step size of " << std::to_string(dtCFL) << ". Problem might be extremely stiff (and can't be solved) or there could be a bug (h=" << _minCellHThisTimeStep << ")");
        }

        const auto dtToEnd                  = std::max(0.0, ::exahype2::RuntimeParameters::getInstance().getMinTerminalTime() - _minTimeStampThisTimeStep);
        _admissibleTimeStepSizeThisTimeStep = std::min(dtToEnd, dtCFL);
      }
"""
        )
