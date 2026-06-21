# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .AbstractFVActionSet import AbstractFVActionSet

import peano4.solversteps

import os
import jinja2

class HandleBoundary(AbstractFVActionSet):
    """
    The global periodic boundary conditions are set in the Constants.h.

    Boundary conditions are applied to QNew and QOld. The initialisation sweep
    projects QNew, and the first solve sweep rolls it over before this action
    set runs, so QOld is available here.
    """

    TemplateHandleBoundary_Prologue = """
    if (
      {{PREDICATE}}
      and
      not marker.hasBeenRefined()
    ) {
"""

    TemplateHandleBoundary_KernelCalls = """
      for (int f = 0; f < TWO_TIMES_D; f++) {
        if (not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[f%DIMENSIONS] and fineGridFaces{{SOLVER_NAME}}FaceLabel(f).getBoundary()) {
          kernels::{{SOLVER_NAME}}::handleBoundary(
            repositories::{{SOLVER_INSTANCE}},
            marker.getX(),
            marker.getH(),
            {{FACE_METADATA_ACCESSOR}}(f).getNewTimeStamp(f<DIMENSIONS ? 1 : 0),
            repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
            f,
            fineGridFaces{{UNKNOWN_IDENTIFIER}}New(f).data()
          );
          kernels::{{SOLVER_NAME}}::handleBoundary(
            repositories::{{SOLVER_INSTANCE}},
            marker.getX(),
            marker.getH(),
            {{FACE_METADATA_ACCESSOR}}(f).getOldTimeStamp(f<DIMENSIONS ? 1 : 0),
            repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
            f,
            fineGridFaces{{UNKNOWN_IDENTIFIER}}Old(f).data()
          );
"""

    TemplateHandleBoundary_Epilogue = """
          bool isLeftEntryOutside = f < DIMENSIONS;
          double innerTimeStamp;

          innerTimeStamp = {{FACE_METADATA_ACCESSOR}}(f).getUpdatedTimeStamp(isLeftEntryOutside ? 1 : 0);
          {{FACE_METADATA_ACCESSOR}}(f).setUpdatedTimeStamp(isLeftEntryOutside ? 0 : 1, innerTimeStamp);

          innerTimeStamp = {{FACE_METADATA_ACCESSOR}}(f).getNewTimeStamp(isLeftEntryOutside ? 1 : 0);
          {{FACE_METADATA_ACCESSOR}}(f).setNewTimeStamp(isLeftEntryOutside ? 0 : 1, innerTimeStamp);

          innerTimeStamp = {{FACE_METADATA_ACCESSOR}}(f).getOldTimeStamp(isLeftEntryOutside ? 1 : 0);
          {{FACE_METADATA_ACCESSOR}}(f).setOldTimeStamp(isLeftEntryOutside ? 0 : 1, innerTimeStamp);
        }
      }
    }
"""

    def __init__(self, solver, guard):
        AbstractFVActionSet.__init__(self, solver)
        self.guard = guard
        self.solver_name = solver._name

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            self.d = {}
            self._solver._init_dictionary_with_default_parameters(self.d)
            self._solver.add_entries_to_text_replacement_dictionary(self.d)
            self.d["PREDICATE"] = self.guard
            self.d["FACE_METADATA_ACCESSOR"] = (
                "fineGridFaces" + self._solver._face_label.name
            )
            result = jinja2.Template(self.TemplateHandleBoundary_Prologue).render(
                **self.d
            )
            result += jinja2.Template(self.TemplateHandleBoundary_KernelCalls).render(
                **self.d
            )
            result += jinja2.Template(self.TemplateHandleBoundary_Epilogue).render(
                **self.d
            )
            pass
        return result

    def get_includes(self):
        return f"""
#include "kernels/{self.solver_name}/HandleBoundary.h"
""" + AbstractFVActionSet.get_includes(
            self
        )

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def generate_kernels(self, namespace, output, dimensions):
        kernels_namespace = namespace + ["kernels", self._solver._name]
        kernels_output_path = f"kernels/{self._solver._name}"
        kernels_templates_prefix = (
            os.path.dirname(os.path.realpath(__file__)) + "/kernels"
        )

        if not os.path.exists(kernels_output_path):
            os.makedirs(kernels_output_path)

        d = output.makefile.dictionary()
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=kernels_templates_prefix + "/HandleBoundary.template.h",
            cppfile_template=kernels_templates_prefix + "/HandleBoundary.template.cpp",
            classname=kernels_output_path + "/HandleBoundary",
            namespace=kernels_namespace,
            subdirectory=".",
            dictionary=d,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(
            kernels_output_path + "/HandleBoundary.h", generated=True
        )
        output.makefile.add_cpp_file(
            kernels_output_path + "/HandleBoundary.cpp", generated=True
        )
        if self._solver._fuse_compute_kernels:
            self.generate_fused_kernels(namespace, output, dimensions)

    def generate_fused_kernels(self, namespace, output, dimensions):
        kernels_namespace = namespace + ["kernels", self._solver._name]
        kernels_output_path = f"kernels/{self._solver._name}"
        kernels_templates_prefix = (
            os.path.dirname(os.path.realpath(__file__)) + "/kernels"
        )

        if not os.path.exists(kernels_output_path):
            os.makedirs(kernels_output_path)

        d = output.makefile.dictionary()
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)

        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=kernels_templates_prefix
            + "/HandleBoundaryFused.template.h",
            cppfile_template=kernels_templates_prefix
            + "/HandleBoundaryFused"
            + (f".{d['GPU']}" if d["GPU"] != "no" else "")
            + ".template.cpp",
            classname=kernels_output_path + "/HandleBoundaryFused",
            namespace=kernels_namespace,
            subdirectory=".",
            dictionary=d,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(
            kernels_output_path + "/HandleBoundaryFused.h", generated=True
        )
        output.makefile.add_cpp_file(
            kernels_output_path + "/HandleBoundaryFused.cpp", generated=True
        )
