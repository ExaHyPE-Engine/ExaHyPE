# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.actionsets.action_set import ActionSet
from exahype2.actionsets.solver_action_set import SolverActionSet
from exahype2.dg.limiting.projection_matrices import create_subcell_projection_matrices
from exahype2.dg.limiting.projection_matrices import render_matrix_table


class SpreadDomain(SolverActionSet):
    """Grows the finite-volume domain by one ring around every troubled cell."""

    _TEMPLATE_TOUCH_FACE_FIRST_TIME = """
  if ({{FACE_GUARD}}) {
    ::exahype2::dg::limiting::snapshotFaceStatus(fineGridFace{{SOLVER_NAME}}FaceLabel);
  }
"""

    _TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME = """
  if ({{FACE_GUARD}}) {
    ::exahype2::dg::limiting::copyCoarseFaceStatusOntoHangingFace(
      marker.getSelectedFaceNumber(),
      fineGridFace{{SOLVER_NAME}}FaceLabel,
      coarseGridFaces{{SOLVER_NAME}}FaceLabel
    );
  }
"""

    _TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME = """
  if ({{FACE_GUARD}}) {
    ::exahype2::dg::limiting::raiseHangingFaceStatusOntoCoarseFace(
      marker.getSelectedFaceNumber(),
      fineGridFace{{SOLVER_NAME}}FaceLabel,
      coarseGridFaces{{SOLVER_NAME}}FaceLabel
    );
  }
"""

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  if ({{CELL_GUARD}}) {
    ::exahype2::dg::limiting::spreadDomain<{{FULL_QUALIFIED_SOLVER_NAME}}, {{HALO_WIDTH}}>(
      marker,
      repositories::{{SOLVER_INSTANCE}},
      fineGridCell{{SOLVER_NAME}}CellLabel,
      fineGridFaces{{SOLVER_NAME}}FaceLabel,
      fineGridFaces{{SOLVER_NAME}}SubcellOld,
      fineGridCell{{UNKNOWN_IDENTIFIER}}_old,
      fineGridCell{{SOLVER_NAME}}SubcellOld,
      DgToFvMatrix
    );
  }
"""

    def __init__(self, solver, cell_guard, face_guard):
        super().__init__(solver)
        self.cell_guard = cell_guard
        self.face_guard = face_guard

    def get_static_initialisations(self, full_qualified_classname):
        """Returns the projection table of the accepted polynomial onto the subgrid."""
        dg_to_fv, _ = create_subcell_projection_matrices(self.solver._basis)
        return (
            "namespace {\n"
            + render_matrix_table(
                "DgToFvMatrix", dg_to_fv, self.solver.precision.storage
            )
            + "} // namespace\n"
        )

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
            return self.render(
                self._TEMPLATE_TOUCH_FACE_FIRST_TIME, FACE_GUARD=self.face_guard
            )
        if operation_name == ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME:
            return self.render(
                self._TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME, FACE_GUARD=self.face_guard
            )
        if operation_name == ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME:
            return self.render(
                self._TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME, FACE_GUARD=self.face_guard
            )
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return self.render(
                self._TEMPLATE_TOUCH_CELL_FIRST_TIME,
                CELL_GUARD=self.cell_guard,
                HALO_WIDTH=self.solver._subcell_limiter_solver.halo_width,
            )
        return ""

    def includes(self):
        return super().includes() + [
            "exahype2/dg/limiting/FaceStatus.h",
            "exahype2/dg/limiting/SpreadDomain.h",
        ]
