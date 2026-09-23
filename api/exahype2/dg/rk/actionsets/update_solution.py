# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.actionsets


class UpdateSolution(exahype2.actionsets.SolverActionSet):
    """Updates one Runge-Kutta stage per grid sweep."""

    _TEMPLATE_TOUCH_FACE_FIRST_TIME = """
  if ({{GUARD}}) {
    ::exahype2::dg::rk::applyBoundaryConditionsAndSolveRiemannProblem<
      {{FULL_QUALIFIED_SOLVER_NAME}},
      {{ "true" if IS_LINEAR else "false" }},
      {{ "true" if USE_RIEMANN_SOLVER else "false" }},
      {{ "true" if USE_FLUX else "false" }},
      {{ "true" if USE_DIFFUSIVE_FLUX else "false" }},
      {{ "true" if USE_NCP else "false" }},
      kernels::{{SOLVER_NAME}}::Quadrature<double>
    >(
      marker,
      repositories::{{SOLVER_INSTANCE}},
      fineGridFace{{SOLVER_NAME}}FaceLabel,
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
      fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.data()
    );
  }
"""

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  if ({{GUARD}}) {
    ::exahype2::dg::rk::updateStage<
      {{FULL_QUALIFIED_SOLVER_NAME}},
      {{ "true" if IS_LINEAR else "false" }},
      {{ "true" if USE_FLUX else "false" }},
      {{ "true" if USE_DIFFUSIVE_FLUX else "false" }},
      {{ "true" if USE_SOURCE else "false" }},
      {{ "true" if USE_NCP else "false" }},
      kernels::{{SOLVER_NAME}}::Quadrature<double>,
      kernels::{{SOLVER_NAME}}::DGMatrices<double>,
      kernels::{{SOLVER_NAME}}::ButcherTableau<double>
    >(
      marker,
      repositories::{{SOLVER_INSTANCE}},
      fineGridCell{{SOLVER_NAME}}CellLabel,
      fineGridFaces{{SOLVER_NAME}}FaceLabel,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution,
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data(),
      [&]([[maybe_unused]] const auto* const oldQ, [[maybe_unused]] auto* const dQdt) {
{%- if POSTPROCESS_UPDATED_CELL_AFTER_RUNGE_KUTTA_STEP %}
        {{POSTPROCESS_UPDATED_CELL_AFTER_RUNGE_KUTTA_STEP | indent(8)}}
{%- endif %}
      },
      [&]([[maybe_unused]] auto* const newQ, [[maybe_unused]] const double timeStamp) {
{%- if POSTPROCESS_UPDATED_CELL_AFTER_FINAL_LINEAR_COMBINATION %}
        {{POSTPROCESS_UPDATED_CELL_AFTER_FINAL_LINEAR_COMBINATION | indent(8)}}
{%- endif %}
      }
    );
  }
"""

    _TEMPLATE_TOUCH_FACE_LAST_TIME = """
  if ({{GUARD}}) {
    ::exahype2::dg::rk::rollOverFaceTimeStamps(repositories::{{SOLVER_INSTANCE}}, fineGridFace{{SOLVER_NAME}}FaceLabel);
  }
"""

    _TEMPLATE_END_TRAVERSAL = """
  repositories::{{SOLVER_INSTANCE}}.reduceLocalUpdates();
"""

    def __init__(self, solver, cell_guard, riemann_solve_guard, face_guard):
        super().__init__(solver)
        self.cell_guard = cell_guard
        self.riemann_solve_guard = riemann_solve_guard
        self.face_guard = face_guard

    def operation_body(self, operation_name):
        if (
            operation_name
            == exahype2.actionsets.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            return self.render(
                self._TEMPLATE_TOUCH_FACE_FIRST_TIME, GUARD=self.riemann_solve_guard
            )
        if (
            operation_name
            == exahype2.actionsets.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            return self.render(
                self._TEMPLATE_TOUCH_CELL_FIRST_TIME, GUARD=self.cell_guard
            )
        if (
            operation_name
            == exahype2.actionsets.ActionSet.OPERATION_TOUCH_FACE_LAST_TIME
        ):
            return self.render(
                self._TEMPLATE_TOUCH_FACE_LAST_TIME, GUARD=self.face_guard
            )
        if operation_name == exahype2.actionsets.ActionSet.OPERATION_END_TRAVERSAL:
            return self.render(self._TEMPLATE_END_TRAVERSAL)
        return ""

    def includes(self):
        return super().includes() + ["exahype2/dg/rk/UpdateStage.h"]
