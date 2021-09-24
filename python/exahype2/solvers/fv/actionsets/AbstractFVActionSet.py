# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org


from peano4.solversteps.ActionSet import ActionSet



class AbstractFVActionSet( ActionSet ):
  def __init__(self,solver):
    """
   
    solver: ADERDG
      Reference to creating class 
   
    """
    self._solver = solver
    pass
  
  
  def get_action_set_name(self):
    """
     
     You should replicate this function in each subclass, so you get 
     meaningful action set names (otherwise, it will be 
     AbstractFVActionSet0,1,2,...).
     
    """
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  def get_includes(self):
    return """
#include <functional>
#include "exahype2/PatchUtils.h"
""" + self._solver._get_default_includes() + self._solver.get_user_action_set_includes() 


