# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import Enum
from enum import auto
from string import Template

from .SolverCodeSnippets import SolverCodeSnippets


class _FixedTimeSteppingScheme(Enum):
    FiniteVolume = auto()
    ADERDG = auto()
    RKDG = auto()


class FixedTimeSteppingCodeSnippets(SolverCodeSnippets):
    """Generates fixed time-stepping code."""

    def __init__(self, scheme, normalised_time_step_size):
        self._scheme = scheme
        self._normalised_time_step_size = normalised_time_step_size

    @classmethod
    def for_finite_volume(cls, normalised_time_step_size):
        return cls(
            _FixedTimeSteppingScheme.FiniteVolume,
            normalised_time_step_size,
        )

    @classmethod
    def for_aderdg(cls, normalised_time_step_size):
        return cls(
            _FixedTimeSteppingScheme.ADERDG,
            normalised_time_step_size,
        )

    @classmethod
    def for_rkdg(cls, normalised_time_step_size):
        return cls(
            _FixedTimeSteppingScheme.RKDG,
            normalised_time_step_size,
        )

    def create_abstract_solver_user_declarations(self):
        return """
private:
  double _timeStepSize;

public:
  double getTimeStepSize() const;
"""

    def create_abstract_solver_user_definitions(self):
        return """
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getTimeStepSize() const {
  return _timeStepSize;
}
"""

    def create_abstract_solver_constructor_statements(self):
        return """
  _timeStepSize = 0.0;
"""

    def create_compute_time_step_size(self):
        return """
    const auto timeStepSize =
      repositories::{{SOLVER_INSTANCE}}.getTimeStepSize();
"""

    def create_compute_new_time_step_size(self):
        if self._scheme is _FixedTimeSteppingScheme.ADERDG:
            return ""
        return """
    const auto newTimeStepSize =
      repositories::{{SOLVER_INSTANCE}}.getTimeStepSize();
"""

    def create_start_time_step_implementation(self):
        return ""

    def _create_time_step_size_computation(self):
        if self._scheme is _FixedTimeSteppingScheme.FiniteVolume:
            template = """
    const auto maxAdmissibleCellSize =
      ::runner::RuntimeParameters::getInstance().getMaxAdmissibleCellSize(
        DomainSize,
        NumberOfFiniteVolumesPerAxisPerPatch
      );
    assertion(
      std::isfinite(_minVolumeHThisTimeStep)
        and _minVolumeHThisTimeStep > 0.0,
      _minVolumeHThisTimeStep
    );
    assertion(
      std::isfinite(maxAdmissibleCellSize) and maxAdmissibleCellSize > 0.0,
      maxAdmissibleCellSize
    );
    _timeStepSize = $normalised_time_step_size
      * _minVolumeHThisTimeStep / maxAdmissibleCellSize;
"""
            return Template(template).substitute(
                normalised_time_step_size=self._normalised_time_step_size
            ).strip("\n")
        if self._scheme is _FixedTimeSteppingScheme.ADERDG:
            template = """
    const auto maxAdmissiblePatchSize =
      ::runner::RuntimeParameters::getInstance().getMaxAdmissiblePatchSize(
        DomainSize
      );
    assertion(
      std::isfinite(_minCellHThisTimeStep) and _minCellHThisTimeStep > 0.0,
      _minCellHThisTimeStep
    );
    assertion(
      std::isfinite(maxAdmissiblePatchSize) and maxAdmissiblePatchSize > 0.0,
      maxAdmissiblePatchSize
    );
    _timeStepSize = $normalised_time_step_size
      * _minCellHThisTimeStep / maxAdmissiblePatchSize;
"""
            return Template(template).substitute(
                normalised_time_step_size=self._normalised_time_step_size
            ).strip("\n")
        template = """
    _timeStepSize = $normalised_time_step_size
      * _minCellHThisTimeStep / {{RK_ORDER}} / {{RK_ORDER}};
"""
        return Template(template).substitute(
            normalised_time_step_size=self._normalised_time_step_size
        ).strip("\n")

    def create_finish_time_step_implementation(self):
        template = """
  if (isLastGridSweepOfTimeStep()) {
$time_step_size_computation
    const auto hasPositiveTimeStepSize =
      std::isfinite(_timeStepSize) and _timeStepSize > 0.0;
    if (not hasPositiveTimeStepSize) {
      tarch::triggerNonCriticalAssertion(
        __FILE__,
        __LINE__,
        "The fixed time step size is finite and positive.",
        "Time step size=" + std::to_string(_timeStepSize)
      );
    }
  }
"""
        return Template(template).substitute(
            time_step_size_computation=(
                self._create_time_step_size_computation()
            )
        )
