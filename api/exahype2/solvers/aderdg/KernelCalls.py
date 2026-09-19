# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import jinja2


def basis_tables(solver_name, precision):
    """
    Returns the C++ initialiser of the BasisTables aggregate that points at
    the solver's operator tables of the given precision.
    """
    quadrature = f"kernels::{solver_name}::Quadrature<{precision}>"
    matrices = f"kernels::{solver_name}::DGMatrices<{precision}>"
    return (
        f"::exahype2::dg::ader::BasisTables<{precision}>{{"
        f"{quadrature}::QuadraturePoints1D, "
        f"{quadrature}::QuadratureWeights1D, "
        f"{quadrature}::InverseQuadratureWeights1D, "
        f"{quadrature}::QuadratureWeightsInCell, "
        f"{quadrature}::InverseQuadratureWeightsInCell, "
        f"{matrices}::StiffnessOperator1D, "
        f"{matrices}::StiffnessOperatorTransposed1D, "
        f"{matrices}::DerivativeOperator1D, "
        f"{matrices}::InversePicardOperator1D, "
        f"{matrices}::BasisFunctionValuesLeft1D, "
        f"{matrices}::BasisFunctionValuesRight1D}}"
    )


SPACE_TIME_PREDICTOR_CALL = """
    using ScratchDeleter = decltype([](auto* const memory) {
      tarch::accelerator::freeHeapMemory(memory);
    });
{% if IS_LINEAR %}
    using PredictorScratch = ::exahype2::dg::ader::LinearPredictorScratch<
      {{DG_ORDER}} + 1, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{"true" if USE_NCP else "false"}}, {{"true" if USE_SOURCE else "false"}}, {{"true" if USE_POINT_SOURCE else "false"}}
    >;
    static thread_local const std::unique_ptr<{{PREDICTOR_COMPUTATION_PRECISIONS[0]}}[], ScratchDeleter> predictorScratch(
      static_cast<{{PREDICTOR_COMPUTATION_PRECISIONS[0]}}*>(tarch::accelerator::allocateHeapMemory(
        PredictorScratch::Size * sizeof({{PREDICTOR_COMPUTATION_PRECISIONS[0]}})
      ))
    );
{% if USE_POINT_SOURCE %}
    int pointSourceIndices[{{SOLVER_NAME}}::NumberOfPointSources];
    const double* const pointSourceLocations = &{{FULL_QUALIFIED_SOLVER_NAME}}::pointSourceLocation[0][0];
    const int numberOfPointSources = ::exahype2::dg::ader::selectPointSources<{{SOLVER_NAME}}::NumberOfPointSources>(
      pointSourceIndices, pointSourceLocations, marker.getX(), marker.getH()
    );
    if (numberOfPointSources > 0) {
      ::exahype2::dg::ader::linearSpaceTimePredictor<
        {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}} + 1, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{"true" if USE_FLUX else "false"}}, {{"true" if USE_NCP else "false"}}, {{"true" if USE_SOURCE else "false"}},
        {{"true" if USE_MATERIAL_PARAMETERS else "false"}}, true
      >(
        stateEstimates, fluxEstimates, updatedState, predictorScratch.get(), pointSourceIndices, numberOfPointSources,
        pointSourceLocations, {{PREDICTOR_TABLES}}, marker.getX(), marker.getH(), timeStamp, timeStepSize
      );
    } else {
{% else %}
    {
      const int* const    pointSourceIndices   = nullptr;
      const double* const pointSourceLocations = nullptr;
{% endif %}
      ::exahype2::dg::ader::linearSpaceTimePredictor<
        {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}} + 1, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{"true" if USE_FLUX else "false"}}, {{"true" if USE_NCP else "false"}}, {{"true" if USE_SOURCE else "false"}},
        {{"true" if USE_MATERIAL_PARAMETERS else "false"}}, false
      >(
        stateEstimates, fluxEstimates, updatedState, predictorScratch.get(), pointSourceIndices, 0,
        pointSourceLocations, {{PREDICTOR_TABLES}}, marker.getX(), marker.getH(), timeStamp, timeStepSize
      );
    }
{% else %}
    using PredictorScratch = ::exahype2::dg::ader::NonlinearPredictorScratch<
      {{DG_ORDER}} + 1, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{"true" if USE_FLUX else "false"}}, {{"true" if USE_NCP else "false"}}, {{"true" if USE_SOURCE else "false"}},
      {{"true" if USE_DIFFUSIVE_FLUX else "false"}}, {{"true" if USE_DIFFUSIVE_SOURCE else "false"}}
    >;
    static thread_local const std::unique_ptr<{{PREDICTOR_COMPUTATION_PRECISIONS[0]}}[], ScratchDeleter> timeAveragedScratch(
      static_cast<{{PREDICTOR_COMPUTATION_PRECISIONS[0]}}*>(tarch::accelerator::allocateHeapMemory(
        PredictorScratch::TimeAveragedSize * sizeof({{PREDICTOR_COMPUTATION_PRECISIONS[0]}})
      ))
    );
    static thread_local const std::unique_ptr<{{PICARD_COMPUTATION_PRECISION}}[], ScratchDeleter> picardScratch(
      static_cast<{{PICARD_COMPUTATION_PRECISION}}*>(tarch::accelerator::allocateHeapMemory(
        PredictorScratch::PicardSize * sizeof({{PICARD_COMPUTATION_PRECISION}})
      ))
    );
    [[maybe_unused]] const int numberOfPicardIterations = ::exahype2::dg::ader::spaceTimePredictor<
      {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}} + 1, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{"true" if USE_FLUX else "false"}}, {{"true" if USE_NCP else "false"}}, {{"true" if USE_SOURCE else "false"}},
      {{"true" if USE_DIFFUSIVE_FLUX else "false"}}, {{"true" if USE_DIFFUSIVE_SOURCE else "false"}}
    >(
      stateEstimates, fluxEstimates, updatedState, timeAveragedScratch.get(), picardScratch.get(),
      {{PREDICTOR_TABLES}}, {{PICARD_TABLES}}, marker.getX(), marker.getH(), timeStamp, timeStepSize,
      {{SOLVER_NAME}}::PicardResidualTolerance
    );
{% if SUBCELL_LIMITER %}
    // A diverged predictor holds no candidate, so the limiter recomputes the cell.
    fineGridCell{{SOLVER_NAME}}CellLabel.setPredictorDiverged(numberOfPicardIterations < 0);
{% else %}
    assertion(
      numberOfPicardIterations >= 0,
      "The space-time predictor diverged. Configure a subcell limiter so failing cells are recomputed.",
      marker.toString()
    );
{% endif %}
{% endif %}
"""


RIEMANN_SOLVER_CALL = """
{% if USE_RIEMANN_SOLVER %}
    {{FULL_QUALIFIED_SOLVER_NAME}}::riemannSolver(
      leftFluxEstimates, rightFluxEstimates, leftStateEstimates, rightStateEstimates,
      timeStamp, timeStepSize, faceCentre, marker.getH(), direction, isBoundary, faceNumber
    );
{% elif IS_LINEAR %}
    ::exahype2::dg::solveLinearRiemannProblem<
      {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}} + 1, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{"true" if USE_FLUX else "false"}}, {{"true" if USE_NCP else "false"}}, {{"true" if USE_MATERIAL_PARAMETERS else "false"}}
    >(
      leftFluxEstimates, rightFluxEstimates, leftStateEstimates, rightStateEstimates,
      kernels::{{SOLVER_NAME}}::Quadrature<{{CORRECTOR_COMPUTATION_PRECISION}}>::QuadratureWeightsOnFace,
      faceCentre, marker.getH(), timeStamp, timeStepSize, direction
    );
{% else %}
    ::exahype2::dg::solveRiemannProblem<
      {{FULL_QUALIFIED_SOLVER_NAME}}, {{DG_ORDER}} + 1, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{"true" if USE_NCP else "false"}}
    >(
      leftFluxEstimates, rightFluxEstimates, leftStateEstimates, rightStateEstimates,
      kernels::{{SOLVER_NAME}}::Quadrature<{{CORRECTOR_COMPUTATION_PRECISION}}>::QuadratureWeightsOnFace,
      faceCentre, marker.getH(), timeStamp, timeStepSize, direction
    );
{% endif %}
"""


FACE_INTEGRAL_CALL = """
    ::exahype2::dg::addFaceIntegral<{{DG_ORDER}} + 1, {{NUMBER_OF_UNKNOWNS}}, {{"true" if IS_LINEAR else "false"}}>(
      updatedState,
      faceFluxEstimates,
      kernels::{{SOLVER_NAME}}::Quadrature<{{CORRECTOR_COMPUTATION_PRECISION}}>::QuadratureWeightsOnFace,
      kernels::{{SOLVER_NAME}}::Quadrature<{{CORRECTOR_COMPUTATION_PRECISION}}>::InverseQuadratureWeightsInCell,
      kernels::{{SOLVER_NAME}}::DGMatrices<{{CORRECTOR_COMPUTATION_PRECISION}}>::BasisFunctionValuesLeft1D,
      kernels::{{SOLVER_NAME}}::DGMatrices<{{CORRECTOR_COMPUTATION_PRECISION}}>::BasisFunctionValuesRight1D,
      direction,
      faceOrientation,
      1.0 / marker.getH()(direction),
      timeStepSize
    );
"""


def add_kernel_calls_to_dictionary(d):
    """Renders the three kernel calls the action sets paste into their bodies."""
    d["PREDICTOR_TABLES"] = basis_tables(
        d["SOLVER_NAME"], d["PREDICTOR_COMPUTATION_PRECISIONS"][0]
    )
    d["PICARD_TABLES"] = basis_tables(
        d["SOLVER_NAME"], d["PICARD_COMPUTATION_PRECISION"]
    )
    for key, template in (
        ("SPACE_TIME_PREDICTOR_CALL", SPACE_TIME_PREDICTOR_CALL),
        ("RIEMANN_SOLVER_CALL", RIEMANN_SOLVER_CALL),
        ("FACE_INTEGRAL_CALL", FACE_INTEGRAL_CALL),
    ):
        d[key] = jinja2.Template(template, undefined=jinja2.DebugUndefined).render(**d)
