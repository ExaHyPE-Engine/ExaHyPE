# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2





class DynamicMeshRefinementTrigger(ActionSet):
  """

    AMR criterion based upon the particle density

    This implementation is almsot exactly the same as the toolbox variant,
    but it does not work with a static field, but instead pipes all the 
    refinement events a global variable. 
    
  """
  def __init__(self):
    """
       
    """
    pass


  def user_should_modify_template(self):
    return False




  def get_body_of_operation(self,operation_name):
    result = "\n"
    return result


  def get_body_of_getGridControlEvents(self):
    return """
  return ::swift2::committedGridControlEvents;
"""


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def get_includes(self):
    return """
#include "swift2/GridControlEvents.h"
"""
    