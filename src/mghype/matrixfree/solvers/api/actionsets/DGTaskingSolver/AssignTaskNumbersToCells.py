from peano4.solversteps.ActionSet import ActionSet

import peano4
import jinja2


class AssignTaskNumbersToCells(ActionSet):
  """!
  We enumerate our existing cells. No need to
  support anything too complicated yet.

  We give the class a static tarch::Enumerator object

  """
  def __init__(self, solver, tasks_per_cell=1, descend_invocation_order=0, parallel=True):
    super(AssignTaskNumbersToCells, self).__init__(descend_invocation_order, parallel)
    self._tasks_per_cell = tasks_per_cell

    self._d = {}
    self._d["TASKS_PER_CELL"]  = tasks_per_cell
    self._d["SOLVER_INSTANCE"] = solver.instance_name()
    self._d["SOLVER_NAME"]     = solver.typename()
  
  def get_body_of_operation(self, operation_name):
    if operation_name == ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      return jinja2.Template("""
  for (int i=0; i<{{TASKS_PER_CELL}}; i++) {
    fineGridCell{{SOLVER_NAME}}.setTaskNumber( i, _enum.getNumber() );
  }
  """).render( **self._d )

    else:
      return ""
        
  def get_action_set_name(self):
      return __name__.replace(".py", "").replace(".", "_")

  def user_should_modify_template(self):
      return False

  def get_includes(self):
      return """
#include "tarch/Enumerator.h"
"""

  def get_attributes(self):
    return """
  static tarch::Enumerator _enum;
"""

  def get_static_initialisations(self, full_qualified_classname):
    return f"""
tarch::Enumerator {full_qualified_classname}::_enum;
"""

