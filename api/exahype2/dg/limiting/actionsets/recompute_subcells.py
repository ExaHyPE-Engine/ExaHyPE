# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.actionsets.action_set import ActionSet
from exahype2.actionsets.solver_action_set import SolverActionSet
from exahype2.dg.limiting.projection_matrices import create_subcell_projection_matrices
from exahype2.dg.limiting.projection_matrices import render_matrix_table


def _cpp_bool(value):
    return "true" if value else "false"


class RecomputeSubcells(SolverActionSet):
    """Recomputes every limited cell on its finite-volume subgrid."""

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  if ({{CELL_GUARD}}) {
    ::exahype2::dg::limiting::recomputeSubcells<
      {{FULL_QUALIFIED_SOLVER_NAME}},
      {{FV_RECONSTRUCTION}},
      {{FV_FALLBACK_RECONSTRUCTION}},
      {{FV_TIME_STEPPING}},
      {{FV_HALO_WIDTH}},
      {{USES_ADMISSIBILITY_CRITERION}},
      {{EVALUATE_FLUX}},
      {{EVALUATE_NONCONSERVATIVE_PRODUCT}},
      {{EVALUATE_SOURCE}},
      {{EVALUATE_DIFFUSIVE_SOURCE}},
      {{EVALUATE_CUSTOM_RIEMANN}},
      {{EVALUATE_MAX_EIGENVALUE}},
      {{EVALUATE_MAX_EIGENVALUE_OF_FINAL_STATE}}
    >(
      marker,
      repositories::{{SOLVER_INSTANCE}},
      fineGridCell{{SOLVER_NAME}}CellLabel,
      fineGridFaces{{SOLVER_NAME}}FaceLabel,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates,
      fineGridFaces{{SOLVER_NAME}}SubcellOld,
      fineGridCell{{SOLVER_NAME}}SubcellOld,
      fineGridCell{{UNKNOWN_IDENTIFIER}},
      DgToFvMatrix,
      FvToDgMatrix
    );
  }
"""

    def __init__(self, solver, cell_guard):
        super().__init__(solver)
        self.cell_guard = cell_guard

    def get_static_initialisations(self, full_qualified_classname):
        """Returns the projection tables between the Galerkin nodes and the subgrid."""
        precision = self.solver.precision.storage
        dg_to_fv, fv_to_dg = create_subcell_projection_matrices(self.solver._basis)
        return (
            "namespace {\n"
            + render_matrix_table("DgToFvMatrix", dg_to_fv, precision)
            + render_matrix_table("FvToDgMatrix", fv_to_dg, precision)
            + "} // namespace\n"
        )

    def operation_body(self, operation_name):
        if operation_name != ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return ""
        scheme = self.solver._subcell_limiter_solver
        dictionary = self.solver.dictionary
        is_adaptive = self.solver.time_stepping.adaptive
        return self.render(
            self._TEMPLATE_TOUCH_CELL_FIRST_TIME,
            CELL_GUARD=self.cell_guard,
            FV_RECONSTRUCTION=scheme.reconstruction,
            FV_FALLBACK_RECONSTRUCTION=scheme.fallback_reconstruction or "void",
            FV_TIME_STEPPING=scheme.time_stepping,
            FV_HALO_WIDTH=scheme.halo_width,
            USES_ADMISSIBILITY_CRITERION=self.solver.subcell_limiter_admissibility_flag,
            EVALUATE_FLUX=_cpp_bool(dictionary["USE_FLUX"]),
            EVALUATE_NONCONSERVATIVE_PRODUCT=_cpp_bool(dictionary["USE_NCP"]),
            EVALUATE_SOURCE=_cpp_bool(dictionary["USE_SOURCE"]),
            EVALUATE_DIFFUSIVE_SOURCE=_cpp_bool(dictionary["USE_DIFFUSIVE_SOURCE"]),
            EVALUATE_CUSTOM_RIEMANN=_cpp_bool(dictionary["USE_SUBCELL_RIEMANN_SOLVER"]),
            EVALUATE_MAX_EIGENVALUE=_cpp_bool(dictionary["USE_MAX_EIGENVALUE"]),
            EVALUATE_MAX_EIGENVALUE_OF_FINAL_STATE=_cpp_bool(
                dictionary["USE_MAX_EIGENVALUE"] and is_adaptive
            ),
        )

    def includes(self):
        return (
            super().includes()
            + ["exahype2/dg/limiting/RecomputeSubcells.h"]
            + list(self.solver._subcell_limiter_solver.includes)
        )
