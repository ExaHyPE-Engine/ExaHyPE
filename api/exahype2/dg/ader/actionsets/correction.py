# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.actionsets


class Correction(exahype2.actionsets.SolverActionSet):
    """Solves the Riemann problems on the faces and adds their integrals to the cells."""

    _TEMPLATE_SOLVE_RIEMANN_PROBLEM = """
  if ({{FACE_GUARD}}) {
    {{COMPUTE_TIME_STEP_SIZE}}
    ::exahype2::dg::ader::solveRiemannProblemOnFace<
      {{FULL_QUALIFIED_SOLVER_NAME}},
      {{DG_ORDER}} + 1,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{"true" if IS_LINEAR else "false"}},
      {{"true" if HANGING_FACE else "false"}},
      {{"true" if USE_RIEMANN_SOLVER else "false"}},
      {{"true" if USE_FLUX else "false"}},
      {{"true" if USE_DIFFUSIVE_FLUX else "false"}},
      {{"true" if USE_NCP else "false"}},
      {{"true" if USE_MATERIAL_PARAMETERS else "false"}}
    >(
      marker,
      fineGridFace{{SOLVER_NAME}}FaceLabel,
      coarseGridFaces{{SOLVER_NAME}}FaceLabel,
      fineGridFace{{UNKNOWN_IDENTIFIER}}Estimates.data(),
      fineGridFace{{UNKNOWN_IDENTIFIER}}FluxEstimates.data(),
      kernels::{{SOLVER_NAME}}::Quadrature<{{CORRECTOR_COMPUTATION_PRECISION}}>::QuadraturePoints1D,
      kernels::{{SOLVER_NAME}}::Quadrature<{{CORRECTOR_COMPUTATION_PRECISION}}>::QuadratureWeightsOnFace,
      timeStepSize
    );
  }
"""

    _TEMPLATE_MERGE_HANGING_FACE_TIME_STAMPS = """
  if ({{HANGING_FACE_GUARD}}) {
    ::exahype2::dg::ader::mergeHangingFaceTimeStamps(
      marker,
      fineGridFace{{SOLVER_NAME}}FaceLabel,
      coarseGridFaces{{SOLVER_NAME}}FaceLabel
    );
  }
"""

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  if ({{CELL_GUARD}}) {
    // The predictor and corrector advance the same time step.
    [[maybe_unused]] const auto timeStamp    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    [[maybe_unused]] const auto timeStepSize = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize();
    // A postprocessing snippet addresses the updated patch as QNew.
    [[maybe_unused]] auto* const QNew = fineGridCell{{UNKNOWN_IDENTIFIER}}.data();

    ::exahype2::dg::ader::correct<
      {{FULL_QUALIFIED_SOLVER_NAME}},
      {{DG_ORDER}} + 1,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{"true" if IS_LINEAR else "false"}},
      {{"true" if REPORT_EIGENVALUES else "false"}}
    >(
      marker,
      repositories::{{SOLVER_INSTANCE}},
      fineGridCell{{SOLVER_NAME}}CellLabel,
      fineGridFaces{{SOLVER_NAME}}FaceLabel,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates,
      QNew,
      kernels::{{SOLVER_NAME}}::Quadrature<{{CORRECTOR_COMPUTATION_PRECISION}}>::QuadratureWeightsOnFace,
      kernels::{{SOLVER_NAME}}::Quadrature<{{CORRECTOR_COMPUTATION_PRECISION}}>::InverseQuadratureWeightsInCell,
      kernels::{{SOLVER_NAME}}::DGMatrices<{{CORRECTOR_COMPUTATION_PRECISION}}>::BasisFunctionValuesLeft1D,
      kernels::{{SOLVER_NAME}}::DGMatrices<{{CORRECTOR_COMPUTATION_PRECISION}}>::BasisFunctionValuesRight1D,
      kernels::{{SOLVER_NAME}}::Quadrature<{{SOLUTION_STORAGE_PRECISION}}>::QuadraturePoints1D,
      [&]() {
{%- if POSTPROCESS_UPDATED_PATCH %}
        {{POSTPROCESS_UPDATED_PATCH | indent(8)}}
{%- endif %}
      }
    );
  }
"""

    def __init__(self, solver, cell_guard, face_guard, hanging_face_guard):
        super().__init__(solver)
        self.cell_guard = cell_guard
        self.face_guard = face_guard
        self.hanging_face_guard = hanging_face_guard

    @property
    def _reports_eigenvalues(self):
        """The correction is provisional while a limiter runs, so the step it would propose rests
        on candidates the flagging may still reject. The flagging and the recompute report the
        wave speeds of the cells they accept."""
        return (
            self.solver.time_stepping.adaptive
            and not self.solver.subcell_limiter_is_enabled
        )

    def includes(self):
        return super().includes() + ["exahype2/dg/ader/Correct.h"]

    def operation_body(self, operation_name):
        if (
            operation_name
            == exahype2.actionsets.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            return self.render(
                self._TEMPLATE_SOLVE_RIEMANN_PROBLEM,
                FACE_GUARD=self.face_guard,
                HANGING_FACE=False,
            )
        if (
            operation_name
            == exahype2.actionsets.ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME
        ):
            face_guard = (
                "("
                + self.hanging_face_guard
                + ") and not marker.isInteriorFaceWithinPatch()"
            )
            return self.render(
                self._TEMPLATE_SOLVE_RIEMANN_PROBLEM,
                FACE_GUARD=face_guard,
                HANGING_FACE=True,
            ) + self.render(
                self._TEMPLATE_MERGE_HANGING_FACE_TIME_STAMPS,
                HANGING_FACE_GUARD=self.hanging_face_guard,
            )
        if (
            operation_name
            == exahype2.actionsets.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            return self.render(
                self._TEMPLATE_TOUCH_CELL_FIRST_TIME,
                CELL_GUARD=self.cell_guard,
                REPORT_EIGENVALUES=self._reports_eigenvalues,
            )
        if operation_name == exahype2.actionsets.ActionSet.OPERATION_END_TRAVERSAL:
            return "  repositories::{}.reduceLocalUpdates();\n".format(
                self.solver.instance_name
            )
        return ""
