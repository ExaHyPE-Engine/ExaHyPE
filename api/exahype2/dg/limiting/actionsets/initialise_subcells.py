# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.actionsets.action_set import ActionSet
from exahype2.actionsets.solver_action_set import SolverActionSet
from exahype2.dg.limiting.projection_matrices import create_dmp_sampling_matrices
from exahype2.dg.limiting.projection_matrices import render_matrix_table


class InitialiseSubcells(SolverActionSet):
    """Samples the initial condition on the finite-volume subgrid of every cell."""

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  if ({{GUARD}}) {
{%- if USES_DMP %}
    ::exahype2::dg::limiting::initialiseSubcells<
      {{FULL_QUALIFIED_SOLVER_NAME}},
      {{NUMBER_OF_DMP_OBSERVABLES}},
      {{USES_RELAXATION_TERM}}
    >(
      marker,
      fineGridCell{{SOLVER_NAME}}CellLabel,
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      fineGridCell{{SOLVER_NAME}}SubcellOld.data(),
      DgToLobattoMatrix,
      DgToSubcellMatrix,
      static_cast<{{SOLUTION_STORAGE_PRECISION}}>({{DMP_RELAXATION_PARAMETER}}),
      static_cast<{{SOLUTION_STORAGE_PRECISION}}>({{DMP_DIFFERENCES_SCALING}})
    );
{%- else %}
    ::exahype2::dg::limiting::initialiseSubcells<{{FULL_QUALIFIED_SOLVER_NAME}}>(
      marker,
      fineGridCell{{SOLVER_NAME}}SubcellOld.data()
    );
{%- endif %}
  }
"""

    def __init__(self, solver, guard):
        super().__init__(solver)
        self.guard = guard

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

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return self.render(
                self._TEMPLATE_TOUCH_CELL_FIRST_TIME,
                GUARD=self.guard,
                USES_DMP=self.solver._subcell_limiter_uses_dmp,
                USES_RELAXATION_TERM=self.solver.subcell_limiter_relaxation_term_flag,
                NUMBER_OF_DMP_OBSERVABLES=self.solver._subcell_limiter_number_of_dmp_observables,
                DMP_RELAXATION_PARAMETER=self.solver._subcell_limiter_dmp_relaxation_parameter,
                DMP_DIFFERENCES_SCALING=self.solver._subcell_limiter_dmp_differences_scaling,
            )
        return ""

    def includes(self):
        return super().includes() + ["exahype2/dg/limiting/InitialiseSubcells.h"]
