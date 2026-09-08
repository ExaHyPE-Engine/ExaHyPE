# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import jinja2

import peano4.output

import exahype2.solvers.aderdg.kernels

from exahype2.solvers.PDETerms import PDETerms


def generate_kernels(solver, dictionary, output):
    kernels_namespace = solver._namespace + ["kernels", solver._name]
    kernels_output_path = "kernels/" + solver._name
    kernels_templates_prefix = os.path.dirname(os.path.realpath(__file__)) + "/kernels/"

    generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
        headfile_template=kernels_templates_prefix + "CellErrorIntegral.template.h",
        cppfile_template=kernels_templates_prefix + "CellErrorIntegral.template.cpp",
        classname=kernels_output_path + "/CellErrorIntegral",
        namespace=kernels_namespace,
        subdirectory=".",
        dictionary=dictionary,
        apply_iteratively=True,
    )
    output.add(generated_kernels)
    output.makefile.add_h_file(
        kernels_output_path + "/CellErrorIntegral.h", generated=True
    )
    output.makefile.add_cpp_file(
        kernels_output_path + "/CellErrorIntegral.cpp", generated=True
    )
    solver._user_action_set_includes += (
        f'#include "{kernels_output_path}/CellErrorIntegral.h"\n'
    )

    generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
        headfile_template=kernels_templates_prefix + "MaxScaledEigenvalue.template.h",
        cppfile_template=kernels_templates_prefix + "MaxScaledEigenvalue.template.cpp",
        classname=kernels_output_path + "/MaxScaledEigenvalue",
        namespace=kernels_namespace,
        subdirectory=".",
        dictionary=dictionary,
        apply_iteratively=True,
    )
    output.add(generated_kernels)
    output.makefile.add_h_file(
        kernels_output_path + "/MaxScaledEigenvalue.h", generated=True
    )
    output.makefile.add_cpp_file(
        kernels_output_path + "/MaxScaledEigenvalue.cpp", generated=True
    )
    solver._user_action_set_includes += (
        f'#include "{kernels_output_path}/MaxScaledEigenvalue.h"\n'
    )

    generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
        headfile_template=kernels_templates_prefix + "FaceIntegral.template.h",
        cppfile_template=kernels_templates_prefix + "FaceIntegral.template.cpp",
        classname=kernels_output_path + "/FaceIntegral",
        namespace=kernels_namespace,
        subdirectory=".",
        dictionary=dictionary,
        apply_iteratively=True,
    )
    output.add(generated_kernels)
    output.makefile.add_h_file(kernels_output_path + "/FaceIntegral.h", generated=True)
    output.makefile.add_cpp_file(
        kernels_output_path + "/FaceIntegral.cpp", generated=True
    )
    solver._user_action_set_includes += (
        f'#include "{kernels_output_path}/FaceIntegral.h"\n'
    )

    generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
        headfile_template=kernels_templates_prefix + "RiemannSolver.template.h",
        cppfile_template=(
            kernels_templates_prefix + "RiemannSolverLinear.template.cpp"
            if solver._is_linear
            else kernels_templates_prefix + "RiemannSolverNonlinear.template.cpp"
        ),
        classname=kernels_output_path + "/RiemannSolver",
        namespace=kernels_namespace,
        subdirectory=".",
        dictionary=dictionary,
        apply_iteratively=True,
    )
    output.add(generated_kernels)
    output.makefile.add_h_file(kernels_output_path + "/RiemannSolver.h", generated=True)
    output.makefile.add_cpp_file(
        kernels_output_path + "/RiemannSolver.cpp", generated=True
    )
    solver._user_action_set_includes += (
        f'#include "{kernels_output_path}/RiemannSolver.h"\n'
    )

    if solver._point_sources_implementation != PDETerms.None_Implementation:
        generated_kernels = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
            headfile_template=kernels_templates_prefix + "PointSources.template.h",
            cppfile_template=kernels_templates_prefix + "PointSources.template.cpp",
            classname=kernels_output_path + "/PointSources",
            namespace=kernels_namespace,
            subdirectory=".",
            dictionary=dictionary,
            apply_iteratively=True,
        )
        output.add(generated_kernels)
        output.makefile.add_h_file(
            kernels_output_path + "/PointSources.h", generated=True
        )
        output.makefile.add_cpp_file(
            kernels_output_path + "/PointSources.cpp", generated=True
        )
        solver._user_action_set_includes += (
            f'#include "{kernels_output_path}/PointSources.h"\n'
        )

    exahype2.solvers.aderdg.kernels.BufferSizes(dictionary).generate_kernels(
        solver, kernels_namespace, output, kernels_templates_prefix, kernels_output_path
    )
    exahype2.solvers.aderdg.kernels.DGMatrices(dictionary).generate_kernels(
        solver, kernels_namespace, output, kernels_templates_prefix, kernels_output_path
    )
    exahype2.solvers.aderdg.kernels.Quadrature(dictionary).generate_kernels(
        solver, kernels_namespace, output, kernels_templates_prefix, kernels_output_path
    )
    exahype2.solvers.aderdg.kernels.FusedSpaceTimePredictorVolumeIntegral(
        dictionary
    ).generate_kernels(
        solver, kernels_namespace, output, kernels_templates_prefix, kernels_output_path
    )

    solver._space_time_predictor_call = jinja2.Template(
        """
{% if IS_LINEAR %}
{% if USE_POINT_SOURCE %}
    const auto pointSources = kernels::{{SOLVER_NAME}}::pointSources(
      repositories::{{SOLVER_INSTANCE}},
      marker.getX(),
      marker.getH()
    );
    if (not pointSources.empty()) {
      kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegralWithPointSources<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[0]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
        repositories::{{SOLVER_INSTANCE}},
        stateEstimates,
        fluxEstimates,
        updatedState,
        marker.getX(),
        marker.getH(),
        timeStamp,
        timeStepSize,
        pointSources
      );
    } else {
      kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[0]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
        repositories::{{SOLVER_INSTANCE}},
        stateEstimates,
        fluxEstimates,
        updatedState,
        marker.getX(),
        marker.getH(),
        timeStamp,
        timeStepSize
      );
    }
{% else %}
    kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[0]}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
      repositories::{{SOLVER_INSTANCE}},
      stateEstimates,
      fluxEstimates,
      updatedState,
      marker.getX(),
      marker.getH(),
      timeStamp,
      timeStepSize
    );
{% endif %}
{% else %}
    [[maybe_unused]] const int numberOfPicardIterations = kernels::{{SOLVER_NAME}}::fusedSpaceTimePredictorVolumeIntegral<{{SOLUTION_STORAGE_PRECISION}}, {{PREDICTOR_COMPUTATION_PRECISIONS[0]}}, {{PICARD_COMPUTATION_PRECISION}}, {{CORRECTOR_COMPUTATION_PRECISION}}>(
      repositories::{{SOLVER_INSTANCE}},
      stateEstimates,
      fluxEstimates,
      updatedState,
      marker.getX(),
      marker.getH(),
      timeStamp,
      timeStepSize
    );
{% if SUBCELL_LIMITER %}
    // Predictor divergence invalidates the DG candidate, so the limiter recomputes
    // the cell as described by Dumbser et al., J. Comput. Phys. 278 (2014) 47-75.
    fineGridCell{{SOLVER_NAME}}CellLabel.setPredictorDiverged(numberOfPicardIterations < 0);
{% else %}
    assertion(
      numberOfPicardIterations >= 0,
      "The space-time predictor diverged. Configure a subcell limiter so failing cells are recomputed.",
      marker.toString()
    );
{% endif %}
{% endif %}
""",
        undefined=jinja2.DebugUndefined,
    ).render(**dictionary)

    solver._riemann_solver_call = jinja2.Template(
        """
    kernels::{{SOLVER_NAME}}::riemannSolver<{{CORRECTOR_COMPUTATION_PRECISION}}>(
      repositories::{{SOLVER_INSTANCE}},
      leftFluxEstimates,
      rightFluxEstimates,
      leftStateEstimates,
      rightStateEstimates,
      timeStamp,
      timeStepSize,
      faceCentre,
      marker.getH(),
      direction,
      isBoundary,
      faceNumber
    );
""",
        undefined=jinja2.DebugUndefined,
    ).render(**dictionary)

    solver._face_integral_call = jinja2.Template(
        """
    kernels::{{SOLVER_NAME}}::faceIntegral(
      updatedState,
      faceFluxEstimates,
      direction,
      faceOrientation,
      1.0 / marker.getH()[direction],
      timeStepSize
    );
""",
        undefined=jinja2.DebugUndefined,
    ).render(**dictionary)
