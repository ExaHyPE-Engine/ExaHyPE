# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet
from .ProjectLinearCombinationOfEstimatesOntoFaces import FaceProjections
from .ProjectLinearCombinationOfEstimatesOntoFaces import (
    compute_number_of_face_projection_quantities,
)

from exahype2.solvers.ButcherTableau import ButcherTableau

import peano4
import jinja2


class ComputeFinalLinearCombination(AbstractRungeKuttaDGActionSet):
    """!

    Action set determining the final solution using a linear combination of previous Runge-Kutta guesses

    We solve the ODE

    @f$ \partial Q = F(Q) @f$

    where we have multiple guesses for F according to the Butcher tableau. The final
    solution can be written down as linear combination over these guesses:


    @f$ Q^{new} = Q^{old} + a_1 \cdot dt \cdot F_1(Q) + a_2 \cdot dt \cdot F_2(Q) + \dots @f$

    We observe the following simple facts:

    - The data in fineGridCell{{UNKNOWN_IDENTIFIER}}.value holds @f$ Q^{old} @f$
    - The data in fineGridCell{{UNKNOWN_IDENTIFIER}}.value shall hold @f$ Q^{new} @f$ eventually.
      We thus can simply add the scaled estimated to it.
    - The estimates do not alter the auxiliary variables.
    - We can thus just keep the previous auxiliary variables implicitly.


    ## Face projections

    As I discuss in detail in solvers.rkdg.RungeKuttaDG.create_data_structures(),
    the field _estimate_projection usually holds the projection of a DG estimate,
    but not the real solution. At some points, I however need the real solution
    after a time step. Local time stepping for example depends upon it. So the
    final combination projects the solution again onto the faces.

    In return, we could skip the projection within the first Runge-Kutta step
    where we effectively take the data and write it to the faces once again.
    But this is too much of an optimisation, I just retrigger the projection
    there agin.

    """

    TemplateProject = """
  if ({{PREDICATE}}) {
    {{COMPUTE_TIME_STEP_SIZE}}

    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithAuxiliaryVariables   ( 1,            {{ORDER}}+1, 0, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}});
    ::exahype2::enumerator::AoSLexicographicEnumerator enumeratorWithoutAuxiliaryVariables( {{RK_STEPS}}, {{ORDER}}+1, 0, {{NUMBER_OF_UNKNOWNS}}, 0 );
    
    dfor( dof, {{ORDER}}+1 ) {
      for (int unknown=0; unknown<{{NUMBER_OF_UNKNOWNS}}; unknown++) {
        {% for WEIGHT_NO in range(0,WEIGHTS|length) %}
        fineGridCell{{UNKNOWN_IDENTIFIER}}.value[ enumeratorWithAuxiliaryVariables(0,dof,unknown) ] += 
          timeStepSize * {{WEIGHTS[WEIGHT_NO]}} * fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.value[ enumeratorWithoutAuxiliaryVariables({{WEIGHT_NO}},dof,unknown) ];
        {% endfor %}
      }
    }
    

    const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

    double* newQ = fineGridCell{{UNKNOWN_IDENTIFIER}}.value;
    
    {{POSTPROCESS_UPDATED_CELL_AFTER_FINAL_LINEAR_COMBINATION}}

    {% if COMPUTE_MAX_EIGENVALUE %}
    ::exahype2::CellData  cellData( nullptr, marker.x(), marker.h(), timeStamp, timeStepSize, fineGridCell{{UNKNOWN_IDENTIFIER}}.value );

    ::exahype2::dg::reduceMaxEigenvalue_patchwise_functors(
      cellData,
      {{ORDER}},                                                   
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      [&](
        const double * __restrict__ Q,
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       t,
        double                                       dt,
        int                                          normal
      )->double {
        return repositories::{{SOLVER_INSTANCE}}.maxEigenvalue( Q, x, t, dt, normal );
      },
      repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1d
    );

    const double maxEigenvalue = cellData.maxEigenvalue[0];
    {% endif %}
        
    // Set the variable
    // double newTimeStepSize
    {{COMPUTE_NEW_TIME_STEP_SIZE}}
     
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStamp(timeStamp+timeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setTimeStepSize(newTimeStepSize);
    fineGridCell{{SOLVER_NAME}}CellLabel.setHasUpdated(true);
    repositories::{{SOLVER_INSTANCE}}.update(newTimeStepSize, timeStamp+timeStepSize, marker.h()(0) );
    
    {% if NUMBER_OF_FACE_PROJECTIONS!=1 %}
    #error Have to tweak compute final projection
    {% endif %}
    
    #if Dimensions==2
    ::exahype2::dg::projectVolumetricDataOntoFaces(
      newQ,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft1d,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(0).value, //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(2).value, //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(1).value, //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(3).value  //up
    );
    #elif Dimensions==3
    ::exahype2::dg::projectVolumetricDataOntoFaces(
      newQ,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft1d,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(0).value, //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(3).value, //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(1).value, //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(4).value, //up
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(2).value, //front
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(5).value  //back
    );
    #endif
  }
"""

    TemplateUpdateFace = """
  if ( repositories::{{SOLVER_INSTANCE}}.isLastGridSweepOfTimeStep() ) {
    for (int leftRight=0; leftRight<2; leftRight++) {
      if ( fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdated(leftRight) ) {
        fineGridFace{{SOLVER_NAME}}FaceLabel.setOldTimeStamp( leftRight, fineGridFace{{SOLVER_NAME}}FaceLabel.getNewTimeStamp(leftRight) );
        fineGridFace{{SOLVER_NAME}}FaceLabel.setNewTimeStamp( leftRight, fineGridFace{{SOLVER_NAME}}FaceLabel.getUpdatedTimeStamp(leftRight) );
        
        ::exahype2::dg::copyOneSideOfFaceProjection(
          {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}},
          {{ORDER}},
          {{NUMBER_OF_FACE_PROJECTIONS}},
          marker.getSelectedFaceNumber() % Dimensions,
          leftRight==1,
          fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.value,
          fineGridFace{{UNKNOWN_IDENTIFIER}}OldSolutionProjection.value
        ); 
      }
    }
  }
"""

    def __init__(self, solver, guard, face_projections: FaceProjections):
        """

        guard_project: String (C++ code)
          Predicate which controls if the solution is actually projected

        guard_safe_old_time_step: String (C++ code)
          Predicate which controls if the projection should be copied into
          the old solution and the time step should also be moved over

        """
        super(ComputeFinalLinearCombination, self).__init__(solver)
        self.guard = guard
        self._butcher_tableau = ButcherTableau(self._solver._rk_order)
        self._face_projections = face_projections
        assert self._face_projections == FaceProjections.Solution, "not supported"

    def get_body_of_operation(self, operation_name):
        result = ""
        d = {}
        self._solver._init_dictionary_with_default_parameters(d)
        self._solver.add_entries_to_text_replacement_dictionary(d)
        d["PREDICATE"] = self.guard
        d["WEIGHTS"] = self._butcher_tableau.final_estimate_weights()
        d["NUMBER_OF_FACE_PROJECTIONS"] = compute_number_of_face_projection_quantities(
            self._face_projections
        )
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME
        ):
            result = jinja2.Template(self.TemplateProject).render(**d)
            pass
        if (
            operation_name
            == peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_LAST_TIME
        ):
            result = jinja2.Template(self.TemplateUpdateFace).render(**d)
            pass
        return result

    def get_action_set_name(self):
        return __name__.replace(".py", "").replace(".", "_")

    def get_includes(self):
        return (
            super(ComputeFinalLinearCombination, self).get_includes()
            + """
#include "exahype2/enumerator/AoSLexicographicEnumerator.h"
"""
        )
