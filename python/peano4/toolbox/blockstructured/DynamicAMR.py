# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet



class DynamicAMR(ActionSet):
  """
  This class assumes that you have an 2MxNxN patch on your faces. 
  
  patch: peano4.datamodel.Patch
  patch_overlap_interpolation:  peano4.datamodel.Patch. 
    Consult remark above about how the dimensions of this overlap 
    patch have to match. This overlap is used for the 
    interpolation. It can be the same as for the restriction.
  patch_overlap_restriction: peano4.datamodel.Patch. 
                 
  """
  
  
  def __init__(self, patch, patch_overlap_interpolation, patch_overlap_restriction, interpolation_scheme="piecewise_constant", restriction_scheme="averaging", clear_overlap_in_touch_first_time=True, clear_guard="true", restrict_guard="true", interpolate_guard="true", additional_includes="" ):
    """
    
    restrict_guard: String
      Predicate as C++ expression. It determines which faces are cleared. 
    
    """
    self.d = {}
    if patch_overlap_interpolation.dim[0] % 2 != 0:
      raise Exception( "Error: Patch associated to face has to have even number of cells. Otherwise, it is not a symmetric overlap." )
    if patch_overlap_interpolation.dim != patch_overlap_restriction.dim:
      raise Exception( "Error: Patches associated to face have to have same dimensions for restriction and interpolation." )
      
    self.d[ "UNKNOWNS" ]                  = str(patch_overlap_interpolation.no_of_unknowns)
    self.d[ "DOFS_PER_AXIS" ]             = str(patch_overlap_interpolation.dim[1])
    self.d[ "OVERLAP" ]                   = str(int(patch_overlap_interpolation.dim[0]/2))
    self.d[ "FINE_GRID_CELL" ]            = "fineGridCell"  + patch.name
    self.d[ "COARSE_GRID_CELL" ]          = "coarseGridCell"  + patch.name
    self.d[ "FINE_GRID_FACE_ACCESSOR_INTERPOLATION" ]   = "fineGridFace"  + patch_overlap_interpolation.name
    self.d[ "COARSE_GRID_FACE_ACCESSOR_INTERPOLATION" ] = "coarseGridFaces"  + patch_overlap_interpolation.name
    self.d[ "FINE_GRID_FACE_ACCESSOR_RESTRICTION" ]   = "fineGridFace"  + patch_overlap_restriction.name
    self.d[ "COARSE_GRID_FACE_ACCESSOR_RESTRICTION" ] = "coarseGridFaces"  + patch_overlap_restriction.name
    self.d[ "INTERPOLATION_SCHEME" ]      = interpolation_scheme
    self.d[ "RESTRICTION_SCHEME" ]        = restriction_scheme
    self.d[ "CLEAR_GUARD" ]               = clear_guard
    self.d[ "RESTRICT_GUARD" ]            = restrict_guard
    self.d[ "INTERPOLATE_GUARD" ]         = interpolate_guard
    
    self._clear_overlap_in_touch_first_time = clear_overlap_in_touch_first_time    
    self.additional_includes                = additional_includes


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


  __Template_TouchFaceFirstTime = """
  if ( {CLEAR_GUARD} ) {{
    logTraceIn( "touchFaceFirstTime(...)---DynamicAMR" );

    ::toolbox::blockstructured::clearHaloLayerAoS(
      marker,
      {DOFS_PER_AXIS},
      {OVERLAP},
      {UNKNOWNS},
      {FINE_GRID_FACE_ACCESSOR_RESTRICTION}.value
    );

    logTraceOut( "touchFaceFirstTime(...)---DynamicAMR" );
  }}
"""


  __Template_CreateHangingFace = """
  if ( {INTERPOLATE_GUARD} ) {{
    logTraceIn( "createHangingFace(...)---DynamicAMR" );

    ::toolbox::blockstructured::interpolateOntoOuterHalfOfHaloLayer_AoS_{INTERPOLATION_SCHEME}(
      marker,
      {DOFS_PER_AXIS},
      {OVERLAP},
      {UNKNOWNS},
      {FINE_GRID_FACE_ACCESSOR_INTERPOLATION}.value,
      {COARSE_GRID_FACE_ACCESSOR_INTERPOLATION}(marker.getSelectedFaceNumber()).value
    );
    
    logTraceOut( "createHangingFace(...)---DynamicAMR" );
  }}
"""

  __Template_DestroyHangingFace = """
  if ( {RESTRICT_GUARD} ) {{
    logTraceInWith3Arguments( "destroyHangingFace(...)---DynamicAMR", "{FINE_GRID_FACE_ACCESSOR_RESTRICTION}", "{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}", marker.getSelectedFaceNumber() );

    ::toolbox::blockstructured::restrictOntoOuterHalfOfHaloLayer_AoS_{RESTRICTION_SCHEME}(
      marker,
      {DOFS_PER_AXIS},
      {OVERLAP},
      {UNKNOWNS},
      {FINE_GRID_FACE_ACCESSOR_RESTRICTION}.value,
      {COARSE_GRID_FACE_ACCESSOR_RESTRICTION}(marker.getSelectedFaceNumber()).value
    );

    logTraceOut( "destroyHangingFace(...)---DynamicAMR" );
  }}
"""

  __Template_CreatePersistentFace = """
  logTraceIn( "createPersistentFace(...)---DynamicAMR" );

  ::toolbox::blockstructured::interpolateHaloLayer_AoS_{INTERPOLATION_SCHEME}(
      marker,
      {DOFS_PER_AXIS},
      {OVERLAP},
      {UNKNOWNS},
      {FINE_GRID_FACE_ACCESSOR_INTERPOLATION}.value,
      {COARSE_GRID_FACE_ACCESSOR_INTERPOLATION}(marker.getSelectedFaceNumber()).value
  );
    
  logTraceOut( "createPersistentFace(...)---DynamicAMR" );
"""

  __Template_DestroyPersistentFace = """
  logTraceIn( "createPersistentFace(...)---DynamicAMR" );

  ::toolbox::blockstructured::restrictHaloLayer_AoS_{RESTRICTION_SCHEME}(
      marker,
      {DOFS_PER_AXIS},
      {OVERLAP},
      {UNKNOWNS},
      {FINE_GRID_FACE_ACCESSOR_INTERPOLATION}.value,
      {COARSE_GRID_FACE_ACCESSOR_INTERPOLATION}(marker.getSelectedFaceNumber()).value
  );
    
  logTraceOut( "createPersistentFace(...)---DynamicAMR" );
"""


  __Template_CreateCell = """
  logTraceIn( "createCell(...)---DynamicAMR" );

  ::toolbox::blockstructured::interpolateCell_AoS_{INTERPOLATION_SCHEME}(
      marker,
      {DOFS_PER_AXIS},
      {UNKNOWNS},
      {FINE_GRID_CELL}.value,
      {COARSE_GRID_CELL}.value
  );
    
  logTraceOut( "createCell(...)---DynamicAMR" );
"""

  __Template_DestroyCell = """
  logTraceIn( "destroyCell(...)---DynamicAMR" );

  ::toolbox::blockstructured::restrictCell_AoS_{RESTRICTION_SCHEME}(
      marker,
      {DOFS_PER_AXIS},
      {UNKNOWNS},
      {FINE_GRID_CELL}.value,
      {COARSE_GRID_CELL}.value
  );
    
  logTraceOut( "destroyCell(...)---DynamicAMR" );
"""


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_CREATE_HANGING_FACE:
      result = self.__Template_CreateHangingFace.format(**self.d)
      pass 
    if operation_name==ActionSet.OPERATION_CREATE_PERSISTENT_FACE:
      result = self.__Template_CreatePersistentFace.format(**self.d)
      pass 
    if operation_name==ActionSet.OPERATION_DESTROY_HANGING_FACE:
      result = self.__Template_DestroyHangingFace.format(**self.d)
      pass 
    if operation_name==ActionSet.OPERATION_DESTROY_PERSISTENT_FACE:
      result = self.__Template_DestroyPersistentFace.format(**self.d)
      pass 
    if operation_name==ActionSet.OPERATION_CREATE_CELL:
      result = self.__Template_CreateCell.format(**self.d)
      pass 
    if operation_name==ActionSet.OPERATION_DESTROY_CELL:
      result = self.__Template_DestroyCell.format(**self.d)
      pass 
    if operation_name==ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME and self._clear_overlap_in_touch_first_time:
      result = self.__Template_TouchFaceFirstTime.format(**self.d)
      pass 
    return result


  def get_attributes(self):
    return ""


  def get_includes(self):
    return """
#include "peano4/utils/Loop.h"
#include "toolbox/blockstructured/Interpolation.h"
""" + self.additional_includes
