# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import jinja2

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet
from .ProjectLinearCombinationOfEstimatesOntoFaces import FaceProjections
from .ProjectLinearCombinationOfEstimatesOntoFaces import (
    compute_number_of_face_projection_quantities,
)


class DynamicAMR(AbstractRungeKuttaDGActionSet):
    """
    Default DG interpolation/restriction action set

    There is a detailed description of the DG AMR semantics in the exahype2 C++
    directory. Logically, any AMR mesh transition translates into a Riemann
    problem. The documentation hence is stored in the Riemann.h file.
    """

    def __init__(self, solver, face_projections: FaceProjections):
        """
        The guards are those for the projection onto the faces.
        """
        super(DynamicAMR, self).__init__(solver)
        self.guards = []
        self.face_projections = face_projections

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(DynamicAMR, self).get_includes()
            + """
"""
        )

    __Template_TouchFaceFirstTime = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ( {{PREDICATES[PREDICATE_NO]}} ) {
    ::exahype2::dg::clearSolutionProjection(
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      {{NUMBER_OF_PROJECTED_QUANTITIES}},
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data()
    );
  }
  {% endfor %}
"""

    __Template_TouchHangingFaceFirstTime = """
  ::exahype2::dg::interpolateRiemannSolution(
    marker,
    {{ORDER}},
    {{NUMBER_OF_UNKNOWNS}},
    repositories::{{SOLVER_INSTANCE}}.InterpolationMatrix1d,
    coarseGridFaces{{UNKNOWN_IDENTIFIER}}RiemannSolution(marker.getSelectedFaceNumber()).data(),
    fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.data()
  );

  fineGridFace{{FACE_LABEL}}.setNewTimeStamp(0, std::max( coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).getNewTimeStamp(0), fineGridFace{{FACE_LABEL}}.getNewTimeStamp(0)) );
  fineGridFace{{FACE_LABEL}}.setNewTimeStamp(1, std::max( coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).getNewTimeStamp(1), fineGridFace{{FACE_LABEL}}.getNewTimeStamp(1)) );
  fineGridFace{{FACE_LABEL}}.setOldTimeStamp(0, std::max( coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).getOldTimeStamp(0), fineGridFace{{FACE_LABEL}}.getOldTimeStamp(0)) );
  fineGridFace{{FACE_LABEL}}.setOldTimeStamp(1, std::max( coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).getOldTimeStamp(1), fineGridFace{{FACE_LABEL}}.getOldTimeStamp(1)) );
"""

    __Template_TouchHangingFaceLastTime = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ( {{PREDICATES[PREDICATE_NO]}} ) {
    ::exahype2::dg::restrictAndAccumulateProjectedFacePolynomial(
      marker,
      {{ORDER}},
      {{NUMBER_OF_PROJECTED_QUANTITIES}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.RestrictionMatrix1d,
      fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data(),
      coarseGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(marker.getSelectedFaceNumber()).data()
    );

    bool isLeftEntryOnCoarseFaceLabel = marker.getSelectedFaceNumber() >= DIMENSIONS;
    coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).setUpdated( isLeftEntryOnCoarseFaceLabel ? 0 : 1,true);
    coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).setUpdatedTimeStamp( 
      isLeftEntryOnCoarseFaceLabel ? 0 : 1,
      std::max( 
        coarseGridFaces{{FACE_LABEL}}(marker.getSelectedFaceNumber()).getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 ), 
        fineGridFace{{FACE_LABEL}}.getUpdatedTimeStamp( isLeftEntryOnCoarseFaceLabel ? 0 : 1 )
      )
    );
  }
  {% endfor %}
"""

    def get_body_of_operation(self, operation_name):
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        d["PREDICATES"] = self.guards
        d["FACE_LABEL"] = self._solver._face_label.name
        d["NUMBER_OF_PROJECTED_QUANTITIES"] = (
            compute_number_of_face_projection_quantities(self.face_projections)
        )

        result = "\n"
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_FIRST_TIME
        ):
            result = jinja2.Template(self.__Template_TouchHangingFaceFirstTime).render(
                **d
            )
            pass
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_HANGING_FACE_LAST_TIME
        ):
            result = jinja2.Template(self.__Template_TouchHangingFaceLastTime).render(
                **d
            )
            pass
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME
        ):
            result = jinja2.Template(self.__Template_TouchFaceFirstTime).render(**d)
            pass
        return result

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
                "Expect one guard per Runge Kutta step. Have {} steps but got guards {}".format(
                    self._solver.number_of_Runge_Kutta_steps(), new_guards
                )
            )
        self._guards = new_guards
