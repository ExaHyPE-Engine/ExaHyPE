# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .AbstractFVActionSet import AbstractFVActionSet

import peano4
import jinja2

class ProjectPatchOntoFaces( AbstractFVActionSet ):
  """
  
   This class is very similar to peano4.toolbox.blockstructured.ProjectPatchOntoFaces, 
   but there are some delicate differences.
  
   - We project the patch boundary data onto the faces. We always project onto the
     new snapshot of a face.
   - At the same time, we roll over the new data into the old data along the faces.
   - We eventually update the face meta data (timestamps) accordingly.

   For further details, please consult peano4.toolbox.blockstructured.ProjectPatchOntoFaces.
   Though I don't inherit from this class directly, it is kind of the blueprint for the present 
   type.
   
  """
  def __init__(self,solver, predicate):
    AbstractFVActionSet.__init__(self, solver)
    
    self.d = {}
    if solver._patch_overlap_new.dim[0] % 2 != 0:
      print( "Error: Patch associated to face has to have even number of cells. Otherwise, it is not a symmetric overlap." )
      assert( solver._patch_overlap_new.dim[0] % 2 == 0 )
    if solver._patch.dim[0] != solver._patch.dim[1]:
      print( "Error: Can only handle square patches." )
      assert( solver._patch.dim[0] == solver._patch.dim[1] )
    if solver._patch_overlap_new.dim[1] != solver._patch.dim[0]:
      print( "Error: Patch of overlap and patch of cell have to match" )
      assert( solver._patch_overlap_new.dim[1] == solver._patch.dim[0] )
      
    self.d[ "UNKNOWNS" ]           = str(solver._patch.no_of_unknowns)
    self.d[ "DOFS_PER_AXIS" ]      = str(solver._patch.dim[0])
    self.d[ "OVERLAP" ]            = str(int(solver._patch_overlap_new.dim[0]/2))
    self.d[ "NEW_FACES_ACCESSOR" ] = "fineGridFaces"  + solver._patch_overlap_new.name
    self.d[ "OLD_FACES_ACCESSOR" ] = "fineGridFaces"  + solver._patch_overlap_old.name
    self.d[ "CELL_ACCESSOR" ]      = "fineGridCell" + solver._patch.name
    self.d[ "GUARD" ]              = predicate


#  def get_constructor_body(self):
#    return ""

  
#  def get_destructor_body(self):
#    return ""


#  def get_body_of_getGridControlEvents(self):
#    return "  return std::vector< peano4::grid::GridControlEvent >();\n" 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  __Template_TouchCellFirstTime = """
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
    logTraceIn( "touchCellLastTime(...)---ProjectPatchOntoFaces" );

    for(int d=0; d<Dimensions; d++) {{
      /**
       * d-loop over all dimensions except d. The vector k's entry d is set
       * to 0. We start with the left/bottom face, i.e. the one closer to the 
       * coordinate system's origin.
       */
      dfore(k,{DOFS_PER_AXIS},d,0) {{
        for (int i=0; i<{OVERLAP}; i++) {{
          tarch::la::Vector<Dimensions,int> patchCell   = k;
          tarch::la::Vector<Dimensions,int> overlapCell = k;
          patchCell(d)   = i;
          overlapCell(d) = i+{OVERLAP};
          
          int patchCellSerialised   = peano4::utils::dLinearised(patchCell,{DOFS_PER_AXIS});
          int overlapCellSerialised = serialisePatchIndex(overlapCell,d);
          for (int j=0; j<{UNKNOWNS}; j++) {{
            assertion7( 
              {CELL_ACCESSOR}.value[patchCellSerialised*{UNKNOWNS}+j]=={CELL_ACCESSOR}.value[patchCellSerialised*{UNKNOWNS}+j], j,i,k,d, patchCell, overlapCell,
              marker.toString() 
            );
            {OLD_FACES_ACCESSOR}(d).value[overlapCellSerialised*{UNKNOWNS}+j] = 
              {NEW_FACES_ACCESSOR}(d).value[overlapCellSerialised*{UNKNOWNS}+j];
            {NEW_FACES_ACCESSOR}(d).value[overlapCellSerialised*{UNKNOWNS}+j] = 
              {CELL_ACCESSOR}.value[patchCellSerialised*{UNKNOWNS}+j];
          }}
  
          patchCell(d)   = i+{DOFS_PER_AXIS}-{OVERLAP};
          overlapCell(d) = i;
          
          patchCellSerialised   = peano4::utils::dLinearised(patchCell,{DOFS_PER_AXIS});
          overlapCellSerialised = serialisePatchIndex(overlapCell,d);
          for (int j=0; j<{UNKNOWNS}; j++) {{
            assertion7( 
              {CELL_ACCESSOR}.value[patchCellSerialised*{UNKNOWNS}+j]=={CELL_ACCESSOR}.value[patchCellSerialised*{UNKNOWNS}+j], j,i,k,d, patchCell, overlapCell, 
              marker.toString() 
            );
            {OLD_FACES_ACCESSOR}(d).value[overlapCellSerialised*{UNKNOWNS}+j] = 
              {NEW_FACES_ACCESSOR}(d).value[overlapCellSerialised*{UNKNOWNS}+j];
            {NEW_FACES_ACCESSOR}(d+Dimensions).value[overlapCellSerialised*{UNKNOWNS}+j] = 
              {CELL_ACCESSOR}.value[patchCellSerialised*{UNKNOWNS}+j];
          }}
        }}
      }}
    }}
    logTraceOut( "touchCellLastTime(...)---ProjectPatchOntoFaces" );
  }}
  else {{
    logTraceInWith1Argument( "touchCellLastTime(...)---ProjectPatchOntoFaces [skip]", marker.toString() );
    logTraceOut( "touchCellLastTime(...)---ProjectPatchOntoFaces [skip]" );
  }}
"""


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_LAST_TIME:
      result = self.__Template_TouchCellFirstTime.format(**self.d)
      pass 
    return result


  def get_attributes(self):
    return ""


#  def get_includes(self):
#    return """
##include "peano4/utils/Loop.h"
#""" + self.additional_includes
