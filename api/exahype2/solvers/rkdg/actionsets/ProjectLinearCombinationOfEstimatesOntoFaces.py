# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

import peano4
import jinja2

from exahype2.solvers.ButcherTableau                import ButcherTableau

from enum import Enum


class FaceProjections(Enum):
  """
  Names the quantities a cell projects onto its faces.
  """
  Solution = 1,
  SolutionAndGradient = 2,
  SolutionAndFluxExtrapolation = 3


def compute_number_of_face_projection_quantities(face_projections: FaceProjections):
  """
  Returns how many quantities a face holds for the given projection.
  """
  if face_projections   == FaceProjections.Solution:
    return 1
  elif face_projections == FaceProjections.SolutionAndGradient:
    return 2
  elif face_projections == FaceProjections.SolutionAndFluxExtrapolation:
    return 2
  else:
    assert False, "unsupported face projection variant {}".format( face_projections )
    return -1


class ProjectLinearCombinationOfEstimatesOntoFaces(AbstractRungeKuttaDGActionSet):
  """
  Projects the linear combination of the Runge-Kutta trials onto the faces.

  The projection runs in one grid sweep and the Riemann solve in the next one.
  Each projection sets Updated and UpdatedTimeStamp and clears
  RequiresDataTransfer on the cell's own face side; ComputeFinalLinearCombination
  sets the new and the old time stamp. The creator of the action set sets one
  guard per Runge-Kutta step before the code generation runs.
  """


  TemplateProjectLinearCombinationSolution = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ({{PREDICATES[PREDICATE_NO]}}) {

    double* QIn  = fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data();

    #if DIMENSIONS==2
    ::exahype2::dg::projectVolumetricDataOntoFaces(
      QIn,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft1d,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(0).data(), //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(2).data(), //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(1).data(), //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(3).data()  //up
    );
    #elif DIMENSIONS==3
    ::exahype2::dg::projectVolumetricDataOntoFaces(
      QIn,
      {{ORDER}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      repositories::{{SOLVER_INSTANCE}}.BasisFunctionValuesLeft1d,
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(0).data(), //left
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(3).data(), //right
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(1).data(), //down
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(4).data(), //up
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(2).data(), //front
      fineGridFaces{{UNKNOWN_IDENTIFIER}}EstimateProjection(5).data()  //back
    );
    #endif


    // The snippet below declares and sets double timeStepSize.
    const double timeStampOldSolution    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();
    {{COMPUTE_TIME_STEP_SIZE}}
    const double timeStamp = timeStampOldSolution + {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize;

    for (int axis=0; axis<DIMENSIONS; axis++) {
      {{FACE_METADATA_ACCESSOR}}(axis).setUpdated(           1,true);
      {{FACE_METADATA_ACCESSOR}}(axis+DIMENSIONS).setUpdated(0,true);

      {{FACE_METADATA_ACCESSOR}}(axis).setUpdatedTimeStamp(           1, timeStamp );
      {{FACE_METADATA_ACCESSOR}}(axis+DIMENSIONS).setUpdatedTimeStamp(0, timeStamp );

      // The projection writes this cell's halves of the face estimates.
      {{FACE_METADATA_ACCESSOR}}(axis).setRequiresDataTransfer(           1, false );
      {{FACE_METADATA_ACCESSOR}}(axis+DIMENSIONS).setRequiresDataTransfer(0, false );
    }
  }
  {% endfor %}
"""


  def __init__(self,
               solver,
               face_projections: FaceProjections):
    super(ProjectLinearCombinationOfEstimatesOntoFaces,self).__init__(solver)
    self.guards            = []
    self.face_projections  = face_projections
    self._butcher_tableau = ButcherTableau(self._solver._rk_order)


  @property
  def guards(self):
    if self._guards==[]:
      raise Exception( "Guards are not initialised" )
    return self._guards


  @guards.setter
  def guards(self,new_guards):
    if new_guards!=[] and len(new_guards)!=self._solver.number_of_Runge_Kutta_steps():
      raise Exception( "Expect one guard per Runge Kutta step. Have {} steps but got guards {}".format(self._solver.number_of_Runge_Kutta_steps(),new_guards) )
    self._guards = new_guards


  def get_body_of_operation(self,operation_name):
    result = ""
    d = {}
    self._solver._init_dictionary_with_default_parameters(d)
    self._solver.add_entries_to_text_replacement_dictionary(d)
    d[ "PREDICATES" ]                               = self.guards
    d[ "FACE_METADATA_ACCESSOR" ]   = "fineGridFaces"  + self._solver._face_label.name
    d[ "CELL_METADATA_ACCESSOR" ]   = "fineGridCell" + self._solver._cell_label.name
    d[ "WEIGHTS" ]                  = self._butcher_tableau.final_estimate_weights()
    d[ "BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES" ] = self._butcher_tableau.time_step_sizes()
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME and self.face_projections==FaceProjections.Solution:
      result = jinja2.Template(self.TemplateProjectLinearCombinationSolution).render(**d)
    elif operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      assert False, "face projection variant {} not supported yet ".format( self.face_projections)

    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
