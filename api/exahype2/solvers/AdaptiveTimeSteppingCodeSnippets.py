# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from enum import Enum
from enum import auto
from string import Template

import jinja2

from .SolverCodeSnippets import SolverCodeSnippets


class _AdaptiveTimeSteppingScheme(Enum):
    FiniteVolume = auto()
    ADERDG = auto()
    RKDG = auto()


class AdaptiveTimeSteppingCodeSnippets(SolverCodeSnippets):
    """Generates adaptive time-stepping code."""

    def __init__(self, scheme, time_step_relaxation=None):
        self._scheme = scheme
        self._time_step_relaxation = time_step_relaxation

    @classmethod
    def for_finite_volume(cls, time_step_relaxation):
        return cls(
            _AdaptiveTimeSteppingScheme.FiniteVolume,
            time_step_relaxation,
        )

    @classmethod
    def for_aderdg(cls):
        return cls(_AdaptiveTimeSteppingScheme.ADERDG)

    @classmethod
    def for_rkdg(cls, time_step_relaxation):
        return cls(
            _AdaptiveTimeSteppingScheme.RKDG,
            time_step_relaxation,
        )

    def _uses_scaled_eigenvalue(self):
        return self._scheme in (
            _AdaptiveTimeSteppingScheme.ADERDG,
            _AdaptiveTimeSteppingScheme.RKDG,
        )

    def create_max_eigenvalue_declaration(self):
        return "  void setMaxEigenvalue(const double eigenvalue);\n"

    def create_max_eigenvalue_definition(self):
        return """
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::setMaxEigenvalue(const double eigenvalue) {
  const auto observedEigenvalueStatistics =
    ::exahype2::NonNegativeStatistics{eigenvalue, eigenvalue};
  if (not observedEigenvalueStatistics.hasPhysicalValues()) {
    tarch::triggerNonCriticalAssertion(
      __FILE__,
      __LINE__,
      "The maximum eigenvalue is finite and non-negative.",
      "Maximum eigenvalue bits="
        + std::to_string(
          observedEigenvalueStatistics.invalidObservationBitsOrIdentity()
        )
    );
  }
  const auto lock = tarch::multicore::Lock(_semaphore);
  const auto accumulatedEigenvalueStatistics =
    ::exahype2::NonNegativeStatistics::accumulate(
      {_maxEigenvalueThisTimeStep, _maxEigenvalueThisTimeStep},
      eigenvalue
    );
  _maxEigenvalueThisTimeStep = accumulatedEigenvalueStatistics.maximum;
}
"""

    def create_max_scaled_eigenvalue_declaration(self):
        if not self._uses_scaled_eigenvalue():
            return ""
        return "  void setMaxScaledEigenvalue(const double eigenvalue);\n"

    def create_max_scaled_eigenvalue_definition(self):
        if not self._uses_scaled_eigenvalue():
            return ""
        return """
void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::setMaxScaledEigenvalue(const double eigenvalue) {
  const auto observedEigenvalueStatistics =
    ::exahype2::NonNegativeStatistics{eigenvalue, eigenvalue};
  if (not observedEigenvalueStatistics.hasPhysicalValues()) {
    tarch::triggerNonCriticalAssertion(
      __FILE__,
      __LINE__,
      "The scaled maximum eigenvalue is finite and non-negative.",
      "Scaled maximum eigenvalue bits="
        + std::to_string(
          observedEigenvalueStatistics.invalidObservationBitsOrIdentity()
        )
    );
  }
  const auto lock = tarch::multicore::Lock(_semaphore);
  const auto accumulatedEigenvalueStatistics =
    ::exahype2::NonNegativeStatistics::accumulate(
      {_maxScaledEigenvalueThisTimeStep, _maxScaledEigenvalueThisTimeStep},
      eigenvalue
    );
  _maxScaledEigenvalueThisTimeStep = accumulatedEigenvalueStatistics.maximum;
}
"""

    def create_abstract_solver_public_declarations(self):
        return (
            self.create_max_eigenvalue_declaration()
            + self.create_max_scaled_eigenvalue_declaration()
            + """
  double getMaxEigenvalue() const;
  virtual double getAdmissibleTimeStepSize() const;
"""
        )

    def create_abstract_solver_protected_declarations(self):
        scaled_eigenvalue_declaration = ""
        if self._uses_scaled_eigenvalue():
            scaled_eigenvalue_declaration = """
  double _maxScaledEigenvalueThisTimeStep;
"""
        return (
            """
  double _maxEigenvalue;
  double _maxEigenvalueThisTimeStep;

  double _admissibleTimeStepSize;
  double _admissibleTimeStepSizeThisTimeStep;
"""
            + scaled_eigenvalue_declaration
        )

    def create_abstract_solver_user_declarations(self):
        """Returns the abstract solver's declarations, closing on ``public:``.

        A scenario appends its own declarations to this string, so whichever
        access label it emits last decides how those additions are reachable.
        Anything a generated action set reads from the solver instance therefore
        has to find the string ending in ``public:``.
        """
        return (
            """
public:
"""
            + self.create_abstract_solver_public_declarations()
            + """
protected:
"""
            + self.create_abstract_solver_protected_declarations()
            + """
public:
"""
        )

    def create_abstract_solver_user_definitions(self):
        return (
            self.create_max_eigenvalue_definition()
            + self.create_max_scaled_eigenvalue_definition()
            + """
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getMaxEigenvalue() const {
  return _maxEigenvalue;
}

/**
 * Returns the admissible time step for the smallest mesh cells. Subcycling
 * scales this value for coarser cells.
 */
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getAdmissibleTimeStepSize() const {
  return _admissibleTimeStepSize;
}
"""
        )

    def create_abstract_solver_constructor_statements(self):
        scaled_eigenvalue_initialisation = ""
        if self._uses_scaled_eigenvalue():
            scaled_eigenvalue_initialisation = """
    _maxScaledEigenvalueThisTimeStep = 0.0;
"""
        return (
            """
    _admissibleTimeStepSize = 0.0;
    _admissibleTimeStepSizeThisTimeStep = 0.0;

    _maxEigenvalue = 0.0;
    _maxEigenvalueThisTimeStep = 0.0;
"""
            + scaled_eigenvalue_initialisation
        )

    def create_compute_time_step_size(self):
        return """
    const auto timeStepSize =
      repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
"""

    def create_compute_new_time_step_size(self):
        if self._scheme is _AdaptiveTimeSteppingScheme.ADERDG:
            return """
    const auto eigenvalueStatistics = kernels::{{SOLVER_NAME}}::reduceMaxEigenvalues(
      repositories::{{SOLVER_INSTANCE}},
      QNew,
      marker.getX(),
      marker.getH(),
      timeStamp,
      timeStepSize
    );

    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue(
      eigenvalueStatistics.maxPhysicalEigenvalue
    );
    repositories::{{SOLVER_INSTANCE}}.setMaxScaledEigenvalue(
      eigenvalueStatistics.maxScaledEigenvalue
    );
"""
        if self._scheme is _AdaptiveTimeSteppingScheme.RKDG:
            return """
    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue(maxPhysicalEigenvalue);
    repositories::{{SOLVER_INSTANCE}}.setMaxScaledEigenvalue(maxScaledEigenvalue);
    const auto newTimeStepSize =
      repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
"""
        return """
    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue(maxEigenvalue);
    const auto newTimeStepSize =
      repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize();
"""

    def create_start_time_step_implementation(self):
        if self._uses_scaled_eigenvalue():
            return """
  if (isFirstGridSweepOfTimeStep()) {
    _admissibleTimeStepSize          = _admissibleTimeStepSizeThisTimeStep;
    _maxEigenvalueThisTimeStep       = 0.0;
    _maxScaledEigenvalueThisTimeStep = 0.0;
  }
"""
        return """
  if (isFirstGridSweepOfTimeStep()) {
    _admissibleTimeStepSize = _admissibleTimeStepSizeThisTimeStep;
    _maxEigenvalueThisTimeStep = 0.0;
  }
"""

    def create_reduced_eigenvalues(self):
        """Names of the eigenvalue members that the end-of-time-step reduction covers.

        A solver that tracks further eigenvalues extends this list rather than adding a
        reduction of its own.
        """
        eigenvalues = ["_maxEigenvalueThisTimeStep"]
        if self._uses_scaled_eigenvalue():
            eigenvalues.append("_maxScaledEigenvalueThisTimeStep")
        return eigenvalues

    def create_max_eigenvalue_reduction(self):
        """Turns the rank-local eigenvalue maxima into global maxima.

        Every reduction synchronises all ranks, so all eigenvalues share one buffer. The
        second buffer carries each observation's validity flag beside its bits, reduced
        with a minimum, so a rank that saw an invalid value decides the outcome instead of
        being outvoted by a larger value elsewhere.
        """
        eigenvalues = self.create_reduced_eigenvalues()
        width = max(len(eigenvalue) for eigenvalue in eigenvalues)

        statistics = "\n".join(
            "    const auto {}Statistics = ::exahype2::NonNegativeStatistics{{{}, {}}};".format(
                eigenvalue.lstrip("_"), eigenvalue, eigenvalue
            )
            for eigenvalue in eigenvalues
        )
        invalid_initialiser = ",\n".join(
            "      {0}Statistics.hasInvalidObservation() ? 0U : 1U,\n"
            "      {0}Statistics.invalidObservationBitsOrIdentity()".format(
                eigenvalue.lstrip("_")
            )
            for eigenvalue in eigenvalues
        )
        assignments = "\n".join(
            "    {0:<{1}} = globalMaxEigenvalues[{2}];\n"
            "    if (globalInvalidObservations[{3}] == 0U) {{\n"
            "      {0} = ::exahype2::NonNegativeStatistics::fromInvalidObservationBits(\n"
            "        globalInvalidObservations[{4}]\n"
            "      ).maximum;\n"
            "    }}".format(eigenvalue, width, index, 2 * index, 2 * index + 1)
            for index, eigenvalue in enumerate(eigenvalues)
        )

        Template = jinja2.Template(
            """
{{STATISTICS}}

    const double   localMaxEigenvalues[]  = {{INITIALISER}};
    double         globalMaxEigenvalues[] = {{ZEROS}};
    const MPI_Comm communicator           = tarch::mpi::Rank::getInstance().getCommunicator();

    const std::uint64_t localInvalidObservations[] = {
{{INVALID_INITIALISER}}
    };
    std::uint64_t globalInvalidObservations[{{INVALID_COUNT}}]{};

    tarch::mpi::allreduce(
      localMaxEigenvalues,
      globalMaxEigenvalues,
      {{COUNT}},
      MPI_DOUBLE,
      MPI_MAX,
      communicator
    );
    tarch::mpi::allreduce(
      localInvalidObservations,
      globalInvalidObservations,
      {{INVALID_COUNT}},
      MPI_UINT64_T,
      MPI_MIN,
      communicator
    );

{{ASSIGNMENTS}}
""",
            keep_trailing_newline=True,
        )

        return Template.render(
            STATISTICS=statistics,
            INITIALISER="{" + ", ".join(eigenvalues) + "}",
            ZEROS="{" + ", ".join(["0.0"] * len(eigenvalues)) + "}",
            COUNT=len(eigenvalues),
            INVALID_INITIALISER=invalid_initialiser,
            INVALID_COUNT=2 * len(eigenvalues),
            ASSIGNMENTS=assignments,
        )

    def _create_scaled_eigenvalue_diagnostic(self):
        if not self._uses_scaled_eigenvalue():
            return ""
        return """
    const auto reducedScaledEigenvalueStatistics =
      ::exahype2::NonNegativeStatistics{
        _maxScaledEigenvalueThisTimeStep,
        _maxScaledEigenvalueThisTimeStep
      };
    const auto hasPositiveReducedScaledEigenvalue =
      reducedScaledEigenvalueStatistics.hasPhysicalValues()
      and _maxScaledEigenvalueThisTimeStep > 0.0;
    if (not hasPositiveReducedScaledEigenvalue) {
      tarch::triggerNonCriticalAssertion(
        __FILE__,
        __LINE__,
        "The reduced scaled maximum eigenvalue is finite and positive.",
        "Scaled maximum eigenvalue bits="
          + std::to_string(
            std::bit_cast<std::uint64_t>(_maxScaledEigenvalueThisTimeStep)
          )
      );
    }
"""

    def _create_time_step_size_from_cfl(self):
        if self._scheme is _AdaptiveTimeSteppingScheme.FiniteVolume:
            template = """
      // One explicit Euler step accumulates the fluxes of all 2 * DIMENSIONS faces.
      // Its stability limit is the volume width divided by DIMENSIONS and the
      // maximum eigenvalue; the relaxation selects a fraction of that limit.
      const auto timeStepSizeFromCFL = $time_step_relaxation
        * _minVolumeHThisTimeStep
        / (DIMENSIONS * _maxEigenvalueThisTimeStep);
"""
            return (
                Template(template)
                .substitute(time_step_relaxation=self._time_step_relaxation)
                .strip("\n")
            )
        if self._scheme is _AdaptiveTimeSteppingScheme.ADERDG:
            return """
      const auto timeStepSizeFromCFL =
        CFL * PNPM / _maxScaledEigenvalueThisTimeStep;
""".strip(
                "\n"
            )
        template = """
      // The scaled eigenvalue includes the cell size and all directional
      // eigenvalues; the relaxation selects a fraction of its stability limit.
      const auto waveSpeedDamping = 1.0 / _maxScaledEigenvalueThisTimeStep;
      constexpr double CockburnShuDamping =
        1.0 / (2.0 * {{RK_ORDER}} - 1.0);
      constexpr double MeshDamping =
        {{DG_ORDER}} == 0 ? 1.0 : 1.0 / ({{DG_ORDER}} * {{DG_ORDER}});
      const auto timeStepSizeFromCFL = $time_step_relaxation
        * waveSpeedDamping * CockburnShuDamping * MeshDamping;
"""
        return (
            Template(template)
            .substitute(time_step_relaxation=self._time_step_relaxation)
            .strip("\n")
        )

    def create_finish_time_step_implementation(self):
        stability_eigenvalue_condition = "hasPositiveReducedEigenvalue"
        if self._uses_scaled_eigenvalue():
            stability_eigenvalue_condition = "hasPositiveReducedScaledEigenvalue"
        template = """
  if (isLastGridSweepOfTimeStep()) {
$max_eigenvalue_reduction

    const auto reducedEigenvalueStatistics =
      ::exahype2::NonNegativeStatistics{
        _maxEigenvalueThisTimeStep,
        _maxEigenvalueThisTimeStep
      };
    const auto hasPositiveReducedEigenvalue =
      reducedEigenvalueStatistics.hasPhysicalValues()
      and _maxEigenvalueThisTimeStep > 0.0;
    if (not hasPositiveReducedEigenvalue) {
      tarch::triggerNonCriticalAssertion(
        __FILE__,
        __LINE__,
        "The reduced maximum eigenvalue is finite and positive.",
        "Maximum eigenvalue bits="
          + std::to_string(
            std::bit_cast<std::uint64_t>(_maxEigenvalueThisTimeStep)
          )
      );
    }
    _maxEigenvalue = _maxEigenvalueThisTimeStep;
$scaled_eigenvalue_diagnostic
    const auto terminalTime =
      ::runner::RuntimeParameters::getInstance().getMinTerminalTime();
    if (
      $stability_eigenvalue_condition
      and _minTimeStampThisTimeStep < terminalTime
    ) {
$time_step_size_from_cfl
      const auto timeStepSizeToTerminalTime =
        terminalTime - _minTimeStampThisTimeStep;
      auto proposedTimeStepStatistics =
        ::exahype2::NonNegativeStatistics::accumulate(
          {},
          timeStepSizeFromCFL
        );
      proposedTimeStepStatistics =
        ::exahype2::NonNegativeStatistics::accumulate(
          proposedTimeStepStatistics,
          timeStepSizeToTerminalTime
        );
      if (not proposedTimeStepStatistics.hasPhysicalValues()) {
        tarch::triggerNonCriticalAssertion(
          __FILE__,
          __LINE__,
          "The proposed time step size is finite and non-negative.",
          "Time step size bits="
            + std::to_string(
              proposedTimeStepStatistics.invalidObservationBitsOrIdentity()
            )
        );
      }
      _admissibleTimeStepSizeThisTimeStep =
        proposedTimeStepStatistics.minimum;

      if (tarch::la::smallerEquals(timeStepSizeFromCFL, 0.0, 1e-10)) {
        tarch::triggerNonCriticalAssertion(
          __FILE__,
          __LINE__,
          "The CFL time step size exceeds the numerical zero tolerance.",
          "CFL time step size=" + std::to_string(timeStepSizeFromCFL)
        );
      }
    }
  }
"""
        return Template(template).substitute(
            max_eigenvalue_reduction=(
                self.create_max_eigenvalue_reduction().strip("\n")
            ),
            scaled_eigenvalue_diagnostic=(
                self._create_scaled_eigenvalue_diagnostic().strip("\n")
            ),
            stability_eigenvalue_condition=(stability_eigenvalue_condition),
            time_step_size_from_cfl=(self._create_time_step_size_from_cfl()),
        )
