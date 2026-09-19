# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.actionsets.action_set import ActionSet
from exahype2.actionsets.solver_action_set import SolverActionSet


class RollOverUpdates(SolverActionSet):
    """Copies the current face data of a solver into its previous data and swaps its cell
    patches.

    The copy runs before the next time step overwrites the current data.
    """

    _TEMPLATE_TOUCH_FACE_FIRST_TIME = """
  if ({{FACE_GUARD}}) {
    ::exahype2::fv::rollOverFaceHalo<{{NUMBER_OF_DATA}}, {{NUMBER_OF_VOLUMES_PER_AXIS}}, {{HALO_SIZE}}>(
      marker,
      {{FACE_METADATA_ACCESSOR}},
      {{FACE_NEW_ACCESSOR}}.data(),
      {{FACE_OLD_ACCESSOR}}.data(),
      ::exahype2::RefinementControlService::getInstance().hasRealisedTopologyChangesSinceLastTimeStep()
    );
  }
"""

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  if ({{CELL_GUARD}}) {
    ::exahype2::fv::rollOverCellSolution(
      {{CELL_METADATA_ACCESSOR}},
      {{CELL_OLD_ACCESSOR}},
      {{CELL_NEW_ACCESSOR}}
    );
  }
"""

    def __init__(self, solver, face_guard, cell_guard):
        """The action set appends the TimeStep state to ``cell_guard``, so no cell rolls over
        during grid construction, initialisation or a mesh update."""
        super().__init__(solver)
        self.face_guard = face_guard
        self.cell_guard = cell_guard + " and repositories::{}.isTimeStepSweep()".format(
            solver.instance_name
        )
        self._face_old_accessor = "fineGridFace" + solver.previous_unknown_identifier()
        self._face_new_accessor = "fineGridFace" + solver.current_unknown_identifier()
        self._cell_old_accessor = "fineGridCell" + solver.previous_unknown_identifier()
        self._cell_new_accessor = "fineGridCell" + solver.current_unknown_identifier()
        self._face_metadata_accessor = "fineGridFace" + solver._face_label.name
        self._cell_metadata_accessor = "fineGridCell" + solver._cell_label.name

    def includes(self):
        return super().includes() + [
            "exahype2/RefinementControlService.h",
            "exahype2/fv/RollOverUpdates.h",
        ]

    def operation_body(self, operation_name):
        if operation_name == ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
            return self.render(
                self._TEMPLATE_TOUCH_FACE_FIRST_TIME,
                FACE_GUARD=self.face_guard,
                FACE_METADATA_ACCESSOR=self._face_metadata_accessor,
                FACE_OLD_ACCESSOR=self._face_old_accessor,
                FACE_NEW_ACCESSOR=self._face_new_accessor,
            )
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            return self.render(
                self._TEMPLATE_TOUCH_CELL_FIRST_TIME,
                CELL_GUARD=self.cell_guard,
                CELL_METADATA_ACCESSOR=self._cell_metadata_accessor,
                CELL_OLD_ACCESSOR=self._cell_old_accessor,
                CELL_NEW_ACCESSOR=self._cell_new_accessor,
            )
        return ""
