# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet

import dastgen2
import peano4.datamodel.DaStGen2


class UpdateFaceLabel(ActionSet):
  def get_attribute_name(solver_name):
    return solver_name + "FaceLabel"
  
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
    if operation_name==ActionSet.OPERATION_CREATE_PERSISTENT_FACE or operation_name==ActionSet.OPERATION_CREATE_HANGING_FACE:
      result += """
  tarch::la::Vector<Dimensions, double> offset(DomainOffset);
  tarch::la::Vector<Dimensions, double> size(DomainSize);
  bool isBoundary = false;
  for (int d=0; d<Dimensions; d++) {{
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) );
    isBoundary |= tarch::la::equals( marker.x()(d), offset(d) + size(d) );
  }}
  fineGridFace""" + UpdateFaceLabel.get_attribute_name(self._solver_name) + """.setBoundary( isBoundary );
"""
    if operation_name==ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = """
  fineGridFace""" + UpdateFaceLabel.get_attribute_name(self._solver_name) + """.setUpdated( false );
"""

    return result


  def get_attributes(self):
    return ""


  def get_includes(self):
    return """
#include "Constants.h"
"""    


def create_face_label(solver_name):
  """

   Build up the DaStGen2 data structure that we need per face to maintain
   per-face data per solver.
     
   solver_name: string
     Name of the solver
     
  """
  result = peano4.datamodel.DaStGen2( UpdateFaceLabel.get_attribute_name( solver_name ) )

  result.data.add_attribute( dastgen2.attributes.Boolean("Boundary") )
  result.data.add_attribute( dastgen2.attributes.BooleanArray("Updated","2") )
  result.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("UpdatedTimeStamp","2") )
  result.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("NewTimeStamp","2") )
  result.data.add_attribute( peano4.dastgen2.Peano4DoubleArray("OldTimeStamp","2") )
  
  result._peano4_aspect.merge_implementation = """
  assertionEquals( _Boundary, neighbour._Boundary );
  
  const int normal         = marker.getSelectedFaceNumber() % Dimensions;
  const int neighbourEntry = marker.outerNormal()(normal)<0.0 ? 0 : 1;
  
  _Updated[ neighbourEntry ]          = neighbour._Updated[ neighbourEntry ];
  _UpdatedTimeStamp( neighbourEntry ) = neighbour._UpdatedTimeStamp( neighbourEntry );
  _NewTimeStamp( neighbourEntry )     = neighbour._NewTimeStamp( neighbourEntry );
  _OldTimeStamp( neighbourEntry )     = neighbour._OldTimeStamp( neighbourEntry );
  
"""

  return result

