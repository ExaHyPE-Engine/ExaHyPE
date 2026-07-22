# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4

import peano4.toolbox.blockstructured.ProjectPatchOntoFaces


class ProjectPatchOntoFaces(peano4.toolbox.blockstructured.ProjectPatchOntoFaces):
    """!

    Project patch data onto faces so we can reconstruct the halo layers later on.

    This class is very similar to peano4.toolbox.blockstructured.ProjectPatchOntoFaces,
    but there are some subtle differences.

    The big difference is that the projection also sets the Updated flag and
    the UpdatedTimeStamp. As the projection writes to these two updated
    records, they have to be rolled over afterwards. The mapping RollOverUpdates
    does that.

    The guard keeps the projection to cells that stay leaves. A cell that will be
    refined hands its faces to its children, so projecting its own patch on top
    would overwrite what those children contribute. See
    exahype2.solvers.fv.FV.add_actions_to_init_grid() for the ordering of the
    action sets.
    """

    def __init__(self, solver, guard):
        peano4.toolbox.blockstructured.ProjectPatchOntoFaces.__init__(
            self,
            patch=solver._patch,
            patch_overlap=solver._patch_overlap_new,
            solver_name=solver._name,
            precision=solver._storage_precision,
            guard="not marker.hasBeenRefined() and " + guard,
            add_assertions=True,
        )

        self._face_metadata_accessor = "fineGridFaces" + solver._face_label.name
        self._cell_metadata_accessor = "fineGridCell" "" + solver._cell_label.name
        self._fuse_compute_kernels = solver._fuse_compute_kernels

        self._Template_TouchCellLastTime += """
  if ({GUARD}) {{
    for (int d = 0; d < DIMENSIONS; d++) {{
      {FACE_METADATA_ACCESSOR}(d).setUpdated(1, true);
      {FACE_METADATA_ACCESSOR}(d).setUpdatedTimeStamp(1, {CELL_METADATA_ACCESSOR}.getTimeStamp());
      {FACE_METADATA_ACCESSOR}(d + DIMENSIONS).setUpdated(0, true);
      {FACE_METADATA_ACCESSOR}(d + DIMENSIONS).setUpdatedTimeStamp(0, {CELL_METADATA_ACCESSOR}.getTimeStamp());

      // The projection derives this cell's face halves, so they need no
      // mesh-transfer projection.
      {FACE_METADATA_ACCESSOR}(d).setRequiresDataTransfer(1, false);
      {FACE_METADATA_ACCESSOR}(d + DIMENSIONS).setRequiresDataTransfer(0, false);
    }}
  }}
"""

    def _add_action_set_entries_to_dictionary(self, d):
        super()._add_action_set_entries_to_dictionary(d)
        d["FACE_METADATA_ACCESSOR"] = self._face_metadata_accessor
        d["CELL_METADATA_ACCESSOR"] = self._cell_metadata_accessor

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def generate_kernels(self, namespace, output, dimensions):
        super().generate_kernels(namespace, output, dimensions)
        if self._fuse_compute_kernels:
            self.generate_fused_kernels(namespace, output, dimensions)
