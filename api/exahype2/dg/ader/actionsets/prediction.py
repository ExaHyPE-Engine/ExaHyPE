# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import exahype2.actionsets


class Prediction(exahype2.actionsets.SolverActionSet):
    """Runs the space-time predictor of every cell the guard admits."""

    _TEMPLATE_TOUCH_CELL_FIRST_TIME = """
  if ({{GUARD}}) {
    {{COMPUTE_TIME_STEP_SIZE}}
    ::exahype2::dg::ader::predict<
      {{FULL_QUALIFIED_SOLVER_NAME}},
      {{DG_ORDER}} + 1,
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{"true" if IS_LINEAR else "false"}},
      {{"true" if USE_FLUX else "false"}},
      {{"true" if USE_NCP else "false"}},
      {{"true" if USE_SOURCE else "false"}},
      {{"true" if USE_DIFFUSIVE_FLUX else "false"}},
      {{"true" if USE_DIFFUSIVE_SOURCE else "false"}},
      {{"true" if USE_MATERIAL_PARAMETERS else "false"}},
      {{"true" if USE_POINT_SOURCE else "false"}}
    >(
      marker,
      repositories::{{SOLVER_INSTANCE}},
      fineGridCell{{SOLVER_NAME}}CellLabel,
      fineGridFaces{{SOLVER_NAME}}FaceLabel,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}Estimates,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}FluxEstimates,
      fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
      fineGridCell{{UNKNOWN_IDENTIFIER}}_old.data(),
      {{PREDICTOR_TABLES}},
      timeStepSize
    );
  }
"""

    def __init__(self, solver, guard):
        super().__init__(solver)
        self.guard = guard

    def includes(self):
        return super().includes() + ["exahype2/dg/ader/Predict.h"]

    def operation_body(self, operation_name):
        if (
            operation_name
            == exahype2.actionsets.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            return self.render(self._TEMPLATE_TOUCH_CELL_FIRST_TIME, GUARD=self.guard)
        if operation_name == exahype2.actionsets.ActionSet.OPERATION_END_TRAVERSAL:
            return "  repositories::{}.reduceLocalUpdates();\n".format(
                self.solver.instance_name
            )
        return ""
