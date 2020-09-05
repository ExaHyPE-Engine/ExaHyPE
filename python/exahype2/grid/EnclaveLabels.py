# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import dastgen2
import peano4.datamodel.DaStGen2


def create_enclave_cell_label(solver_name):
  """
  
   solver_name: string
     Name of the solver
     
  """
  result = peano4.datamodel.DaStGen2( EnclaveLabels.get_attribute_name( solver_name ) )
  result.data.add_attribute( dastgen2.attributes.Integer("SemaphoreNumber") )
  return result


class EnclaveLabels(ActionSet):
  def get_attribute_name(solver_name):
    return solver_name + "CellSemaphoreLabel"

  """
  
   SetLabels is an action set which is automatically merged into all ExaHyPE2
   steps by the project. There's nothing for the user solvers to be done here.
  
  """
  def __init__(self, solver_name):
    self._solver_name = solver_name
    pass


  def get_constructor_body(self):
    return ""

    
  def get_destructor_body(self):
    return ""


  #def get_body_of_getGridControlEvents(self):
  #  return self.__Template_GridControlEvents.format(**self.d)


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_CREATE_CELL:
      result += """
  fineGridCell""" + EnclaveLabels.get_attribute_name(self._solver_name) + """.setSemaphoreNumber(  ::exahype2::EnclaveBookkeeping::NoEnclaveTaskNumber );
"""

    return result


  def get_attributes(self):
    return ""


  def get_includes(self):
    return """
#include "Constants.h"
#include "exahype2/EnclaveBookkeeping.h"
"""    

