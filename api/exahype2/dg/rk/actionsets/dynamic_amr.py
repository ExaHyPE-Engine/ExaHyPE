# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import textwrap

import exahype2.actionsets


class AdaptiveMeshFaceCoupling(exahype2.actionsets.SolverActionSet):
    """Restricts the trace of the subfaces below a coarse face onto that face.

    The restricted trace feeds the Riemann problem of the coarse face, and a hanging face
    takes its time stamps from the coarse face.
    """

    _TEMPLATE_TOUCH_FACE_FIRST_TIME = """
{% for predicate_number in range(0, PREDICATES|length) %}
  if ({{PREDICATES[predicate_number]}}) {
    ::exahype2::amr::clearTransferRecord(fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection);
  }
{% endfor %}
"""

    _TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME = """
  ::exahype2::amr::raiseFaceTimeStampsFromCoarseFace(
    coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()),
    fineGridFace{{FACE_LABEL}}
  );
"""

    _TEMPLATE_RESTRICT_HANGING_FACE_TRACE = """
  {{COMPUTE_TIME_STEP_SIZE}}
  ::exahype2::dg::rk::restrictHangingFaceTrace<
    {{FULL_QUALIFIED_SOLVER_NAME}},
    {{ "true" if USE_FLUX else "false" }},
    {{ "true" if USE_DIFFUSIVE_FLUX else "false" }},
    kernels::{{SOLVER_NAME}}::Quadrature<double>,
    kernels::{{SOLVER_NAME}}::DGMatrices<double>
  >(
    marker,
    _projectionRestrictionStage,
    coarseGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(marker.getSelectedFaceNumber()).data(),
    fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
    coarseGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(marker.getSelectedFaceNumber()).data(),
    coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()),
    fineGridFace{{FACE_LABEL}},
    timeStepSize
  );
"""

    _TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME = """
{% for predicate_number in range(0, PREDICATES|length) %}
  if ({{PREDICATES[predicate_number]}}) {
    {{RESTRICT_HANGING_FACE_TRACE | indent(4)}}
  }
{% endfor %}
"""

    _TEMPLATE_TOUCH_CELL_LAST_TIME = ""
    _TEMPLATE_CREATE_FACE = ""
    _TEMPLATE_CREATE_CELL = ""
    _TEMPLATE_DESTROY_CELL = ""

    def __init__(self, solver):
        super().__init__(solver, descend_invocation_order=1)
        self.guards = []

    @property
    def name(self):
        return "exahype2_dg_rk_actionsets_" + type(self).__name__

    def _entries(self):
        labels = {
            "FACE_LABEL": self._solver._face_label.name,
            "CELL_LABEL": self._solver._cell_label.name,
        }
        return {
            **labels,
            "PREDICATES": self._guards,
            "RESTRICT_HANGING_FACE_TRACE": textwrap.dedent(
                self.render(self._TEMPLATE_RESTRICT_HANGING_FACE_TRACE, **labels)
            ).strip(),
        }

    def operation_body(self, operation_name):
        operations = exahype2.actionsets.ActionSet
        template = {
            operations.OPERATION_TOUCH_FACE_FIRST_TIME: self._TEMPLATE_TOUCH_FACE_FIRST_TIME,
            operations.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME: self._TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME,
            operations.OPERATION_TOUCH_HANGING_FACE_LAST_TIME: self._TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME,
            operations.OPERATION_TOUCH_CELL_LAST_TIME: self._TEMPLATE_TOUCH_CELL_LAST_TIME,
            operations.OPERATION_CREATE_FACE: self._TEMPLATE_CREATE_FACE,
            operations.OPERATION_CREATE_CELL: self._TEMPLATE_CREATE_CELL,
            operations.OPERATION_DESTROY_CELL: self._TEMPLATE_DESTROY_CELL,
        }.get(operation_name)
        if template is None:
            return "\n"
        return self.render(template, **self._entries())

    def attributes(self):
        return self.render("""
  ::exahype2::amr::ChildDataStage<double> _projectionRestrictionStage{
    ::exahype2::amr::SubfacesPerFace,
    facedata::{{UNKNOWN_IDENTIFIER}}EstimateProjection::Cardinality / 2
  };
""")

    def includes(self):
        return super().includes() + ["exahype2/dg/rk/DynamicAMR.h"]

    @property
    def guards(self):
        if self._guards == []:
            raise ValueError("The guards are not initialised.")
        return self._guards

    @guards.setter
    def guards(self, new_guards):
        if new_guards != [] and len(new_guards) != self._solver.number_of_stages:
            raise ValueError(
                "One guard per Runge-Kutta step is needed. There are {} steps and the guards are {}.".format(
                    self._solver.number_of_stages, new_guards
                )
            )
        self._guards = new_guards


class DynamicAMRTransfer(AdaptiveMeshFaceCoupling):
    """Moves the solution and the Runge-Kutta stages across a level jump."""

    _TEMPLATE_TOUCH_FACE_FIRST_TIME = """
  ::exahype2::amr::clearTransferRecord(fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection);
"""

    _TEMPLATE_TOUCH_HANGING_FACE_FIRST_TIME = _TEMPLATE_TOUCH_FACE_FIRST_TIME

    _TEMPLATE_TOUCH_HANGING_FACE_LAST_TIME = """
  {
    {{RESTRICT_HANGING_FACE_TRACE | indent(4)}}
  }
"""

    _TEMPLATE_CREATE_FACE = """
  ::exahype2::dg::rk::createFace(
    fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection,
    fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution,
    coarseGridCell{{CELL_LABEL}},
    fineGridFace{{FACE_LABEL}}
  );
"""

    _TEMPLATE_CREATE_CELL = """
  ::exahype2::dg::rk::createCell<
    {{FULL_QUALIFIED_SOLVER_NAME}},
    kernels::{{SOLVER_NAME}}::DGMatrices<double>
  >(
    marker,
    repositories::{{SOLVER_INSTANCE}},
    coarseGridCell{{UNKNOWN_IDENTIFIER}},
    fineGridCell{{UNKNOWN_IDENTIFIER}},
    coarseGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates,
    fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates,
    coarseGridCell{{CELL_LABEL}},
    fineGridCell{{CELL_LABEL}}
  );
"""

    _TEMPLATE_DESTROY_CELL = """
  ::exahype2::dg::rk::destroyCell<
    {{FULL_QUALIFIED_SOLVER_NAME}},
    kernels::{{SOLVER_NAME}}::DGMatrices<double>
  >(
    marker,
    _solutionRestrictionStage,
    _rhsEstimatesRestrictionStage,
    coarseGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    coarseGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data(),
    coarseGridFaces{{FACE_LABEL}},
    coarseGridCell{{CELL_LABEL}},
    fineGridCell{{CELL_LABEL}}
  );
"""

    _TEMPLATE_TOUCH_CELL_LAST_TIME = """
  ::exahype2::dg::rk::projectStageAfterTransfer<
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
    fineGridCell{{CELL_LABEL}},
    fineGridFaces{{FACE_LABEL}},
    fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection,
    fineGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution,
    fineGridCell{{UNKNOWN_IDENTIFIER}}.data(),
    fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data()
  );
"""

    def grid_control_events_body(self):
        return "  return ::exahype2::amr::gridControlEvents();\n"

    def attributes(self):
        return super().attributes() + self.render("""
  ::exahype2::amr::ChildDataStage<double> _solutionRestrictionStage{
    peano4::sfc::ActiveCurve::ChildrenPerCell,
    celldata::{{UNKNOWN_IDENTIFIER}}::Cardinality
  };
  ::exahype2::amr::ChildDataStage<double> _rhsEstimatesRestrictionStage{
    peano4::sfc::ActiveCurve::ChildrenPerCell,
    celldata::{{UNKNOWN_IDENTIFIER}}RhsEstimates::Cardinality
  };
""")

    def includes(self):
        return super().includes() + ["exahype2/dg/rk/DynamicAMR.h"]
