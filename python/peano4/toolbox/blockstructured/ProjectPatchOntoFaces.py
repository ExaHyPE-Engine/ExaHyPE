# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4.output

from peano4.solversteps.ActionSet import ActionSet


class ProjectPatchOntoFaces(ActionSet):
    """!
    This class assumes that you have NxNxN patch within your block. It also
    assumes that you have an 2MxNxN patch on your faces. M<N. The code interprets
    these face-associated data as overlap with the patch, hooks into
    touchCellLastTime, and projects the cell's patch data onto the overlap
    (simple copy).

    And then runs over your grid and maps the patch data on the respective entries of the
    face patch which is an auxiliary data structure.
    """

    def __init__(
        self,
        patch,
        patch_overlap,
        solver_name,
        precision,
        guard="true",
        add_assertions=True,
    ):
        """!
        patch: peano4.datamodel.Patch
            This is the input data structure, i.e. the patch from which we extract the boundary data

        patch_overlap: peano4.datamodel.Patch
            Consult remark above about how the dimensions of this overlap patch have to match
        """
        super(ProjectPatchOntoFaces, self).__init__(
            descend_invocation_order=1
        )

        assert patch_overlap.dim[0] % 2 == 0
        assert patch.dim[0] == patch.dim[1]
        assert patch_overlap.dim[1] == patch.dim[0]

        self.guard = guard
        self.solver_name = solver_name
        self.precision = precision
        self.no_of_unknowns = int(patch.no_of_unknowns)
        self.dofs_per_axis = int(patch.dim[0])
        self.total_overlap = int(patch_overlap.dim[0])
        self.overlap_name = patch_overlap.name
        self.patch_name = patch.name
        self.add_assertion_prefix = add_assertions

    def _add_action_set_entries_to_dictionary(self, d):
        assert isinstance(self.no_of_unknowns, int)
        assert isinstance(self.dofs_per_axis, int)
        assert isinstance(self.total_overlap, int)

        d["GUARD"] = self.guard
        d["SOLVER_NAME"] = self.solver_name
        d["PRECISION"] = self.precision
        d["UNKNOWNS"] = self.no_of_unknowns
        d["DOFS_PER_AXIS"] = self.dofs_per_axis
        d["DOFS"] = self.dofs_per_axis * self.dofs_per_axis
        d["DOFS_WITH_OVERLAP"] = (self.dofs_per_axis + self.total_overlap) * (
            self.dofs_per_axis + self.total_overlap
        )
        d["OVERLAP"] = int(self.total_overlap / 2)
        d["FACES_PER_DOF"] = self.total_overlap * self.dofs_per_axis
        d["CELL_ACCESSOR"] = "fineGridCell" + self.patch_name
        d["FACE_ACCESSOR"] = "fineGridFaces" + self.overlap_name
        d["ASSERTION_PREFIX"] = "false" if self.add_assertion_prefix else "true"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        result = super(ProjectPatchOntoFaces, self).get_includes()
        result += f"""
#include "kernels/{self.solver_name}/ProjectPatch.h"
"""
        return result

    def user_should_modify_template(self):
        return False

    _Template_TouchCellLastTime = r"""
  if ({GUARD}) {{
    auto* /*const*/ QIn = {CELL_ACCESSOR}.data();

    {PRECISION}* facesOut[TWO_TIMES_D];
    for (int d = 0; d < TWO_TIMES_D; ++d) {{
      facesOut[d] = {FACE_ACCESSOR}(d).data();
    }}

    kernels::{SOLVER_NAME}::projectPatch(QIn, facesOut);
  }}
"""

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
            d = {}
            self._add_action_set_entries_to_dictionary(d)
            result = self._Template_TouchCellLastTime.format(**d)
            pass
        return result

    def generate_kernels(self, namespace, output, dimensions):
        kernels_namespace = namespace + ["kernels", self.solver_name]
        kernels_output_path = f"kernels/{self.solver_name}"
        kernels_templates_prefix = (
            os.path.dirname(os.path.realpath(__file__)) + "/kernels"
        )

        d = output.makefile.dictionary()
        self._add_action_set_entries_to_dictionary(d)
        d["DIMENSIONS"] = dimensions
        if dimensions == 3:
            d["DOFS"] *= self.dofs_per_axis
            d["DOFS_WITH_OVERLAP"] *= self.dofs_per_axis + self.total_overlap
            d["FACES_PER_DOF"] *= self.dofs_per_axis

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=kernels_templates_prefix + "/ProjectPatch.template.h",
            cppfile_template=kernels_templates_prefix + "/ProjectPatch.template.cpp",
            classname=kernels_output_path + "/ProjectPatch",
            namespace=kernels_namespace,
            subdirectory=".",
            dictionary=d,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(
            kernels_output_path + "/ProjectPatch.h", generated=True
        )
        output.makefile.add_cpp_file(
            kernels_output_path + "/ProjectPatch.cpp", generated=True
        )

    def generate_fused_kernels(self, namespace, output, dimensions):
        kernels_namespace = namespace + ["kernels", self.solver_name]
        kernels_output_path = f"kernels/{self.solver_name}"
        kernels_templates_prefix = (
            os.path.dirname(os.path.realpath(__file__)) + "/kernels"
        )

        d = output.makefile.dictionary()
        self._add_action_set_entries_to_dictionary(d)
        d["DIMENSIONS"] = dimensions
        if dimensions == 3:
            d["DOFS"] *= self.dofs_per_axis
            d["DOFS_WITH_OVERLAP"] *= self.dofs_per_axis + self.total_overlap
            d["FACES_PER_DOF"] *= self.dofs_per_axis

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=kernels_templates_prefix
            + "/ProjectPatchFused.template.h",
            cppfile_template=kernels_templates_prefix
            + "/ProjectPatchFused"
            + (f".{d['GPU']}" if d["GPU"] != "no" else "")
            + ".template.cpp",
            classname=kernels_output_path + "/ProjectPatchFused",
            namespace=kernels_namespace,
            subdirectory=".",
            dictionary=d,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(
            kernels_output_path + "/ProjectPatchFused.h", generated=True
        )
        output.makefile.add_cpp_file(
            kernels_output_path + "/ProjectPatchFused.cpp", generated=True
        )
