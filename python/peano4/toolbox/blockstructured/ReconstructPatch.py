# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import jinja2

import peano4.output

from peano4.solversteps.ActionSet import ActionSet


class ReconstructPatch(ActionSet):
    """!
    This class assumes that you have an NxNxN patch within your block. It also assumes that
    you have an 2MxNxN patch on your faces. M<N. The code interprets these face-associated
    data as overlap with the patch, hooks into touchCellLastTime, and projects the cell's
    patch data onto the overlap (simple copy). See ProjectPatchOntoFaces for details.

    If the face patches are auxiliary patches holding copies from the cell patches, then
    we can reconstruct N+2M x N+2M x N+2M patches per cell whenever we hit a cell: We
    create a new temporary array, and copy data from both the cell patch and the faces
    into this array (gather operation). After that, we can launch the passed functor
    giving it access to the temporary, large array plus the original patch data.
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
        """!
      patch: peano4.datamodel.Patch
          Patch which is to be reconstructed

      patch_overlap: peano4.datamodel.Patch
          Consult remark above about how the dimensions of this overlap patch have to match

      patch_reconstructed: peano4.datamodel.Patch
          Patch which is used to reconstruct
        """
        super(ReconstructPatch, self).__init__(
            descend_invocation_order=1, parallel=False
        )

        assert(patch_overlap.dim[0] % 2 == 0)
        assert(patch.dim[0] == patch.dim[1])
        assert(patch_overlap.dim[1] == patch.dim[0])

        self.guard = guard
        self.solver_name = solver_name
        self.precision = precision
        self.no_of_unknowns = int(patch.no_of_unknowns)
        self.dofs_per_axis = int(patch.dim[0])
        self.total_overlap = int(patch_overlap.dim[0])
        self.overlap_name = patch_overlap.name
        self.patch_name = patch.name
        self.patch_reconstructed_name = patch_reconstructed.name
        self.add_assertions_to_halo_exchange = add_assertions_to_halo_exchange

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
        d["DOFS_WITH_OVERLAP"] = (self.dofs_per_axis + self.total_overlap) * (self.dofs_per_axis + self.total_overlap)
        d["OVERLAP"] = int(self.total_overlap / 2)
        d["FACES_PER_DOF"] = self.total_overlap * self.dofs_per_axis
        d["CELL_ACCESSOR"] = "fineGridCell" + self.patch_name
        d["FACE_ACCESSOR"] = "fineGridFaces" + self.overlap_name
        d["RECONSTRUCTED_ACCESSOR"] = "fineGridCell" + self.patch_reconstructed_name

        if self.add_assertions_to_halo_exchange:
            d["ASSERTION_PREFIX_FOR_HALO"] = "false"
        else:
            d["ASSERTION_PREFIX_FOR_HALO"] = "true"

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        result = super(ReconstructPatch, self).get_includes()
        result += f"""
#include "kernels/{self.solver_name}/ReconstructPatch.h"
"""
        return result

    def user_should_modify_template(self):
        return False

    _Template_ReconstructPatch = r"""
  if ({GUARD}) {{
    auto* /*const*/ QIn = {CELL_ACCESSOR}.value;
    auto* /*const*/ QOut = {RECONSTRUCTED_ACCESSOR}.value;

    {PRECISION}* faces[TwoTimesD];
    for (int d = 0; d < TwoTimesD; ++d) {{
      faces[d] = {FACE_ACCESSOR}(d).value;
    }}

    kernels::{SOLVER_NAME}::reconstructPatch(QIn, faces, QOut);
  }}
"""

    def get_body_of_operation(self, operation_name):
        result = "\n"
        if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
            d = {}
            self._add_action_set_entries_to_dictionary(d)
            result = self._Template_ReconstructPatch.format(**d)
            pass
        return result

    def generate_kernels(self, namespace, output, dimensions, subdirectory):
        kernels_namespace = namespace + ["kernels", self.solver_name]
        kernels_output_path = subdirectory + f"kernels/{self.solver_name}"
        kernels_templates_prefix = os.path.dirname(os.path.realpath(__file__)) + "/kernels"

        if not os.path.exists(kernels_output_path):
            os.makedirs(kernels_output_path)

        d = output.makefile.dictionary()
        self._add_action_set_entries_to_dictionary(d)
        if dimensions == 3:
            d["DOFS"] *= self.dofs_per_axis
            d["DOFS_WITH_OVERLAP"] *= (self.dofs_per_axis + self.total_overlap)
            d["FACES_PER_DOF"] *= self.dofs_per_axis

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=kernels_templates_prefix + "/ReconstructPatch.template.h",
            cppfile_template=kernels_templates_prefix + "/ReconstructPatch.template.cpp",
            classname=kernels_output_path + "/ReconstructPatch",
            namespace=kernels_namespace,
            subdirectory=subdirectory + ".",
            dictionary=d,
            default_overwrite=True,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(kernels_output_path + "/ReconstructPatch.h", generated=True)
        output.makefile.add_cpp_file(kernels_output_path + "/ReconstructPatch.cpp", generated=True)

    def generate_fused_kernels(self, namespace, output, dimensions, subdirectory):
        kernels_namespace = namespace + ["kernels", self.solver_name]
        kernels_output_path = subdirectory + f"kernels/{self.solver_name}"
        kernels_templates_prefix = os.path.dirname(os.path.realpath(__file__)) + "/kernels"

        if not os.path.exists(kernels_output_path):
            os.makedirs(kernels_output_path)

        d = output.makefile.dictionary()
        self._add_action_set_entries_to_dictionary(d)
        if dimensions == 3:
            d["DOFS"] *= self.dofs_per_axis
            d["DOFS_WITH_OVERLAP"] *= (self.dofs_per_axis + self.total_overlap)
            d["FACES_PER_DOF"] *= self.dofs_per_axis

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=kernels_templates_prefix + "/ReconstructPatchFused.template.h",
            cppfile_template=kernels_templates_prefix + "/ReconstructPatchFused.template.cpp",
            classname=kernels_output_path + "/ReconstructPatchFused",
            namespace=kernels_namespace,
            subdirectory=subdirectory + ".",
            dictionary=d,
            default_overwrite=True,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(kernels_output_path + "/ReconstructPatchFused.h", generated=True)
        output.makefile.add_cpp_file(kernels_output_path + "/ReconstructPatchFused.cpp", generated=True)
