# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import jinja2

import peano4.output


def generate_kernels(
    solver,
    output,
    subdirectory,
):
    kernels_namespace = solver._namespace + ["kernels", solver._name]
    kernels_output_path = subdirectory + "kernels/" + solver._name
    kernels_templates_prefix = os.path.dirname(os.path.realpath(__file__)) + "/kernels/"

    if not os.path.exists(kernels_output_path):
        os.makedirs(kernels_output_path)

    d = output.makefile.dictionary()
    solver._init_dictionary_with_default_parameters(d)
    solver.add_entries_to_text_replacement_dictionary(d)

    solver_file = "Solver"

    generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
        headfile_template=kernels_templates_prefix + f"{solver_file}.template.h",
        cppfile_template=kernels_templates_prefix + f"{solver_file}.template.cpp",
        classname=kernels_output_path + f"/{solver_file}",
        namespace=kernels_namespace,
        subdirectory=subdirectory + ".",
        dictionary=d,
        default_overwrite=True,
        apply_iteratively=True,
    )
    output.add(generated_kernels)
    output.makefile.add_h_file(
        kernels_output_path + f"/{solver_file}.h", generated=True
    )
    output.makefile.add_cpp_file(
        kernels_output_path + f"/{solver_file}.cpp", generated=True
    )
    solver._user_action_set_includes += (
        f'#include "{kernels_output_path}/{solver_file}.h"\n'
    )

    solver._compute_kernel_call = jinja2.Template(
        """
    kernels::{{SOLVER_NAME}}::updateSolution(
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

    boundary_file = "BoundaryConditions"

    generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
        headfile_template=kernels_templates_prefix + f"{boundary_file}.template.h",
        cppfile_template=kernels_templates_prefix + f"{boundary_file}.template.cpph",
        classname=kernels_output_path + f"/{boundary_file}",
        namespace=kernels_namespace,
        subdirectory=subdirectory + ".",
        dictionary=d,
        default_overwrite=True,
        apply_iteratively=True,
    )
    output.add(generated_kernels)
    generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
        headfile_template=kernels_templates_prefix + f"{boundary_file}.template.h",
        cppfile_template=kernels_templates_prefix + f"{boundary_file}.cpp",
        classname=kernels_output_path + f"/{boundary_file}",
        namespace=kernels_namespace,
        subdirectory=subdirectory + ".",
        dictionary=d,
        default_overwrite=True,
        apply_iteratively=True,
    )
    output.add(generated_kernels)
    output.makefile.add_h_file(
        kernels_output_path + f"/{boundary_file}.h", generated=True
    )
    output.makefile.add_cpp_file(
        kernels_output_path + f"/{boundary_file}.cpp", generated=True
    )
    solver._user_action_set_includes += (
        f'#include "{kernels_output_path}/{boundary_file}.h"\n'
    )
