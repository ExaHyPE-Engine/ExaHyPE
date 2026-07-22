# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet

from exahype2.solvers.ButcherTableau                import ButcherTableau

import peano4
import jinja2


class SolveVolumeIntegral(AbstractRungeKuttaDGActionSet):
  """
  Solve volume integral.

  Solve the volumetric part of the DG method. This includes the evaluation of the
  volume integral of the operator multiplied with a trial function, plus the
  subsequent multiplication with an inverted (lumped) mass matrix. So, eventually,
  we do an explicit Euler step subject to a relaxed time step size from the Butcher
  tableau. The input is already properly computed (see LinearCombinationOfEstimates()).

  The volume integral definition is an underdetermined action set, i.e., you still
  have to ensure that a number of macros are defined when you use it:

  - COMPUTE_TIME_STEP_SIZE has to specify a double timeStepSize. It may be const if
    you want. The action set does not alter it.

  In return, the following variables will be set by the action set:

  - timeStampOldSolution: double This is the time stamp of the original
    solution, i.e. not the one we are currently evaluating due to the
    Runge-Kutta scheme.
  - timeStamp: double This is the time stamp of the current Runge-Kutta
    guess. It is timeStampOldSolution plus the weight of the Butcher scheme
    times the time step size.
  - QIn: double* A pointer to the linear combination of previous
    guesses according to the Butcher tableau.
  - QOut: For Runge-Kutta, we have to store all guesses of the future solution,
    so we can eventually combine them into one final outcome. QOut points to
    the segment that is to be computed in this step.
  """

  TemplateCellKernel = """
  {% for PREDICATE_NO in range(0,PREDICATES|length) %}
  if ({{PREDICATES[PREDICATE_NO]}}) {
    const double timeStampOldSolution    = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

    // The snippet below declares and sets double timeStepSize.
    {{COMPUTE_TIME_STEP_SIZE}}

    const double timeStamp = timeStampOldSolution + {{BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES[PREDICATE_NO]}} * timeStepSize;

    assertion( tarch::la::greaterEquals( timeStepSize, 0.0 ),  timeStamp, timeStepSize, timeStampOldSolution );
    assertion( tarch::la::greaterEquals( timeStamp, 0.0 ),     timeStamp, timeStepSize, timeStampOldSolution );

    double* QIn  = fineGridCell{{UNKNOWN_IDENTIFIER}}LinearCombination.data();
    #if DIMENSIONS==2
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data() + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_2D}} * {{NUMBER_OF_UNKNOWNS}};
    #elif DIMENSIONS==3
    double* QOut = fineGridCell{{UNKNOWN_IDENTIFIER}}RhsEstimates.data() + {{PREDICATE_NO}} * {{NUMBER_OF_DOFS_PER_CELL_3D}} * {{NUMBER_OF_UNKNOWNS}};
    #endif

    ::exahype2::CellData cellData(
      QIn,
      marker.getX(),
      marker.getH(),
      timeStamp,
      timeStepSize,
      QOut
    );

    {% if STATELESS_PDE_TERMS %}
    if (repositories::{{SOLVER_INSTANCE}}.cellCanUseStatelessPDETerms(
      marker.getX(),
      marker.getH(),
      timeStamp,
      timeStepSize
    )) {
      ::exahype2::dg::{{VOLUMETRIC_COMPUTE_KERNEL_CALL_STATELESS}}
    } else
    {% endif %}

    ::exahype2::dg::{{VOLUMETRIC_COMPUTE_KERNEL_CALL}}
  }
  {% endfor %}
"""


  def __init__(self,solver):
    """
    Construct the action set for the volume integral.

    :: Arguments

    solver: RungeKuttaDG
      Reference to creating class.
    """
    super(SolveVolumeIntegral,self).__init__(solver)
    self.guards                      = []
    self._butcher_tableau            = ButcherTableau(self._solver._rk_order)


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
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATES" ]                               = self.guards
      d[ "BUTCHER_TABLEAU_RELATIVE_TIME_STEP_SIZES" ] = self._butcher_tableau.time_step_sizes()
      result = jinja2.Template(self.TemplateCellKernel).render(**d)
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_includes(self):
    return super( SolveVolumeIntegral, self ).get_includes()
