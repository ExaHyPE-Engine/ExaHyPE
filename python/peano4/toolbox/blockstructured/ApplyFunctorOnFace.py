# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet



class ApplyFunctorOnFace(ActionSet):
  """
  
  This class plugs into the first access to a face and copies the data over
  
  patch_overlap_in: peano4.datamodel.Patch
     Patch data associated with cell.
        
  patch_overlap_out: peano4.datamodel.Patch
     Patch data associated with faces.
  
  invoke_in_touch_first: Boolean
    If you set this one to True, then I do the copying in touchFaceFirstTime. 
    Otherwise, I do it in touchFaceLastTime
    
  guard: Boolean 
    Predicate to determine when to invoke this thing. Pass in
    true if you wanna be it called always
  
  """
  
  
  def __init__(self,patch_on_face,functor_implementation,guard,additional_includes,invoke_in_touch_first=True):
    self.d = {}
    self.d[ "UNKNOWNS" ]           = str(patch_on_face.no_of_unknowns)
    self.d[ "DOFS_PER_AXIS" ]      = str(patch_on_face.dim[1])
    self.d[ "OVERLAP" ]            = str(patch_on_face.dim[0]/2)
    self.d[ "GUARD_PREDICATE" ]    = guard
    
    self.d[ "FUNCTOR_IMPLEMENTATION" ]    = functor_implementation
    
    self.invoke_in_touch_first = invoke_in_touch_first
    self.additional_includes    = additional_includes


  def set_guard_predicate(self,guard):
    self.d[ "GUARD_PREDICATE" ]    = guard
    

  def get_constructor_body(self):
    return ""

  
  def get_destructor_body(self):
    return ""


  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n" 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  __Template = """
    if ({GUARD_PREDICATE}) {{
      {FUNCTOR_IMPLEMENTATION}
    }}
"""


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if self.invoke_in_touch_first and operation_name==ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = self.__Template.format(**self.d)
      pass 
    if not self.invoke_in_touch_first and operation_name==ActionSet.OPERATION_TOUCH_FACE_LAST_TIME:
      result = self.__Template.format(**self.d)
      pass 
    return result


  def get_attributes(self):
    return ""


  def get_includes(self):
    return """
#include <functional>
""" + self.additional_includes
