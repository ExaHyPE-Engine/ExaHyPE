# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


import jinja2


class DynamicAMR(ActionSet):
  """
  This class assumes that you have an 2MxNxN patch on your faces. 
  
  patch: peano4.datamodel.Patch
  patch_overlap_interpolation:  peano4.datamodel.Patch
    Consult remark above about how the dimensions of this overlap 
    patch have to match. This overlap is used for the 
    interpolation. It can be the same as for the restriction.
  patch_overlap_restriction: peano4.datamodel.Patch
                 
  """
  
  
  def __init__(self, patch, patch_overlap_interpolation, patch_overlap_restriction, interpolation_scheme="linear", restriction_scheme="averaging", clear_guard="true", restrict_guard="true", interpolate_guard="true", additional_includes="", point_wise_postprocessing_of_interpolation="" , point_wise_postprocessing_of_restriction="" ):
    """
    
    restrict_guard: String
      Predicate as C++ expression. It determines which faces are cleared. 
      
    interpolation_scheme: String
      This is a string that is used to assemble the interpolation scheme that 
      is actually used. At the moment, I mainly offer three variants here:
      - piecewise_constant.
      - linear.
      - linear_precomputed_operators<3>. The 3 has to be replaced by the 
        number of unknowns that you use per coordinate axis.
        
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
    self.d[ "FINE_GRID_FACE_ACCESSOR_RESTRICTION" ]     = "fineGridFace"  + patch_overlap_restriction.name
    self.d[ "COARSE_GRID_FACE_ACCESSOR_RESTRICTION" ]   = "coarseGridFaces"  + patch_overlap_restriction.name
    self.d[ "INTERPOLATION_SCHEME" ]      = interpolation_scheme
    self.d[ "RESTRICTION_SCHEME" ]        = restriction_scheme
    self.d[ "CLEAR_GUARD" ]               = clear_guard
    self.d[ "RESTRICT_GUARD" ]            = restrict_guard
    self.d[ "INTERPOLATE_GUARD" ]         = interpolate_guard
    self.d[ "POINT_WISE_POSTPROCESSING_INTERPOLATION" ] = point_wise_postprocessing_of_interpolation
    self.d[ "POINT_WISE_POSTPROCESSING_RESTRICTION" ]   = point_wise_postprocessing_of_restriction
    
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
  if ( {{CLEAR_GUARD}} ) {
    logTraceInWith2Arguments( "touchFaceFirstTime(...)", marker.toString(), "clear halo layer {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}" );
    
    ::toolbox::blockstructured::clearHaloLayerAoS(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}.value
    );

    logTraceOut( "touchFaceFirstTime(...)" );
  }
"""


  __Template_CreateHangingFace_Prologue = """
  if ( {{INTERPOLATE_GUARD}} ) {
    logTraceInWith1Argument( "createHangingFace(...)", marker.toString() );
"""


  __Template_CreateHangingFace_Core = """
    ::toolbox::blockstructured::interpolateOntoOuterHalfOfHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_INTERPOLATION}}.value,
      {{COARSE_GRID_FACE_ACCESSOR_INTERPOLATION}}(marker.getSelectedFaceNumber()).value
    );
"""


  __Template_CreateHangingFace_Epilogue = """
    {% if POINT_WISE_POSTPROCESSING_INTERPOLATION!="" %}
    const int  normal                        = marker.getSelectedFaceNumber() % Dimensions;
    const bool pickLeftHalfOfHaloOnFineGrid  = marker.getSelectedFaceNumber() < Dimensions;

    dfore(k,{{DOFS_PER_AXIS}},normal,pickLeftHalfOfHaloOnFineGrid ? 0 : {{OVERLAP}}) {
      for (int i=0; i<{{OVERLAP}}; i++) {
        tarch::la::Vector<Dimensions,int>    fineVolume = k;
        fineVolume(normal) +=i ;
        int fineVolumeLinearised = toolbox::blockstructured::serialisePatchIndexInOverlap(
          fineVolume,
          {{DOFS_PER_AXIS}}, {{OVERLAP}}, normal
        );
        double* targetVolume = {{FINE_GRID_FACE_ACCESSOR_INTERPOLATION}}.value + fineVolumeLinearised * {{UNKNOWNS}};
        {{POINT_WISE_POSTPROCESSING_INTERPOLATION}};
      }  
    }
    {% endif %}
    
    logTraceOut( "createHangingFace(...)" );
  }
  else {
    logDebug( "createHangingFace(...)", "skip interpolation for " << marker.toString() << " as interpolation guard did not yield true" );
  }
"""


  __Template_DestroyHangingFace_Prologue = """
  if ( {{RESTRICT_GUARD}} ) {
    logTraceInWith4Arguments( "destroyHangingFace(...)", "{{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}", "{{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}", marker.getSelectedFaceNumber(), marker.toString() );
"""


  __Template_DestroyHangingFace_Core = """
    ::toolbox::blockstructured::restrictOntoOuterHalfOfHaloLayer_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}.value,
      {{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}(marker.getSelectedFaceNumber()).value
    );
"""

    
  __Template_DestroyHangingFace_Epilogue = """
    {% if POINT_WISE_POSTPROCESSING_RESTRICTION!="" %}
    const int  normal                        = marker.getSelectedFaceNumber() % Dimensions;
    const bool pickLeftHalfOfHaloOnFineGrid  = marker.getSelectedFaceNumber() < Dimensions;

    dfore(k,{{DOFS_PER_AXIS}},normal,pickLeftHalfOfHaloOnFineGrid ? 0 : {{OVERLAP}}) {
      for (int i=0; i<{{OVERLAP}}; i++) {
        tarch::la::Vector<Dimensions,int>    fineVolume = k;
        fineVolume(normal) +=i ;
        int fineVolumeLinearised = toolbox::blockstructured::serialisePatchIndexInOverlap(
          fineVolume,
          {{DOFS_PER_AXIS}}, {{OVERLAP}}, normal
        );
        double* targetVolume = {{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}(marker.getSelectedFaceNumber()).value + fineVolumeLinearised * {{UNKNOWNS}};
        {{POINT_WISE_POSTPROCESSING_RESTRICTION}};
      }  
    }
    {% endif %}

    logTraceOut( "destroyHangingFace(...)" );
  }
"""


  __Template_CreatePersistentFace_Prologue = """
  logTraceInWith1Argument( "createPersistentFace(...)", marker.toString() );
"""


  __Template_CreatePersistentFace_Core = """
  ::toolbox::blockstructured::interpolateHaloLayer_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_INTERPOLATION}}.value,
      {{COARSE_GRID_FACE_ACCESSOR_INTERPOLATION}}(marker.getSelectedFaceNumber()).value,
      {{COARSE_GRID_CELL}}.value
  );
"""

    
  __Template_CreatePersistentFace_Epilogue = """
  logTraceOutWith1Argument( "createPersistentFace(...)", marker.toString() );
"""
  

  __Template_DestroyPersistentFace_Prologue = """
  logTraceIn( "destroyPersistentFace(...)" );
  
  if ( not marker.isInteriorFaceWithinPatch() ) {
"""


  __Template_DestroyPersistentFace_Core = """
  ::toolbox::blockstructured::restrictHaloLayer_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{OVERLAP}},
      {{UNKNOWNS}},
      {{FINE_GRID_FACE_ACCESSOR_RESTRICTION}}.value,
      {{COARSE_GRID_FACE_ACCESSOR_RESTRICTION}}(marker.getSelectedFaceNumber()).value
  );
"""    

  
  __Template_DestroyPersistentFace_Epilogue = """
  }
  logTraceOut( "destroyPersistentFace(...)" );
"""


  __Template_CreateCell_Prologue = """
  logTraceIn( "createCell(...)" );
"""

  
  __Template_CreateCell_Core = """
  ::toolbox::blockstructured::interpolateCell_AoS_{{INTERPOLATION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}},
      {{FINE_GRID_CELL}}.value,
      {{COARSE_GRID_CELL}}.value
  );
"""


  __Template_CreateCell_Epilogue = """   
  logTraceOut( "createCell(...)" );
"""


  __Template_DestroyCell_Prologue = """
  logTraceInWith1Argument( "destroyCell(...)", marker.toString() );
"""


  __Template_DestroyCell_Core = """
  ::toolbox::blockstructured::restrictCell_AoS_{{RESTRICTION_SCHEME}}(
      marker,
      {{DOFS_PER_AXIS}},
      {{UNKNOWNS}},
      {{FINE_GRID_CELL}}.value,
      {{COARSE_GRID_CELL}}.value
  );
"""

    
  __Template_DestroyCell_Epilogue = """
  logTraceOut( "destroyCell(...)" );
"""


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_CREATE_HANGING_FACE:
      result =  jinja2.Template(self.__Template_CreateHangingFace_Prologue).render(**self.d)
      result += jinja2.Template(self.__Template_CreateHangingFace_Core).render(**self.d)
      result += jinja2.Template(self.__Template_CreateHangingFace_Epilogue).render(**self.d)
      pass 
    if operation_name==ActionSet.OPERATION_CREATE_PERSISTENT_FACE:
      result =  jinja2.Template(self.__Template_CreatePersistentFace_Prologue).render(**self.d)
      result += jinja2.Template(self.__Template_CreatePersistentFace_Core).render(**self.d)
      result += jinja2.Template(self.__Template_CreatePersistentFace_Epilogue).render(**self.d)
      pass 
    if operation_name==ActionSet.OPERATION_DESTROY_HANGING_FACE:
      result  = jinja2.Template(self.__Template_DestroyHangingFace_Prologue).render(**self.d)
      result += jinja2.Template(self.__Template_DestroyHangingFace_Core).render(**self.d)
      result += jinja2.Template(self.__Template_DestroyHangingFace_Epilogue).render(**self.d)
      pass 
    if operation_name==ActionSet.OPERATION_DESTROY_PERSISTENT_FACE:
      result  = jinja2.Template(self.__Template_DestroyPersistentFace_Prologue).render(**self.d)
      result += jinja2.Template(self.__Template_DestroyPersistentFace_Core).render(**self.d)
      result += jinja2.Template(self.__Template_DestroyPersistentFace_Epilogue).render(**self.d)
      pass 
    if operation_name==ActionSet.OPERATION_CREATE_CELL:
      result  = jinja2.Template(self.__Template_CreateCell_Prologue).render(**self.d)
      result += jinja2.Template(self.__Template_CreateCell_Core).render(**self.d)
      result += jinja2.Template(self.__Template_CreateCell_Epilogue).render(**self.d)
      pass 
    if operation_name==ActionSet.OPERATION_DESTROY_CELL:
      result  = jinja2.Template(self.__Template_DestroyCell_Prologue).render(**self.d)
      result += jinja2.Template(self.__Template_DestroyCell_Core).render(**self.d)
      result += jinja2.Template(self.__Template_DestroyCell_Epilogue).render(**self.d)
      pass 
    if operation_name==ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = jinja2.Template(self.__Template_TouchFaceFirstTime).render(**self.d)
      pass 
    return result


  def get_attributes(self):
    return ""


  def get_includes(self):
    return """
#include "peano4/utils/Loop.h"
#include "toolbox/blockstructured/Enumeration.h"
#include "toolbox/blockstructured/Interpolation.h"
""" + self.additional_includes


  def get_clear_guard(self):
    return self.d[ "CLEAR_GUARD" ]

  
  def get_restrict_guard(self):
    return self.d[ "RESTRICT_GUARD" ]


  def get_interpolate_guard(self):
    return self.d[ "INTERPOLATE_GUARD" ]


  def switch_interpolation_scheme(self,interpolation_scheme):
    self.d[ "INTERPOLATION_SCHEME" ]      = interpolation_scheme


  def switch_restriction_scheme(self,restriction_scheme):
    self.d[ "RESTRICTION_SCHEME" ]        = restriction_scheme


  def switch_point_wise_postprocessing_of_interpolation(self,point_wise_postprocessing_of_interpolation):
    self.d[ "POINT_WISE_POSTPROCESSING_INTERPOLATION" ] = point_wise_postprocessing_of_interpolation


  def switch_point_wise_postprocessing_of_restriction(self,point_wise_postprocessing_of_restriction):
    self.d[ "POINT_WISE_POSTPROCESSING_RESTRICTION" ]   = point_wise_postprocessing_of_restriction

