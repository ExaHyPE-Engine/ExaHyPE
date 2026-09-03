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
        return "  void setMaxEigenvalue(double eigenvalue);\n"

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
  // Every traversal task may report concurrently, so the extremum updates atomically.
  if (tarch::la::greater(eigenvalue, 0.0)) {
    tarch::accelerator::updateAtomicMax(_maxEigenvalueThisTimeStep, eigenvalue);
  }
}
"""

    def create_max_scaled_eigenvalue_declaration(self):
        if not self._uses_scaled_eigenvalue():
            return ""
        return "  void setMaxScaledEigenvalue(double eigenvalue);\n"

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
  // Every traversal task may report concurrently, so the extremum updates atomically.
  if (tarch::la::greater(eigenvalue, 0.0)) {
    tarch::accelerator::updateAtomicMax(_maxScaledEigenvalueThisTimeStep, eigenvalue);
  }
}
"""

    def create_abstract_solver_public_declarations(self):
        scaled_eigenvalue_getter = ""
        if self._uses_scaled_eigenvalue():
            scaled_eigenvalue_getter = """
  double getMaxScaledEigenvalueThisTimeStep() const;
"""
        return (
            self.create_max_eigenvalue_declaration()
            + self.create_max_scaled_eigenvalue_declaration()
            + """
  double getMaxEigenvalue() const;
  virtual double getAdmissibleTimeStepSize() const;

  double getAdmissibleTimeStepSizeThisTimeStep() const;

  /** Overwrites the admissible time step size; a checkpoint resume reinstates it. */
  void setAdmissibleTimeStepSize(double timeStepSize);

  /** Overwrites the next step's admissible time step size; a checkpoint resume reinstates it. */
  void setAdmissibleTimeStepSizeThisTimeStep(double timeStepSize);

  /** Overwrites the reduced eigenvalue of the last completed step; a checkpoint resume reinstates it. */
  void setReducedMaxEigenvalue(double eigenvalue);
"""
            + scaled_eigenvalue_getter
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
        """Returns the declarations of the abstract solver.

        The string ends in ``public:``, so the declarations a scenario appends stay reachable
        for a generated action set.
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
        scaled_eigenvalue_getter = ""
        if self._uses_scaled_eigenvalue():
            scaled_eigenvalue_getter = """
double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getMaxScaledEigenvalueThisTimeStep() const {
  return _maxScaledEigenvalueThisTimeStep;
}
"""
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

double {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::getAdmissibleTimeStepSizeThisTimeStep() const {
  return _admissibleTimeStepSizeThisTimeStep;
}

void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::setAdmissibleTimeStepSize(const double timeStepSize) {
  _admissibleTimeStepSize = timeStepSize;
}

void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::setAdmissibleTimeStepSizeThisTimeStep(const double timeStepSize) {
  _admissibleTimeStepSizeThisTimeStep = timeStepSize;
}

void {{FULL_QUALIFIED_NAMESPACE}}::{{CLASSNAME}}::setReducedMaxEigenvalue(const double eigenvalue) {
  _maxEigenvalue = eigenvalue;
}
"""
            + scaled_eigenvalue_getter
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
        if self._scheme is _AdaptiveTimeSteppingScheme.ADERDG:
            return """
  // A resume seeds the time step sizes and the eigenvalues of the replayed sweep before
  // the InitialPrediction starts, so that sweep keeps them.
  if (isFirstGridSweepOfTimeStep()
      and not (_solverState == SolverState::InitialPrediction
               and ::runner::RuntimeParameters::getInstance().hasCheckpointLoadPath())) {
    _admissibleTimeStepSize          = _admissibleTimeStepSizeThisTimeStep;
    _maxEigenvalueThisTimeStep       = 0.0;
    _maxScaledEigenvalueThisTimeStep = 0.0;
  }
"""
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
        """Returns the names of the eigenvalue members the reduction of a time step covers."""
        eigenvalues = ["_maxEigenvalueThisTimeStep"]
        if self._uses_scaled_eigenvalue():
            eigenvalues.append("_maxScaledEigenvalueThisTimeStep")
        return eigenvalues

    def create_max_eigenvalue_reduction(self):
        """Turns the rank-local eigenvalue maxima into global maxima.

        One buffer holds every eigenvalue, so the ranks synchronise once. A second buffer
        carries the validity flag of each observation beside its bits and reduces with a
        minimum, so one rank with an invalid value decides the outcome.
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

        reduction_template = jinja2.Template(
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

        return reduction_template.render(
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

    def _finish_time_step_condition(self):
        """Returns the C++ predicate that guards the end-of-step reduction.

        A resumed ADER-DG run replays the last prediction as its InitialPrediction sweep,
        so that sweep also reduces the eigenvalues and derives the next admissible time
        step size, exactly as the replayed sweep did in the writing run.
        """
        if self._scheme is _AdaptiveTimeSteppingScheme.ADERDG:
            return (
                "isLastGridSweepOfTimeStep() or (_solverState == SolverState::InitialPrediction "
                "and ::runner::RuntimeParameters::getInstance().hasCheckpointLoadPath())"
            )
        return "isLastGridSweepOfTimeStep()"

    def create_finish_time_step_implementation(self):
        stability_eigenvalue_condition = "hasPositiveReducedEigenvalue"
        if self._uses_scaled_eigenvalue():
            stability_eigenvalue_condition = "hasPositiveReducedScaledEigenvalue"
        template = """
  if ($finish_time_step_condition) {
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
            finish_time_step_condition=(self._finish_time_step_condition()),
            max_eigenvalue_reduction=(
                self.create_max_eigenvalue_reduction().strip("\n")
            ),
            scaled_eigenvalue_diagnostic=(
                self._create_scaled_eigenvalue_diagnostic().strip("\n")
            ),
            stability_eigenvalue_condition=(stability_eigenvalue_condition),
            time_step_size_from_cfl=(self._create_time_step_size_from_cfl()),
        )

    def create_checkpoint_scalars(self):
        """Returns the C++ expression for the solver scalars a checkpoint stores.

        A fused ADER-DG resume replays the last prediction, which ran with the time step
        size the cell labels carry, so the checkpoint keeps that size beside the
        admissible one and both reduced eigenvalues. A limiting ADER-DG resume and every
        other scheme continue with their regular first sweep and store the two
        admissible sizes.
        """
        if self._scheme is _AdaptiveTimeSteppingScheme.ADERDG:
            return (
                "{% if SUBCELL_LIMITER %}"
                "std::vector<double>{"
                "repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize(), "
                "repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSizeThisTimeStep(), "
                "repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue()}"
                "{% else %}"
                "std::vector<double>{"
                "repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize(), "
                "_capturedTimeStepSize, "
                "repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue(), "
                "repositories::{{SOLVER_INSTANCE}}.getMaxScaledEigenvalueThisTimeStep()}"
                "{% endif %}"
            )
        if self._scheme is _AdaptiveTimeSteppingScheme.RKDG:
            return (
                "std::vector<double>{"
                "repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSize(), "
                "repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSizeThisTimeStep(), "
                "repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue()}"
            )
        return (
            "std::vector<double>{"
            "repositories::{{SOLVER_INSTANCE}}.getAdmissibleTimeStepSizeThisTimeStep(), "
            "repositories::{{SOLVER_INSTANCE}}.getMaxEigenvalue()}"
        )

    def create_checkpoint_scalar_restore(self):
        """Returns the C++ statements that reinstate the scalars a checkpoint stored."""
        if self._scheme is _AdaptiveTimeSteppingScheme.ADERDG:
            return """
{% if SUBCELL_LIMITER %}
    const std::vector<double>& checkpointScalars = _checkpointLoader->getSolverScalars();
    repositories::{{SOLVER_INSTANCE}}.setAdmissibleTimeStepSize(checkpointScalars.at(0));
    repositories::{{SOLVER_INSTANCE}}.setAdmissibleTimeStepSizeThisTimeStep(checkpointScalars.at(1));
    repositories::{{SOLVER_INSTANCE}}.setReducedMaxEigenvalue(checkpointScalars.at(2));
{% else %}
    const std::vector<double>& checkpointScalars = _checkpointLoader->getSolverScalars();
    repositories::{{SOLVER_INSTANCE}}.setAdmissibleTimeStepSizeThisTimeStep(checkpointScalars.at(0));
    repositories::{{SOLVER_INSTANCE}}.setAdmissibleTimeStepSize(checkpointScalars.at(1));
    repositories::{{SOLVER_INSTANCE}}.setMaxEigenvalue(checkpointScalars.at(2));
    repositories::{{SOLVER_INSTANCE}}.setMaxScaledEigenvalue(checkpointScalars.at(3));
{% endif %}
"""
        if self._scheme is _AdaptiveTimeSteppingScheme.RKDG:
            return """
    const std::vector<double>& checkpointScalars = _checkpointLoader->getSolverScalars();
    repositories::{{SOLVER_INSTANCE}}.setAdmissibleTimeStepSize(checkpointScalars.at(0));
    repositories::{{SOLVER_INSTANCE}}.setAdmissibleTimeStepSizeThisTimeStep(checkpointScalars.at(1));
    repositories::{{SOLVER_INSTANCE}}.setReducedMaxEigenvalue(checkpointScalars.at(2));
"""
        return """
    const std::vector<double>& checkpointScalars = _checkpointLoader->getSolverScalars();
    repositories::{{SOLVER_INSTANCE}}.setAdmissibleTimeStepSize(checkpointScalars.at(0));
    repositories::{{SOLVER_INSTANCE}}.setAdmissibleTimeStepSizeThisTimeStep(checkpointScalars.at(0));
    repositories::{{SOLVER_INSTANCE}}.setReducedMaxEigenvalue(checkpointScalars.at(1));
"""
