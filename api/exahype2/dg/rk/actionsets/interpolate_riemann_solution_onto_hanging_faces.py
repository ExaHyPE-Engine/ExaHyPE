# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.actionsets

from exahype2.dg.rk.butcher_tableau import ButcherTableau


class InterpolateRiemannSolutionOntoHangingFaces(exahype2.actionsets.SolverActionSet):
    """
    Solves the Riemann problems at hanging faces.

    Hanging faces store no data between grid sweeps.
    """

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
{% for predicate_number in range(0, PREDICATES|length) %}
  if ({{PREDICATES[predicate_number]}}) {
    ::exahype2::dg::rk::interpolateRiemannSolution<
      {{FULL_QUALIFIED_SOLVER_NAME}},
      {{ "true" if IS_LINEAR else "false" }},
      {{ "true" if USE_RIEMANN_SOLVER else "false" }},
      {{ "true" if USE_FLUX else "false" }},
      {{ "true" if USE_DIFFUSIVE_FLUX else "false" }},
      {{ "true" if USE_NCP else "false" }},
      kernels::{{SOLVER_NAME}}::Quadrature<double>,
      kernels::{{SOLVER_NAME}}::DGMatrices<double>,
      kernels::{{SOLVER_NAME}}::ButcherTableau<double>
    >(
      marker,
      fineGridCell{{SOLVER_NAME}}CellLabel,
      fineGridFaces{{SOLVER_NAME}}FaceLabel,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection,
      coarseGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution,
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data(),
      {{STAGES_IN_FLIGHT[predicate_number]}},
      {{RELATIVE_STAGE_TIME_STEP_SIZES[predicate_number]}}
    );
  }
{% endfor %}
"""

    def __init__(self, solver):
        super().__init__(solver)
        self.guards = []
        self._butcher_tableau = ButcherTableau(self._solver.rk_order)

    @property
    def guards(self):
        if self._guards == []:
            raise ValueError("The guards are not initialised.")
        return self._guards

    @guards.setter
    def guards(self, new_guards):
        if new_guards != [] and len(new_guards) != self._solver.number_of_stages:
            raise ValueError(
                "One guard per Runge-Kutta step is needed. There are {} steps and the guards are {}.".format(
                    self._solver.number_of_stages, new_guards
                )
            )
        self._guards = new_guards

    def _relative_stage_time_step_sizes(self):
        """Returns the fraction of the time step each stage starts at, rotated by one stage
        because a mesh-refinement sweep runs ahead of the stage it prepares."""
        sizes = self._butcher_tableau.time_step_sizes()
        return sizes[-1:] + sizes[:-1]

    def _stages_in_flight(self):
        """Returns the stage whose linear combination the cell carries in the mesh-refinement
        sweep of each stage, which is the stage before it."""
        stages = list(range(self._solver.number_of_stages))
        return stages[-1:] + stages[:-1]

    def operation_body(self, operation_name):
        if (
            operation_name
            == exahype2.actionsets.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            return self.render(
                self._TEMPLATE_TOUCH_CELL_FIRST_TIME,
                PREDICATES=self.guards,
                STAGES_IN_FLIGHT=self._stages_in_flight(),
                RELATIVE_STAGE_TIME_STEP_SIZES=self._relative_stage_time_step_sizes(),
            )
        return ""

    def includes(self):
        return super().includes() + ["exahype2/dg/rk/InterpolateRiemannSolution.h"]
