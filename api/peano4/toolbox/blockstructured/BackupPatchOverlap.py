# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


class BackupPatchOverlap(ActionSet):
  """
  Copy the overlap data of a face into a second face patch.

  patch_overlap_in: peano4.datamodel.Patch
     Face overlap to read from.

  patch_overlap_out: peano4.datamodel.Patch
     Face overlap to write to.

  invoke_in_touch_first: Boolean
    Set this one to True to copy in touchFaceFirstTime, and to False to copy
    in touchFaceLastTime.

  guard: Boolean
    Predicate to determine when the copy is invoked. Pass in true to have it
    called always.
  """

  def __init__(self,patch_overlap_in,patch_overlap_out,invoke_in_touch_first,guard="",additional_includes=""):
    super(BackupPatchOverlap,self).__init__(descend_invocation_order=1)
    self.d = {}
    self.d[ "UNKNOWNS" ]           = str(patch_overlap_in.no_of_unknowns)
    self.d[ "DOFS_PER_AXIS" ]      = str(patch_overlap_in.dim[1])
    self.d[ "OVERLAP" ]            = str(int(patch_overlap_in.dim[0]/2))
    self.d[ "FACES_ACCESSOR_IN" ]  = "fineGridFace"  + patch_overlap_in.name
    self.d[ "FACES_ACCESSOR_OUT" ] = "fineGridFace"  + patch_overlap_out.name
    self.d[ "GUARD_PREDICATE" ]    = guard

    self.invoke_in_touch_first = invoke_in_touch_first
    self._additional_includes  = additional_includes


  def set_guard_predicate(self,guard):
    self.d[ "GUARD_PREDICATE" ]    = guard


  def get_constructor_body(self):
    return ""


  def get_destructor_body(self):
    return ""


  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< ::peano4::grid::GridControlEvent >();\n"


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  __Template = """
    if ({GUARD_PREDICATE}) {{
      int counter = 0;
      peano4::dForEachExclusive({DOFS_PER_AXIS}, 0, 0, [&](const tarch::la::Vector<DIMENSIONS, int>& k) {{
        for (int i = 0; i < {OVERLAP} * 2; i++) {{
          for (int j = 0; j < {UNKNOWNS}; j++) {{
            {FACES_ACCESSOR_OUT}.data()[counter] = {FACES_ACCESSOR_IN}.data()[counter];
            counter++;
          }}
        }}
      }});
    }}
"""


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if self.invoke_in_touch_first and operation_name==ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = self.__Template.format(**self.d)
    if not self.invoke_in_touch_first and operation_name==ActionSet.OPERATION_TOUCH_FACE_LAST_TIME:
      result = self.__Template.format(**self.d)
    return result


  def get_attributes(self):
    return ""


  def get_includes(self):
    return """
#include "peano4/Loop.h"
""" + self._additional_includes
