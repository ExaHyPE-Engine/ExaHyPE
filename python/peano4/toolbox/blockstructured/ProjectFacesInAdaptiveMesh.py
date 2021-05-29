# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet



class ProjectFacesInAdaptiveMesh(ActionSet):
  """
  This class assumes that you have an 2MxNxN patch on your faces. 
  
  patch          Instance of peano4.datamodel.Patch
  patch_overlap  Instance of peano4.datamodel.Patch. Consult remark above about how the
                 dimensions of this overlap patch have to match. 
  """
  
  
  def __init__(self, patch_overlap, guard="true", additional_includes="" ):
    self.d = {}
    if patch_overlap.dim[0] % 2 != 0:
      print( "Error: Patch associated to face has to have even number of cells. Otherwise, it is not a symmetric overlap." )
      assert( patch_overlap.dim[0] % 2 == 0 )
      
    self.d[ "UNKNOWNS" ]                  = str(patch_overlap.no_of_unknowns)
    self.d[ "DOFS_PER_AXIS" ]             = str(patch_overlap.dim[1])
    self.d[ "OVERLAP" ]                   = str(int(patch_overlap.dim[0]/2))
    self.d[ "FINE_GRID_FACE_ACCESSOR" ]   = "fineGridFace"  + patch_overlap.name
    self.d[ "COARSE_GRID_FACE_ACCESSOR" ] = "coarseGridFaces"  + patch_overlap.name
    self.d[ "GUARD" ]                     = guard
    
    self.additional_includes = additional_includes


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


  __Template_CreateHangingFace = """
  if ( {GUARD} ) {{
    logTraceIn( "createHangingFace(...)" );

    ::toolbox::blockstructured::clearHalfOfHaloLayerAoS(
      marker,
      {DOFS_PER_AXIS},
      {OVERLAP},
      {UNKNOWNS},
      false,
      {FINE_GRID_FACE_ACCESSOR}.value
    );

    ::toolbox::blockstructured::clearHalfOfHaloLayerAoS(
      marker,
      {DOFS_PER_AXIS},
      {OVERLAP},
      {UNKNOWNS},
      true,
      {COARSE_GRID_FACE_ACCESSOR}(marker.getSelectedFaceNumber()).value
    );

    ::toolbox::blockstructured::interpolateOntoOuterHalfOfHaloLayer_AoS_piecewise_constant(
      marker,
      {DOFS_PER_AXIS},
      {OVERLAP},
      {UNKNOWNS},
      {FINE_GRID_FACE_ACCESSOR}.value,
      {COARSE_GRID_FACE_ACCESSOR}(marker.getSelectedFaceNumber()).value
    );
    
    logTraceOut( "createHangingFace(...)" );
  }}
"""

  __Template_DestroyHangingFace = """
  if ( {GUARD} ) {{
    logTraceIn( "destroyHangingFace(...)" );

    ::toolbox::blockstructured::restrictOntoOuterHalfOfHaloLayer_AoS_piecewise_constant(
      marker,
      {DOFS_PER_AXIS},
      {OVERLAP},
      {UNKNOWNS},
      {FINE_GRID_FACE_ACCESSOR}.value,
      {COARSE_GRID_FACE_ACCESSOR}(marker.getSelectedFaceNumber()).value
    );

    logTraceOut( "destroyHangingFace(...)" );
  }}
"""


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_CREATE_HANGING_FACE:
      result = self.__Template_CreateHangingFace.format(**self.d)
      pass 
    if operation_name==ActionSet.OPERATION_DESTROY_HANGING_FACE:
      result = self.__Template_DestroyHangingFace.format(**self.d)
      pass 
    return result


  def get_attributes(self):
    return ""


  def get_includes(self):
    return """
#include "peano4/utils/Loop.h"
#include "toolbox/blockstructured/Interpolation.h"
""" + self.additional_includes
