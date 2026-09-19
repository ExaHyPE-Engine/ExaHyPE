# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from exahype2.actionsets.action_set import ActionSet
from exahype2.actionsets.solver_action_set import SolverActionSet


class ProjectPatchOntoFaces(SolverActionSet):
    """Copies the boundary volumes of a patch into the face halves the cell owns.

    A cell that will be refined hands its faces to its children, so projecting its own patch
    on top would overwrite what those children contribute.
    """

    _TEMPLATE_TOUCH_CELL_LAST_TIME = """
  if ({{GUARD}}) {
    ::exahype2::fv::projectPatchOntoFaces<
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{HALO_SIZE}},
      {{NUMBER_OF_DATA}},
      {{STORAGE_PRECISION}}
    >(
      {{FACE_METADATA_ACCESSOR}},
      {{CELL_ACCESSOR}}.data(),
      {{FACE_ACCESSOR}},
      {{CELL_METADATA_ACCESSOR}}.getTimeStamp()
    );
  }
"""

    def __init__(self, solver, guard):
        super().__init__(solver)
        assert solver._patch_overlap_new.dim[0] % 2 == 0
        assert solver._patch.dim[0] == solver._patch.dim[1]
        assert solver._patch_overlap_new.dim[1] == solver._patch.dim[0]

        self.guard = "not marker.hasBeenRefined() and " + guard
        self._cell_accessor = "fineGridCell" + solver.current_unknown_identifier()
        self._face_accessor = "fineGridFaces" + solver.current_unknown_identifier()
        self._face_metadata_accessor = "fineGridFaces" + solver._face_label.name
        self._cell_metadata_accessor = "fineGridCell" + solver._cell_label.name

    def includes(self):
        return super().includes() + ["exahype2/fv/ProjectPatchOntoFaces.h"]

    def operation_body(self, operation_name):
        if operation_name != ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            return ""
        return self.render(
            self._TEMPLATE_TOUCH_CELL_LAST_TIME,
            GUARD=self.guard,
            CELL_ACCESSOR=self._cell_accessor,
            FACE_ACCESSOR=self._face_accessor,
            FACE_METADATA_ACCESSOR=self._face_metadata_accessor,
            CELL_METADATA_ACCESSOR=self._cell_metadata_accessor,
        )
