# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .ActionSet import ActionSet


class UserActionSet(ActionSet):
  def __init__(self,name,descend_invocation_order = 0):
    super(UserActionSet,self).__init__(descend_invocation_order)
    self.name = name
    pass


  def user_should_modify_template(self):
    return True


  def get_constructor_body(self):
    return "// @todo Please implement\n"


  def get_destructor_body(self):
    return "// @todo Please implement\n"


  def get_body_of_getGridControlEvents(self):
    return "// @todo Please implement\nreturn std::vector< ::peano4::grid::GridControlEvent >();\n"


  def get_body_of_operation(self,operation_name):
    return """
// @todo Please implement
"""


  def get_action_set_name(self):
    """
     Returns a description (word) for the mapping which is also used as class name
     for the generated type. As a consequence, the result should be one word (if
     possible) and uppercase. Also, every subclass should overwrite this routine.
    """
    return self.name


  def get_attributes(self):
    return "// @todo Please implement\n"


  def get_includes(self):
    return "// @todo Please implement\n"
