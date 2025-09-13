# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import jinja2

import peano4.output


def generate_kernels(
    solver,
    output,
    subdirectory,
    reconstruction,
):
    kernels_namespace = solver._namespace + ["kernels", solver._name]
    kernels_output_path = subdirectory + "kernels/" + solver._name
    kernels_templates_prefix = os.path.dirname(os.path.realpath(__file__)) + "/kernels/"

    if not os.path.exists(kernels_output_path):
        os.makedirs(kernels_output_path)

    d = output.makefile.dictionary()
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)
    d["RECONSTRUCTION"] = reconstruction

    riemann_solver_file = "RiemannSolver"

    generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
        headfile_template=kernels_templates_prefix
        + f"{riemann_solver_file}.template.h",
        cppfile_template=kernels_templates_prefix
        + f"{riemann_solver_file}.template.cpp",
        classname=kernels_output_path + f"/{riemann_solver_file}",
        namespace=kernels_namespace,
        subdirectory=subdirectory + ".",
        dictionary=d,
        default_overwrite=True,
        apply_iteratively=True,
    )
    output.add(generated_kernels)
    output.makefile.add_h_file(
        kernels_output_path + f"/{riemann_solver_file}.h", generated=True
    )
    output.makefile.add_cpp_file(
        kernels_output_path + f"/{riemann_solver_file}.cpp", generated=True
    )
    solver._user_action_set_includes += (
        f'#include "{kernels_output_path}/{riemann_solver_file}.h"\n'
    )

    solver._compute_kernel_call = jinja2.Template(
        """
    kernels::{{SOLVER_NAME}}::updateSolutionWithRiemann(
      repositories::{{SOLVER_INSTANCE}},
      QIn,
      QOut,
      timeStamp,
      timeStepSize,
      marker.x(),
      marker.h()
    );
""",
        undefined=jinja2.DebugUndefined,
    ).render(**d)


def generate_fused_kernels(
    solver,
    output,
    subdirectory,
    reconstruction,
):
    kernels_namespace = solver._namespace + ["kernels", solver._name]
    kernels_output_path = subdirectory + "kernels/" + solver._name
    kernels_templates_prefix = os.path.dirname(os.path.realpath(__file__)) + "/kernels/"

    if not os.path.exists(kernels_output_path):
        os.makedirs(kernels_output_path)

    d = output.makefile.dictionary()
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)
    d["RECONSTRUCTION"] = reconstruction

    riemann_solver_file = "RiemannSolverFused"

    generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
        headfile_template=kernels_templates_prefix
        + f"{riemann_solver_file}.template.h",
        cppfile_template=kernels_templates_prefix
        + f"{riemann_solver_file}.template.cpp",
        classname=kernels_output_path + f"/{riemann_solver_file}",
        namespace=kernels_namespace,
        subdirectory=subdirectory + ".",
        dictionary=d,
        default_overwrite=True,
        apply_iteratively=True,
    )
    output.add(generated_kernels)
    output.makefile.add_h_file(
        kernels_output_path + f"/{riemann_solver_file}.h", generated=True
    )
    output.makefile.add_cpp_file(
        kernels_output_path + f"/{riemann_solver_file}.cpp", generated=True
    )
    solver._user_action_set_includes += (
        f'#include "{kernels_output_path}/{riemann_solver_file}.h"\n'
    )

    solver._fused_compute_kernel_call = jinja2.Template(
        """
    kernels::{{SOLVER_NAME}}::updateSolutionWithRiemannFused(
      repositories::{{SOLVER_INSTANCE}},
      patchData,
      TARGET_DEVICE
    );
""",
        undefined=jinja2.DebugUndefined,
    ).render(**d)
