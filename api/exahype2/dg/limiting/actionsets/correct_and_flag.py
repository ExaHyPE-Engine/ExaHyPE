# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.actionsets.action_set import ActionSet
from exahype2.actionsets.solver_action_set import SolverActionSet
from exahype2.dg.limiting.projection_matrices import create_dmp_sampling_matrices
from exahype2.dg.limiting.projection_matrices import create_subcell_projection_matrices
from exahype2.dg.limiting.projection_matrices import render_matrix_table


class CorrectAndFlag(SolverActionSet):
    """Tests the candidate solution of every cell and marks an inadmissible one as troubled."""

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  if ({{CELL_GUARD}}) {
{%- if USES_DMP %}
    ::exahype2::dg::limiting::correctAndFlag<
      {{FULL_QUALIFIED_SOLVER_NAME}},
      {{USES_ADMISSIBILITY_CRITERION}},
      {{NUMBER_OF_DMP_OBSERVABLES}},
      {{USES_RELAXATION_TERM}}
    >(
      marker,
      repositories::{{SOLVER_INSTANCE}},
      fineGridCell{{SOLVER_NAME}}CellLabel,
      fineGridFaces{{SOLVER_NAME}}FaceLabel,
      fineGridVertices{{SOLVER_NAME}}VertexLabel,
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data(),
      DgToFvMatrix,
      DgToLobattoMatrix,
      DgToSubcellMatrix,
      kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::QuadraturePoints1D,
      static_cast<{{SOLUTION_STORAGE_PRECISION}}>({{DMP_RELAXATION_PARAMETER}}),
      static_cast<{{SOLUTION_STORAGE_PRECISION}}>({{DMP_DIFFERENCES_SCALING}})
    );
{%- else %}
    ::exahype2::dg::limiting::correctAndFlag<
      {{FULL_QUALIFIED_SOLVER_NAME}},
      {{USES_ADMISSIBILITY_CRITERION}}
    >(
      marker,
      repositories::{{SOLVER_INSTANCE}},
      fineGridCell{{SOLVER_NAME}}CellLabel,
      fineGridFaces{{SOLVER_NAME}}FaceLabel,
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      DgToFvMatrix,
      kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::QuadraturePoints1D
    );
{%- endif %}
  }
"""

    _TEMPLATE_TOUCH_FACE_FIRST_TIME = """
  if ({{SWEEP_GUARD}}) {
    ::exahype2::dg::limiting::resetRefinedFaceStatus(fineGridFace{{SOLVER_NAME}}FaceLabel);
  }
"""

    _TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME = """
  if ({{SWEEP_GUARD}}) {
    ::exahype2::dg::limiting::copyCoarseFaceStatusOntoHangingFace(
      marker.getSelectedFaceNumber(),
      fineGridFace{{SOLVER_NAME}}FaceLabel,
      coarseGridFaces{{SOLVER_NAME}}FaceLabel
    );
  }
"""

    _TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME = """
  if ({{SWEEP_GUARD}}) {
    ::exahype2::dg::limiting::raiseHangingFaceStatusOntoCoarseFace(
      marker.getSelectedFaceNumber(),
      fineGridFace{{SOLVER_NAME}}FaceLabel,
      coarseGridFaces{{SOLVER_NAME}}FaceLabel
    );
  }
"""

    _TEMPLATE_TOUCH_HANGING_VERTEX_FIRST_TIME = """
{%- if USES_DMP %}
  if ({{SWEEP_GUARD}}) {
    ::exahype2::dg::limiting::interpolateHangingVertexObservableExtrema<{{NUMBER_OF_DMP_OBSERVABLES}}>(
      marker.getRelativePositionWithinFatherCell(),
      fineGridVertex{{SOLVER_NAME}}VertexLabel,
      coarseGridVertices{{SOLVER_NAME}}VertexLabel
    );
  }
{%- endif %}
"""

    def __init__(self, solver, cell_guard, sweep_guard):
        super().__init__(solver)
        self.cell_guard = cell_guard
        self.sweep_guard = sweep_guard

    def get_static_initialisations(self, full_qualified_classname):
        """Returns the projection table of the subgrid and the sampling tables of the principle."""
        precision = self.solver.precision.storage
        dg_to_fv, _ = create_subcell_projection_matrices(self.solver._basis)
        tables = render_matrix_table("DgToFvMatrix", dg_to_fv, precision)
        if self.solver._subcell_limiter_uses_dmp:
            dg_to_lobatto, dg_to_subcells = create_dmp_sampling_matrices(
                self.solver._basis
            )
            tables += render_matrix_table("DgToLobattoMatrix", dg_to_lobatto, precision)
            tables += render_matrix_table(
                "DgToSubcellMatrix", dg_to_subcells, precision
            )
        return "namespace {\n" + tables + "} // namespace\n"

    def _render(self, template, **entries):
        return self.render(
            template,
            USES_DMP=self.solver._subcell_limiter_uses_dmp,
            USES_ADMISSIBILITY_CRITERION=self.solver.subcell_limiter_admissibility_flag,
            USES_RELAXATION_TERM=self.solver.subcell_limiter_relaxation_term_flag,
            NUMBER_OF_DMP_OBSERVABLES=self.solver._subcell_limiter_number_of_dmp_observables,
            DMP_RELAXATION_PARAMETER=self.solver._subcell_limiter_dmp_relaxation_parameter,
            DMP_DIFFERENCES_SCALING=self.solver._subcell_limiter_dmp_differences_scaling,
            **entries,
        )

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
            return self._render(
                self._TEMPLATE_TOUCH_FACE_FIRST_TIME, SWEEP_GUARD=self.sweep_guard
            )
        if operation_name == ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME:
            return self._render(
                self._TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME,
                SWEEP_GUARD=self.sweep_guard,
            )
        if operation_name == ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME:
            return self._render(
                self._TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME,
                SWEEP_GUARD=self.sweep_guard,
            )
        if operation_name == ActionSet.OPERATION_TOUCH_HANGING_VERTEX_FIRST_TIME:
            return self._render(
                self._TEMPLATE_TOUCH_HANGING_VERTEX_FIRST_TIME,
                SWEEP_GUARD=self.sweep_guard,
            )
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return self._render(
                self._TEMPLATE_TOUCH_CELL_FIRST_TIME, CELL_GUARD=self.cell_guard
            )
        return ""

    def includes(self):
        return super().includes() + [
            "exahype2/dg/limiting/CorrectAndFlag.h",
            "exahype2/dg/limiting/FaceStatus.h",
            "exahype2/dg/limiting/VertexExtrema.h",
        ]
