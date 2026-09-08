# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

from exahype2.solvers.ButcherTableau import ButcherTableau

import peano4
import jinja2


class HandleBoundary(AbstractRungeKuttaDGActionSet):
    """
    Applies the boundary conditions along the faces flagged as boundary.

    One grid sweep projects the linear combination of the Runge-Kutta trials
    onto the faces, and the next sweep solves the Riemann problems there.
    """

    TemplateHandleBoundary_Prologue = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if (
    {{PREDICATES[PREDICATE_NO]}}
    and
    not repositories::{{SOLVER_INSTANCE}}.PeriodicBC[marker.getSelectedFaceNumber()%DIMENSIONS]
    and
    not marker.hasBeenRefined()
    and
    fineGridFace{{SOLVER_NAME}}FaceLabel.getBoundary()
  ) {
    const double timeStampOldSolution = fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp()(0);

    // The snippet below declares and sets double timeStepSize.
    {{COMPUTE_TIME_STEP_SIZE}}

    const double timeStamp = timeStampOldSolution + {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize;

    assertion( tarch::la::greaterEquals( timeStepSize, 0.0 ),  timeStamp, timeStepSize, timeStampOldSolution, marker.toString(), fineGridFace{{SOLVER_NAME}}FaceLabel.toString() );
    assertion( tarch::la::greaterEquals( timeStamp, 0.0 ),     timeStamp, timeStepSize, timeStampOldSolution, marker.toString(), fineGridFace{{SOLVER_NAME}}FaceLabel.toString() );

"""

    TemplateHandleBoundary_KernelCalls = """
    ::exahype2::dg::applyBoundaryConditions(
      [&](
        const double* const NOALIAS                  Qinside,
        double* const NOALIAS                        Qoutside,
        const tarch::la::Vector<DIMENSIONS,double>&  x,
        const double                                 t,
        const double                                 dt,
        const int                                    normal
      ) -> void {
        repositories::{{SOLVER_INSTANCE}}.boundaryConditions( Qinside, Qoutside, x, t, normal );
      },
      marker.getX(),
      marker.getH(),
      timeStamp,
      repositories::{{SOLVER_INSTANCE}}.getMinTimeStepSize(),
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      marker.getSelectedFaceNumber(),
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1D,
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data()
    );
"""

    TemplateHandleBoundary_Epilogue = """

    if (marker.getSelectedFaceNumber()<DIMENSIONS) {
      fineGridFace{{SOLVER_NAME}}FaceLabel.setUpdatedTimeStamp(0, fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdatedTimeStamp(1) );
      fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp    (0, fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(1) );
      fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp    (0, fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(1) );
    }
    else {
      fineGridFace{{SOLVER_NAME}}FaceLabel.setUpdatedTimeStamp(1, fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdatedTimeStamp(0) );
      fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp(    1, fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(0) );
      fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp(    1, fineGridFace{{SOLVER_NAME}}FaceLabel.getOldTimeStamp(0) );
    }
  }
  {% endfor %}
"""

    def __init__(self, solver, guard):
        """
        `guard` is not evaluated; the predicates come from the guards property.
        """
        super(HandleBoundary, self).__init__(solver)
        self.guards = []
        self._butcher_tableau = ButcherTableau(self._solver._rk_order)

    @property
    def guards(self):
        if self._guards == []:
            raise Exception("Guards are not initialised")
        return self._guards

    @guards.setter
    def guards(self, new_guards):
        if (
            new_guards != []
            and len(new_guards) != self._solver.number_of_Runge_Kutta_steps()
        ):
            raise Exception(
                "Expect one guard per Runge-Kutta step. There are {} steps, but the guards are {}".format(
                    self._solver.number_of_Runge_Kutta_steps(), new_guards
                )
            )
        self._guards = new_guards

    def get_body_of_operation(self, operation_name):
        result = ""
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            d = {}
            self._solver._init_dictionary_with_default_parameters(d)
            self._solver.add_entries_to_text_replacement_dictionary(d)
            d["PREDICATES"] = self.guards
            d["BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES"] = (
                self._butcher_tableau.time_step_sizes()
            )
            result = jinja2.Template(
                self.TemplateHandleBoundary_Prologue
                + self.TemplateHandleBoundary_KernelCalls
                + self.TemplateHandleBoundary_Epilogue
            ).render(**d)
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")
