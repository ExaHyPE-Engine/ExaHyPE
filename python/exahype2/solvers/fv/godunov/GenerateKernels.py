# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import jinja2

import peano4.output


def generate_kernels(solver,
                     namespace,
                     output,
                     subdirectory,
                     dimensions,
                    ):
    kernels_namespace = namespace + ["kernels", "fv", "godunov", solver._name]
    kernels_output_path = subdirectory + "kernels/fv/godunov/" + solver._name
    kernels_templates_prefix = os.path.dirname(os.path.realpath(__file__)) + "/kernels/"
    if not os.path.exists(kernels_output_path):
        os.makedirs(kernels_output_path)

    d = output.makefile.dictionary()
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)
    d["DIMENSIONS"] = dimensions
    d["KERNELS_NAMESPACE"] = "::".join(kernels_namespace)
    d["FULL_QUALIFIED_SOLVER_NAME"] = "::".join(namespace) + "::" + solver._name

    riemann_solver_file = "RiemannSolver"

    generated_kernels = (
        peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=kernels_templates_prefix + f"{riemann_solver_file}.template.h",
            cppfile_template=kernels_templates_prefix + f"{riemann_solver_file}.template.cpp",
            classname=kernels_output_path + f"/{riemann_solver_file}",
            namespace=kernels_namespace,
            subdirectory=subdirectory + ".",
            dictionary=d,
            default_overwrite=True,
            apply_iteratively=True,
          )
    )
    output.add(generated_kernels)
    output.makefile.add_h_file(kernels_output_path + f"/{riemann_solver_file}.h", generated=True)
    output.makefile.add_cpp_file(kernels_output_path + f"/{riemann_solver_file}.cpp", generated=True)
    solver._user_action_set_includes += f'#include "{kernels_output_path}/{riemann_solver_file}.h"\n'

    Template_Riemann = """
    {{KERNELS_NAMESPACE}}::updateSolutionWithRiemann(
      repositories::{{SOLVER_INSTANCE}},
      oldQWithHalo,
      newQ,
      timeStamp,
      timeStepSize,
      marker.x(),
      marker.h()
    );
"""

    Template_Riemann_Stateless = """
    {{KERNELS_NAMESPACE}}::updateSolutionWithRiemann(
      oldQWithHalo,
      newQ,
      timeStamp,
      timeStepSize,
      marker.x(),
      marker.h()
    );
"""

    if solver._pde_terms_without_state:
        solver._compute_kernel_call = jinja2.Template(Template_Riemann_Stateless, undefined=jinja2.DebugUndefined).render(**d)
    else:
        solver._compute_kernel_call = jinja2.Template(Template_Riemann, undefined=jinja2.DebugUndefined).render(**d)

def generate_fused_kernels(solver,
                           namespace,
                           output,
                           subdirectory,
                           dimensions,
                          ):
    kernels_namespace = namespace + ["kernels", "fv", "godunov", solver._name]
    kernels_output_path = subdirectory + "kernels/fv/godunov/" + solver._name
    kernels_templates_prefix = os.path.dirname(os.path.realpath(__file__)) + "/kernels/"
    if not os.path.exists(kernels_output_path):
        os.makedirs(kernels_output_path)

    d = output.makefile.dictionary()
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)
    d["DIMENSIONS"] = dimensions
    d["KERNELS_NAMESPACE"] = "::".join(kernels_namespace)
    d["FULL_QUALIFIED_SOLVER_NAME"] = "::".join(namespace) + "::" + solver._name

    riemann_solver_file = "RiemannSolverFused"

    generated_kernels = (
        peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=kernels_templates_prefix + f"{riemann_solver_file}.template.h",
            cppfile_template=kernels_templates_prefix + f"{riemann_solver_file}.template.cpp",
            classname=kernels_output_path + f"/{riemann_solver_file}",
            namespace=kernels_namespace,
            subdirectory=subdirectory + ".",
            dictionary=d,
            default_overwrite=True,
            apply_iteratively=True,
          )
    )
    output.add(generated_kernels)
    output.makefile.add_h_file(kernels_output_path + f"/{riemann_solver_file}.h", generated=True)
    output.makefile.add_cpp_file(kernels_output_path + f"/{riemann_solver_file}.cpp", generated=True)
    solver._user_action_set_includes += f'#include "{kernels_output_path}/{riemann_solver_file}.h"\n'

    Template_Riemann_Fused = """
    {{KERNELS_NAMESPACE}}::updateSolutionWithRiemannFused(
      patchData,
      targetDevice
    );
"""

    solver._fused_compute_kernel_call = jinja2.Template(Template_Riemann_Fused, undefined=jinja2.DebugUndefined).render(**d)
