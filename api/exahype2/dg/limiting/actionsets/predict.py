# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.actionsets.action_set import ActionSet
from exahype2.actionsets.solver_action_set import SolverActionSet
from exahype2.dg.limiting.projection_matrices import create_dmp_sampling_matrices
from exahype2.dg.limiting.projection_matrices import render_matrix_table


class Predict(SolverActionSet):
    """Opens the limiter step of every cell and seeds the extrema its corrector tests against."""

    _TEMPLATE_RESET_VERTEX_EXTREMA = """
{%- if USES_DMP %}
  if ({{SWEEP_GUARD}}) {
    ::exahype2::dg::limiting::resetVertexObservableExtrema<{{NUMBER_OF_DMP_OBSERVABLES}}>(
      fineGridVertex{{SOLVER_NAME}}VertexLabel
    );
  }
{%- endif %}
"""

    _TEMPLATE_TOUCH_HANGING_VERTEX_LAST_TIME = """
{%- if USES_DMP %}
  if ({{SWEEP_GUARD}}) {
    ::exahype2::dg::limiting::restrictHangingVertexObservableExtrema<{{NUMBER_OF_DMP_OBSERVABLES}}>(
      marker.getRelativePositionWithinFatherCell(),
      fineGridVertex{{SOLVER_NAME}}VertexLabel,
      coarseGridVertices{{SOLVER_NAME}}VertexLabel
    );
  }
{%- endif %}
"""

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  if ({{CELL_GUARD}}) {
{%- if USES_DMP %}
    ::exahype2::dg::limiting::predictSubcells<
      {{FULL_QUALIFIED_SOLVER_NAME}},
      {{NUMBER_OF_DMP_OBSERVABLES}}
    >(
      fineGridCell{{SOLVER_NAME}}CellLabel,
      fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data(),
      DgToLobattoMatrix,
      DgToSubcellMatrix,
      fineGridVertices{{SOLVER_NAME}}VertexLabel
    );
{%- else %}
    ::exahype2::dg::limiting::predictSubcells(fineGridCell{{SOLVER_NAME}}CellLabel);
{%- endif %}
  }
"""

    def __init__(self, solver, cell_guard, sweep_guard):
        super().__init__(solver)
        self.cell_guard = cell_guard
        self.sweep_guard = sweep_guard

    def get_static_initialisations(self, full_qualified_classname):
        """Returns the sampling tables of the discrete maximum principle."""
        if not self.solver._subcell_limiter_uses_dmp:
            return super().get_static_initialisations(full_qualified_classname)
        precision = self.solver.precision.storage
        dg_to_lobatto, dg_to_subcells = create_dmp_sampling_matrices(self.solver._basis)
        return (
            "namespace {\n"
            + render_matrix_table("DgToLobattoMatrix", dg_to_lobatto, precision)
            + render_matrix_table("DgToSubcellMatrix", dg_to_subcells, precision)
            + "} // namespace\n"
        )

    def _render(self, template, **entries):
        return self.render(
            template,
            USES_DMP=self.solver._subcell_limiter_uses_dmp,
            NUMBER_OF_DMP_OBSERVABLES=self.solver._subcell_limiter_number_of_dmp_observables,
            **entries,
        )

    def operation_body(self, operation_name):
        if operation_name in (
            ActionSet.OPERATION_TOUCH_VERTEX_FIRST_TIME,
            ActionSet.OPERATION_TOUCH_HANGING_VERTEX_FIRST_TIME,
        ):
            return self._render(
                self._TEMPLATE_RESET_VERTEX_EXTREMA, SWEEP_GUARD=self.sweep_guard
            )
        if operation_name == ActionSet.OPERATION_TOUCH_HANGING_VERTEX_LAST_TIME:
            return self._render(
                self._TEMPLATE_TOUCH_HANGING_VERTEX_LAST_TIME,
                SWEEP_GUARD=self.sweep_guard,
            )
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return self._render(
                self._TEMPLATE_TOUCH_CELL_FIRST_TIME, CELL_GUARD=self.cell_guard
            )
        return ""

    def includes(self):
        return super().includes() + [
            "exahype2/dg/limiting/PredictSubcells.h",
            "exahype2/dg/limiting/VertexExtrema.h",
        ]
