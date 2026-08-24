# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4

import peano4.blockstructured.ProjectPatchOntoFaces


class ProjectPatchOntoFaces(peano4.blockstructured.ProjectPatchOntoFaces):
    """Projects patch data into the configured face halo and updates its metadata.

    The updated flag and timestamp tell ``RollOverUpdates`` which face halves
    have a new snapshot.

    The guard keeps the projection to cells that stay leaves. A cell that will be
    refined hands its faces to its children, so projecting its own patch on top
    would overwrite what those children contribute. See
    exahype2.solvers.fv.FV.add_actions_to_init_grid() for the ordering of the
    action sets.
    """

    def __init__(self, solver, guard):
        super().__init__(
            patch=solver._patch,
            patch_overlap=solver._patch_overlap_new,
            solver_name=solver._name,
            precision=solver._storage_precision,
            guard="not marker.hasBeenRefined() and " + guard,
            add_assertions=True,
        )

        self._face_metadata_accessor = "fineGridFaces" + solver._face_label.name
        self._cell_metadata_accessor = "fineGridCell" + solver._cell_label.name

        self._Template_TouchCellLastTime += """
  if ({GUARD}) {{
    const auto updatedTimeStamp = {CELL_METADATA_ACCESSOR}.getTimeStamp();
    for (int dimension = 0; dimension < DIMENSIONS; dimension++) {{
      {FACE_METADATA_ACCESSOR}(dimension).setUpdated(1, true);
      {FACE_METADATA_ACCESSOR}(dimension).setUpdatedTimeStamp(1, updatedTimeStamp);
      {FACE_METADATA_ACCESSOR}(dimension + DIMENSIONS).setUpdated(0, true);
      {FACE_METADATA_ACCESSOR}(dimension + DIMENSIONS).setUpdatedTimeStamp(0, updatedTimeStamp);

      // The projection derives this cell's face halves, so they need no
      // mesh-transfer projection.
      {FACE_METADATA_ACCESSOR}(dimension).setRequiresDataTransfer(1, false);
      {FACE_METADATA_ACCESSOR}(dimension + DIMENSIONS).setRequiresDataTransfer(0, false);
    }}
  }}
"""

    def _add_action_set_entries_to_dictionary(self, dictionary):
        super()._add_action_set_entries_to_dictionary(dictionary)
        dictionary["FACE_METADATA_ACCESSOR"] = self._face_metadata_accessor
        dictionary["CELL_METADATA_ACCESSOR"] = self._cell_metadata_accessor

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
