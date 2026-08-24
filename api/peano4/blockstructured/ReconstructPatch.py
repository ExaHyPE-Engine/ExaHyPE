# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


class ReconstructPatch(ActionSet):
    """Reconstructs a cell patch and its halo in ``touchCellFirstTime``.

    The N-wide cell patch and 2M-wide face overlaps produce a reconstructed
    patch of width N+2M in every dimension. ``ProjectPatchOntoFaces`` describes
    the face-overlap layout.
    """

    def __init__(
        self,
        patch,
        patch_overlap,
        patch_reconstructed,
        solver_name,
        precision,
        guard="true",
        add_assertions_to_halo_exchange=True,
    ):
        super().__init__(descend_invocation_order=1)

        assert patch_overlap.dim[0] % 2 == 0
        assert patch.dim[0] == patch.dim[1]
        assert patch_overlap.dim[1] == patch.dim[0]

        self._guard = guard
        self._precision = precision
        self._number_of_unknowns = int(patch.no_of_unknowns)
        self._number_of_dofs_per_axis = int(patch.dim[0])
        self._total_overlap = int(patch_overlap.dim[0])
        self._overlap_name = patch_overlap.name
        self._patch_name = patch.name
        self._reconstructed_patch_name = patch_reconstructed.name

    def _add_action_set_entries_to_dictionary(self, dictionary):
        assert isinstance(self._number_of_unknowns, int)
        assert isinstance(self._number_of_dofs_per_axis, int)
        assert isinstance(self._total_overlap, int)

        dictionary["GUARD"] = self._guard
        dictionary["PRECISION"] = self._precision
        dictionary["UNKNOWNS"] = self._number_of_unknowns
        dictionary["DOFS_PER_AXIS"] = self._number_of_dofs_per_axis
        dictionary["OVERLAP"] = self._total_overlap // 2
        dictionary["CELL_ACCESSOR"] = "fineGridCell" + self._patch_name
        dictionary["FACE_ACCESSOR"] = "fineGridFaces" + self._overlap_name
        dictionary["RECONSTRUCTED_ACCESSOR"] = (
            "fineGridCell" + self._reconstructed_patch_name
        )

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        result = super().get_includes()
        result += """
#include "toolbox/blockstructured/ReconstructPatch.h"
"""
        return result

    def user_should_modify_template(self):
        return False

    _Template_TouchCellFirstTime = r"""
  if ({GUARD}) {{
    const auto* const inputPatch = {CELL_ACCESSOR}.data();
    auto* const reconstructedPatch = {RECONSTRUCTED_ACCESSOR}.data();

    {PRECISION}* faces[TWO_TIMES_D];
    for (int faceNumber = 0; faceNumber < TWO_TIMES_D; ++faceNumber) {{
      faces[faceNumber] = {FACE_ACCESSOR}(faceNumber).data();
    }}

    ::toolbox::blockstructured::reconstructPatch<
      {DOFS_PER_AXIS},
      {OVERLAP},
      {UNKNOWNS},
      {PRECISION}
    >(inputPatch, faces, reconstructedPatch);
  }}
"""

    def get_body_of_operation(self, operation_name):
        result = ""
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            dictionary = {}
            self._add_action_set_entries_to_dictionary(dictionary)
            result = self._Template_TouchCellFirstTime.format(**dictionary)
        return result

    def generate_kernels(self, namespace, output, dimensions):
        """Generates nothing because the action calls the library template."""
        return None
