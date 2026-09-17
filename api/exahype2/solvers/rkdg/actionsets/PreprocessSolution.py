# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org

from .AbstractRungeKuttaDGActionSet import AbstractRungeKuttaDGActionSet


import peano4
import jinja2


class EmptyPreprocessSolution(AbstractRungeKuttaDGActionSet):
  def __init__(self,solver):
    super(EmptyPreprocessSolution,self).__init__(solver)
    self.guard               = "true"
    self.descend_invocation_order            = 2


  def get_body_of_operation(self,operation_name):
    result = ""
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


class DoFWisePreprocessSolution(AbstractRungeKuttaDGActionSet):
  """
  Runs a code snippet on every degree of freedom of a cell.

  The snippet plugs into touchCellLastTime(), so an action set a subclass adds
  to touchCellFirstTime() runs before it.
  """
  def __init__(self,solver):
    super(DoFWisePreprocessSolution,self).__init__(solver)
    self.guard               = "true"
    self._compute_kernel     = ""
    self.descend_invocation_order            = 1


  def add_postprocessing_kernel(self, operation_per_point):
    """
    Adds the C++ snippet `operation_per_point`, which runs on every degree of
    freedom and reads its data through `values` and its position through `x`.
    """
    self._compute_kernel += """
{
  {{COMPUTE_TIME_STEP_SIZE}}
  [[maybe_unused]] const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

  if (
    not marker.hasBeenRefined()
    and
    {{PREDICATE}}
  ) {

    constexpr int valuesPerDoF = {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    peano4::dForEach({{DG_ORDER}} + 1, [&](const tarch::la::Vector<DIMENSIONS, int>& index) {
      double* values   = fineGridCell{{UNKNOWN_IDENTIFIER}}.data()
        + peano4::dLinearised(index, {{DG_ORDER}}+1) * valuesPerDoF;
      auto    x        = ::exahype2::dg::getQuadraturePoint(
        marker.getX(), marker.getH(), index, repositories::{{SOLVER_INSTANCE}}.DGOrder, repositories::{{SOLVER_INSTANCE}}.QuadraturePoints1D
      );
      """ + operation_per_point + """
    });
  }
}
"""

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]           = jinja2.Template(self.guard, undefined=jinja2.DebugUndefined).render(**d)
      result = jinja2.Template(self._compute_kernel, undefined=jinja2.DebugUndefined).render(**d)
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_includes(self):
    return super(DoFWisePreprocessSolution,self).get_includes() + """
#include "peano4/Loop.h"
"""


class CellWisePreprocessSolution(AbstractRungeKuttaDGActionSet):
  def __init__(self,solver):
    super(CellWisePreprocessSolution,self).__init__(solver)
    self.guard                            = "true"
    self._compute_kernel                  = ""


  def add_postprocessing_kernel(self, operation_per_cell):
    """
    Adds the C++ snippet `operation_per_cell`, which runs once per cell and
    reads the cell's `timeStamp`.
    """
    self._compute_kernel += """
{
  {{COMPUTE_TIME_STEP_SIZE}}
  [[maybe_unused]] const double timeStamp = fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStamp();

  if (
    not marker.hasBeenRefined()
    and
    {{PREDICATE}}
  ) {
    """ + operation_per_cell + """
  }
}
"""

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)
      d[ "PREDICATE" ]           = jinja2.Template(self.guard, undefined=jinja2.DebugUndefined).render(**d)
      result = jinja2.Template(self._compute_kernel, undefined=jinja2.DebugUndefined).render(**d)
    return result


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
