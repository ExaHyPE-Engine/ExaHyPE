# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

from exahype2.solvers.ButcherTableau import ButcherTableau

import peano4
import jinja2


class SolveRiemannProblem(AbstractRungeKuttaDGActionSet):
  """
  Solve the Riemann problem and add its contribution back to the solution.

  See RungeKuttaDG for a discussion of the individual steps.
  """

  TemplateProject = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ({{PREDICATES[PREDICATE_NO]}}) {
    // Local time stepping and subcycling are not supported, so both sides of the
    // face carry the same time stamp and either one can be read.
    const double timeStampOldSolution = {{FACE_METADATA_ACCESSOR}}.getOldTimeStamp(0);

    // The snippet below declares and sets double timeStepSize.
    {{COMPUTE_TIME_STEP_SIZE}}

    const double timeStamp = timeStampOldSolution + {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize;

    assertion( tarch::la::greaterEquals( timeStepSize, 0.0 ),  timeStamp, timeStepSize, timeStampOldSolution, {{FACE_METADATA_ACCESSOR}}.toString(), marker.toString() );
    assertion( tarch::la::greaterEquals( timeStamp, 0.0 ),     timeStamp, timeStepSize, timeStampOldSolution, {{FACE_METADATA_ACCESSOR}}.toString(), marker.toString() );

    const double* NOALIAS QIn  = fineGridFace{{UNKNOWN_IDENTIFIER}}EstimateProjection.data();
    double* NOALIAS       QOut = fineGridFace{{UNKNOWN_IDENTIFIER}}RiemannSolution.data();

    ::exahype2::dg::{{KERNEL_NAMESPACE}}::{{RIEMANN_COMPUTE_KERNEL_CALL}}
  }
  {% endfor %}
"""


  def __init__(self,solver):
    """
    Construct the action set for the Riemann solve.

    :: Arguments

    solver: RungeKuttaDG
      Reference to creating class.
    """
    super(SolveRiemannProblem,self).__init__(solver)
    self.guards            = []
    self._butcher_tableau     = ButcherTableau(self._solver._rk_order)


  @property
  def guards(self):
    if self._guards==[]:
      raise Exception("Guards are not initialised")
    return self._guards


  @guards.setter
  def guards(self,new_guards):
    if new_guards!=[] and len(new_guards)!=self._solver.number_of_Runge_Kutta_steps():
      raise Exception( "Expect one guard per Runge-Kutta step. There are {} steps, but the guards are {}".format(self._solver.number_of_Runge_Kutta_steps(),new_guards) )
    self._guards = new_guards


  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATES" ]            = self.guards
      d[ "BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES" ] = self._butcher_tableau.time_step_sizes()
      d[ "FACE_METADATA_ACCESSOR" ] = "fineGridFace"  + self._solver._face_label.name
      result = jinja2.Template(self.TemplateProject).render(**d)
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_includes(self):
    return super(SolveRiemannProblem,self).get_includes() + """
#include "exahype2/dg/rusanov/Rusanov.h"
"""
