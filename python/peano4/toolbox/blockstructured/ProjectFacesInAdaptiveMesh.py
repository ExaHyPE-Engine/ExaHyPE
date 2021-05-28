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
    self.d[ "COARSE_GRID_FACE_ACCESSOR" ] = "fineGridFace"  + patch_overlap.name
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
  auto serialisePatchIndex = [](tarch::la::Vector<Dimensions,int> overlapCell, int normal) {{
    int base   = 1;
    int result = 0;
    for (int d=0; d<Dimensions; d++) {{
      result += overlapCell(d) * base;
      if (d==normal) {{
        base *= {OVERLAP}*2;
      }}
      else {{
        base *= {DOFS_PER_AXIS};
      }}
    }}
    return result;
  }};
  
  if ( {GUARD} ) {{
    logTraceIn( "createHangingFace(...)" );
  
    const int normal = marker.getSelectedFaceNumber() % Dimensions;
    const bool left  = marker.getSelectedFaceNumber() < Dimensions;
    
    // clear fine grid data structure
    dfore(k,{DOFS_PER_AXIS},normal,0) {{
      for (int i=0; i<{OVERLAP}; i++) {{
        tarch::la::Vector<Dimensions,int> targetCell = k;
        targetCell(normal)  += left ? i : i + {OVERLAP};
        int targetCellSerialised = serialisePatchIndex(targetCell,normal);
        for (int j=0; j<{UNKNOWNS}; j++) {{
          {FINE_GRID_FACE_ACCESSOR}.value[targetCellSerialised*{UNKNOWNS}+j] = 0.0;
        }}
      }}
    }}
    
    // clear coarse grid data structure (if the fine grid has erased
    // right, I have to erase left)
    dfore(k,{DOFS_PER_AXIS},normal,0) {{
      for (int i=0; i<{OVERLAP}; i++) {{
        tarch::la::Vector<Dimensions,int> targetCell = k;
        targetCell(normal)  += left ? i + {OVERLAP} : i;
        int targetCellSerialised = serialisePatchIndex(targetCell,normal);
        for (int j=0; j<{UNKNOWNS}; j++) {{
          {COARSE_GRID_FACE_ACCESSOR}.value[targetCellSerialised*{UNKNOWNS}+j] = 0.0;
        }}
      }}
    }}
    logTraceOut( "createHangingFace(...)" );
  }}
"""


  ##          {FACES_ACCESSOR}(d+Dimensions).value[overlapCellSerialised*{UNKNOWNS}+j] = 
  #            {CELL_ACCESSOR}.value[patchCellSerialised*{UNKNOWNS}+j];

  __Template_DestroyHangingFace = """
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
""" + self.additional_includes
