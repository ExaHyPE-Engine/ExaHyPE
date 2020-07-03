# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import dastgen2
import peano4.datamodel.DaStGen2


def create_face_label():
  result = peano4.datamodel.DaStGen2( SetLabels.AttributeName )
  result.data.add_attribute( dastgen2.attributes.Boolean("Boundary") )
  return result


class SetLabels(ActionSet):
  AttributeName = "Label"
  
  """
  
   SetLabels is an action set which is automatically merged into all ExaHyPE2
   steps by the project. There's nothing for the user solvers to be done here.
  
  """
  def __init__(self):
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
    if operation_name==ActionSet.OPERATION_CREATE_PERSISTENT_FACE or operation_name==ActionSet.OPERATION_CREATE_HANGING_FACE:
      result += """
  tarch::la::Vector<Dimensions, double> offset(DomainOffset);
  tarch::la::Vector<Dimensions, double> size(DomainSize);
  bool isBoundary = false;
  for (int d=0; d<Dimensions; d++) {{
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) );
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) + size(d) );
  }}
  fineGridFace""" + self.AttributeName + """.setBoundary( isBoundary );
"""

    return result


  def get_attributes(self):
    return ""


  def get_includes(self):
    return """
#include "Constants.h"
"""    

