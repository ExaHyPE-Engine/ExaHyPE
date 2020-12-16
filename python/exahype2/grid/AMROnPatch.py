# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


class AMROnPatch(ActionSet):
  """

 
  
  """
  def __init__(self,patch,functor_implementation,guard,additional_includes):
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

  
  def get_body_of_getGridControlEvents(self):
    return """
  return refinementControl.getGridControlEvents();
""" 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")
  

  def get_body_of_operation(self,operation_name):
    result = ""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = """
  _localRefinementControl.clear();
"""

    if operation_name==peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
      result = """
  refinementControl.merge( _localRefinementControl );
"""
    
    result += super(AMROnPatch,self).get_body_of_operation(operation_name)
    return result


  def get_static_initialisations(self,full_qualified_classname):
    return ""


  def get_attributes(self):
    return """
    ::exahype2::RefinementControl         _localRefinementControl;
"""


  def get_includes(self):
    return """
#include <functional>
""" + self.additional_includes
  
