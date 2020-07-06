# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


class ApplyFunctorOnPatch(ActionSet):
  """
  
  This class assumes that you have NxNxN patch within your block. All it does
  is that it takes the particular patch data and offers it to a functor, so 
  you can alter the patch data content.  
  
  """
  
  
  def __init__(self,patch,functor_implementation,guard,additional_includes):
    """

  patch          Instance of peano4.datamodel.Patch
  functor_implementation Plain C++ code
  
  
  <h2> functor_implementation </h2> 
  
  The functor implementation is a plain C/C++ 
  
  - If you want to use brackets in your implementation, please use double brackets {{ }} as 
    the template system otherwise gets confused.
  - The following C++ variables are defined:
  
    patchData
    
    It is a plain double pointer.
   
    """
    self.d = {}
    if patch.dim[0] != patch.dim[1]:
      print( "Error: Can only handle square patches." )
      assert( patch.dim[0] == patch.dim[1] )
      
    self.d[ "UNKNOWNS" ]           = str(patch.no_of_unknowns)
    self.d[ "DOFS_PER_AXIS" ]      = str(patch.dim[0])
    self.d[ "NUMBER_OF_DOUBLE_VALUES_IN_ORIGINAL_PATCH_2D" ] = str(patch.no_of_unknowns * patch.dim[0] * patch.dim[0])
    self.d[ "NUMBER_OF_DOUBLE_VALUES_IN_ORIGINAL_PATCH_3D" ] = str(patch.no_of_unknowns * patch.dim[0] * patch.dim[0] * patch.dim[0])
    self.d[ "CELL_ACCESSOR" ]                                = "fineGridCell" + patch.name
    self.d[ "FUNCTOR_IMPLEMENTATION" ]                       = functor_implementation
    self.d[ "GUARD" ]                                        = guard
    
    self.additional_includes = additional_includes


  def get_constructor_body(self):
    return ""

  
  def get_destructor_body(self):
    return ""


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  __Template_TouchCellFirstTime = """
  if ({GUARD}) {{
    logTraceIn( "touchCellFirstTime(...)" );
    double* patchData = {CELL_ACCESSOR}.value;
    {FUNCTOR_IMPLEMENTATION}
    logTraceOut( "touchCellFirstTime(...)" );
  }}
"""


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = self.__Template_TouchCellFirstTime.format(**self.d)
      pass 
    return result


  def get_includes(self):
    return """
#include <functional>
""" + self.additional_includes
