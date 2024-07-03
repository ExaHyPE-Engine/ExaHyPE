# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

class AbstractLimiterActionSet(ActionSet):
  def __init__(self,solver):
    self._solver = solver

  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")

  def user_should_modify_template(self):
    return False

  def get_includes(self):
    return """
#include <functional>
""" + self._solver._get_default_includes() + self._solver.get_user_action_set_includes()
