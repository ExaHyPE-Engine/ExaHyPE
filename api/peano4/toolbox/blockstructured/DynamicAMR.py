# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import jinja2


class DynamicAMR(ActionSet):
    """
    This class assumes that you have an 2MxNxN patch on your faces.

    patch: peano4.datamodel.Patch

    patch_overlap_interpolation:  peano4.datamodel.Patch
      Consult remark above about how the dimensions of this overlap
      patch have to match. This overlap is used for the
      interpolation. It can be the same as for the restriction.

    patch_overlap_restriction: peano4.datamodel.Patch
    """

    def __init__(
        self,
        patch,
        patch_overlap_interpolation,
        patch_overlap_restriction,
        clear_guard="true",
        restrict_guard="true",
        interpolate_guard="true",
        additional_includes="",
    ):
        """
        Restrict_guard: String.

          Predicate as C++ expression. It determines which faces are restricted.

        The transfer operators are hard-wired to piecewise-constant
        interpolation and averaging restriction (see toolbox AMRRoutines).
        """
        super(DynamicAMR, self).__init__(descend_invocation_order=1)
        self.d = {}
        if patch_overlap_interpolation.dim[0] % 2 != 0:
            raise Exception(
                "Error: Patch associated to face has to have even number of cells. Otherwise, it is not a symmetric overlap."
            )
        if patch_overlap_interpolation.dim != patch_overlap_restriction.dim:
            raise Exception(
                "Error: Patches associated to face have to have same dimensions for restriction and interpolation."
            )

        overlap = int(patch_overlap_interpolation.dim[0] / 2)

        self.d["UNKNOWNS"] = str(patch_overlap_interpolation.no_of_unknowns)
        self.d["DOFS_PER_AXIS"] = str(patch_overlap_interpolation.dim[1])
        self.d["OVERLAP"] = str(overlap)
        self.d["FINE_GRID_CELL"] = "fineGridCell" + patch.name
        self.d["COARSE_GRID_CELL"] = "coarseGridCell" + patch.name
        self.d["FINE_GRID_FACE_ACCESSOR_INTERPOLATION"] = (
            "fineGridFace" + patch_overlap_interpolation.name
        )
        self.d["COARSE_GRID_FACE_ACCESSOR_INTERPOLATION"] = (
            "coarseGridFaces" + patch_overlap_interpolation.name
        )
        self.d["FINE_GRID_FACE_ACCESSOR_RESTRICTION"] = (
            "fineGridFace" + patch_overlap_restriction.name
        )
        self.d["COARSE_GRID_FACE_ACCESSOR_RESTRICTION"] = (
            "coarseGridFaces" + patch_overlap_restriction.name
        )
        self.d["CLEAR_GUARD"] = clear_guard
        self.d["RESTRICT_GUARD"] = restrict_guard
        self.d["INTERPOLATE_GUARD"] = interpolate_guard

        self.additional_includes = additional_includes

    def get_constructor_body(self):
        return ""

    def get_destructor_body(self):
        return ""

    def get_body_of_getGridControlEvents(self):
        return "  return std::vector< ::peano4::grid::GridControlEvent >();\n"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def user_should_modify_template(self):
        return False

    _Template_TouchFaceFirstTime = """
  if ({{CLEAR_GUARD}}) {
    // Only the exterior side of the face is needed, but the interior half is
    // unused, so we clear the entire face rather than just one side.
    std::fill_n(
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}.data(),
#if DIMENSIONS == 2
      2 * {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{OVERLAP}},
#else
      2 * {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}} * {{OVERLAP}},
#endif
      0
    );
  }
"""

    _Template_TouchCellFirstTime = """
  if (marker.hasBeenRefined() and not marker.willBeRefined()) {
    std::fill_n(
      {{FINE_GRID_CELL}}.data(),
#if DIMENSIONS == 2
      {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}},
#else
      {{UNKNOWNS}} * {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}} * {{DOFS_PER_AXIS}},
#endif
      0
    );
  }
"""

    _Template_TouchHangingFaceFirstTime = """
  if ({{INTERPOLATE_GUARD}}) {
    const auto leftRightEntry = marker.getSelectedFaceNumber() < DIMENSIONS ? 0 : 1;
    ::toolbox::blockstructured::interpolateHaloLayer(
      marker,
      {{COARSE_GRID_FACE_ACCESSOR_INTERPOLATION}}(marker.getSelectedFaceNumber()).data(),
      {{FINE_GRID_FACE_ACCESSOR_INTERPOLATION}}.data(),
      leftRightEntry,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}}
    );
  }
"""

    _Template_TouchHangingFaceLastTime = """
  if ({{RESTRICT_GUARD}}) {
    const auto facePosition = marker.getSelectedFaceNumber() < DIMENSIONS ? 1 : 0;
    ::toolbox::blockstructured::restrictHaloLayer(
      marker,
      {{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}(marker.getSelectedFaceNumber()).data(),
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}.data(),
      facePosition,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}}
    );
  }
"""

    _Template_CreateFace = """
  if (marker.isInteriorFaceWithinPatch()) {
    for (int facePosition = 0; facePosition < 2; facePosition++) {
      ::toolbox::blockstructured::interpolateHaloLayerFromCoarseCell(
        marker,
        {{COARSE_GRID_CELL}}.data(),
        {{FINE_GRID_FACE_ACCESSOR_INTERPOLATION}}.data(),
        facePosition,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
    }
  } else {
    for (int facePosition = 0; facePosition < 2; facePosition++) {
      ::toolbox::blockstructured::interpolateHaloLayer(
        marker,
        {{COARSE_GRID_FACE_ACCESSOR_INTERPOLATION}}(marker.getSelectedFaceNumber()).data(),
        {{FINE_GRID_FACE_ACCESSOR_INTERPOLATION}}.data(),
        facePosition,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
    }
  }
"""

    _Template_DestroyFace = """
  if (not marker.isInteriorFaceWithinPatch()) {
    for (int facePosition = 0; facePosition < 2; facePosition++) {
      ::toolbox::blockstructured::restrictHaloLayer(
        marker,
        {{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}(marker.getSelectedFaceNumber()).data(),
        {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}.data(),
        facePosition,
        {{DOFS_PER_AXIS}},
        {{UNKNOWNS}}
      );
    }
  }
"""

    _Template_CreateCell = """
  ::toolbox::blockstructured::interpolateCell(
    marker,
    {{COARSE_GRID_CELL}}.data(),
    {{FINE_GRID_CELL}}.data(),
    {{DOFS_PER_AXIS}},
    {{UNKNOWNS}}
  );
"""

    _Template_DestroyCell = """
  ::toolbox::blockstructured::restrictCell(
    marker,
    {{COARSE_GRID_CELL}}.data(),
    {{FINE_GRID_CELL}}.data(),
    {{DOFS_PER_AXIS}},
    {{UNKNOWNS}}
  );
"""

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_CREATE_FACE:
            result = jinja2.Template(self._Template_CreateFace).render(**self.d)
            pass
        if operation_name == ActionSet.OPERATION_DESTROY_FACE:
            result = jinja2.Template(self._Template_DestroyFace).render(**self.d)
            pass
        if operation_name == ActionSet.OPERATION_CREATE_CELL:
            result = jinja2.Template(self._Template_CreateCell).render(**self.d)
            pass
        if operation_name == ActionSet.OPERATION_DESTROY_CELL:
            result = jinja2.Template(self._Template_DestroyCell).render(**self.d)
            pass
        if operation_name == ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
            result = jinja2.Template(self._Template_TouchFaceFirstTime).render(**self.d)
            pass
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            result = jinja2.Template(self._Template_TouchCellFirstTime).render(**self.d)
            pass
        if operation_name == ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME:
            result = jinja2.Template(self._Template_TouchHangingFaceFirstTime).render(
                **self.d
            )
            pass
        if operation_name == ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME:
            result = jinja2.Template(self._Template_TouchHangingFaceLastTime).render(
                **self.d
            )
            pass
        return result

    def get_attributes(self):
        return ""

    def get_includes(self):
        return (
            """
#include "peano4/Loop.h"
#include "toolbox/blockstructured/AMRRoutines.h"
"""
            + self.additional_includes
        )

    def get_clear_guard(self):
        return self.d["CLEAR_GUARD"]

    def get_restrict_guard(self):
        return self.d["RESTRICT_GUARD"]

    def get_interpolate_guard(self):
        return self.d["INTERPOLATE_GUARD"]
