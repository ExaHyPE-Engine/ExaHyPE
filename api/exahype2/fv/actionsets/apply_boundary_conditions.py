# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.actionsets.action_set import ActionSet
from exahype2.actionsets.solver_action_set import SolverActionSet


class ApplyBoundaryConditions(SolverActionSet):
    """Fills the outer halo of every face of a cell that carries the domain boundary.

    The boundary conditions write into the halo of the previous snapshot, which holds the
    solution while this action set runs.
    """

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  if ({{GUARD}} and not marker.hasBeenRefined()) {
    ::exahype2::fv::applyBoundaryConditions<{{HALO_SIZE}}, {{STORAGE_PRECISION}}>(
      marker,
      repositories::{{SOLVER_INSTANCE}},
      {{FACE_METADATA_ACCESSOR}},
      {{FACE_ACCESSOR}}
    );
  }
"""

    def __init__(self, solver, guard):
        super().__init__(solver)
        self.guard = guard
        self._face_accessor = "fineGridFaces" + solver.previous_unknown_identifier()
        self._face_metadata_accessor = "fineGridFaces" + solver._face_label.name

    def includes(self):
        return super().includes() + ["exahype2/fv/ApplyBoundaryConditions.h"]

    def operation_body(self, operation_name):
        if operation_name != ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return ""
        return self.render(
            self._TEMPLATE_TOUCH_CELL_FIRST_TIME,
            GUARD=self.guard,
            FACE_ACCESSOR=self._face_accessor,
            FACE_METADATA_ACCESSOR=self._face_metadata_accessor,
        )
