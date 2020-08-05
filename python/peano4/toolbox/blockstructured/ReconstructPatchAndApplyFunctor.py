# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from peano4.solversteps.ActionSet import ActionSet


from enum import Enum

class ReconstructedArrayMemoryLocation(Enum):
  """
   All arrays are held on the call stack. Might not work with PGI compilers or 
   Clang, but it does work with GCC and Intel. This mode is the preferred one,
   as all memory frees are implicitly done.
  """
  CallStack = 0,
  """
   Create data on the heap. Done via a plain new. You have to delete the 
   reconstructed data yourself.
  """
  Heap = 1,
  """
   Create data on the heap for an accelerator. It relies on the tarch::allocateMemoryOnAccelerator
   routine and you have to use this one again to free it.
  """
  AcceleratorMemory = 2




class ReconstructPatchAndApplyFunctor(ActionSet):
  """
  This class assumes that you have NxNxN patch within your block. It also assumes that 
  you have an 2MxNxN patch on your faces. M<N. The code interprets these face-associated
  data as overlap with the patch, hooks into touchCellLastTime, and projects the cell's
  patch data onto the overlap (simple copy). See ProjectPatchOntoFaces for details. 
  
  If the face patches are auxiliary patches holding copies from the cell patches, then 
  we can reconstruct N+2M x N+2M x N+2M patches per cell whenever we hit a cell: We 
  create a new temporary array, and copy data from both the cell patch and the faces
  into this array (gather operation). After that, we can launch the passed functor
  giving it access to the temporary, large array plus the original patch data. 
  
  """
  
  
  def __init__(self,patch,patch_overlap,functor_implementation,touch_face_first_time_functor,guard_cell_operation,guard_face_operation,additional_includes,reconstructed_array_memory_location=ReconstructedArrayMemoryLocation.CallStack):
    """

  patch: peano4.datamodel.Patch
    Patch which is to be used
    
  patch_overlap: peano4.datamodel.Patch
    Consult remark above about how the dimensions of this overlap 
    patch have to match
     
  <h2> Functor_implementation </h2> 
  
  The functor implementation is a plain C/C++ 
  
  - If you want to use brackets in your implementation, please use double brackets {{ }} as 
    the template system otherwise gets confused.
  - The following C++ variables are defined:
  
    reconstructedPatch 
    originalPatch
    
    Both are plain double pointers.
  
  <h2> Data validation </h2>
  
  I use quite a lot of validitiy checks for the copied data via comparison to its self. This
  way, I can at least spot nans. I use the dictionary entries ASSERTION_WITH_X_ARGUMENTS to 
  realise this, and they are, by default, set to Peano's assertion macros. You can redefine 
  them. 
   
    """
    self.d = {}
    if patch_overlap.dim[0] % 2 != 0:
      print( "Error: Patch associated to face has to have even number of cells. Otherwise, it is not a symmetric overlap." )
      assert( patch_overlap.dim[0] % 2 == 0 )
    if patch.dim[0] != patch.dim[1]:
      print( "Error: Can only handle square patches." )
      assert( patch.dim[0] == patch.dim[1] )
    if patch_overlap.dim[1] != patch.dim[0]:
      print( "Error: Patch of overlap and patch of cell have to match" )
      assert( patch_overlap.dim[1] == patch.dim[0] )
      
    self.d[ "UNKNOWNS" ]           = str(int(patch.no_of_unknowns))
    self.d[ "DOFS_PER_AXIS" ]      = str(int(patch.dim[0]))
    self.d[ "OVERLAP" ]            = str(int(patch_overlap.dim[0]/2))
    self.d[ "NUMBER_OF_DOUBLE_VALUES_IN_ORIGINAL_PATCH_2D" ]      = str(int(patch.no_of_unknowns * patch.dim[0] * patch.dim[0]))
    self.d[ "NUMBER_OF_DOUBLE_VALUES_IN_ORIGINAL_PATCH_3D" ]      = str(int(patch.no_of_unknowns * patch.dim[0] * patch.dim[0] * patch.dim[0]))
    self.d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_2D" ] = str(int(patch.no_of_unknowns * (patch_overlap.dim[0] + patch.dim[0]) * (patch_overlap.dim[0] + patch.dim[0])))
    self.d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_3D" ] = str(int(patch.no_of_unknowns * (patch_overlap.dim[0] + patch.dim[0]) * (patch_overlap.dim[0] + patch.dim[0]) * (patch_overlap.dim[0] + patch.dim[0])))
    self.d[ "FACES_ACCESSOR" ]     = "fineGridFaces"  + patch_overlap.name
    self.d[ "CELL_ACCESSOR" ]      = "fineGridCell" + patch.name
    
    self.d[ "ASSERTION_WITH_1_ARGUMENTS" ] = "assertion1"
    self.d[ "ASSERTION_WITH_2_ARGUMENTS" ] = "assertion2"
    self.d[ "ASSERTION_WITH_3_ARGUMENTS" ] = "assertion3"
    self.d[ "ASSERTION_WITH_4_ARGUMENTS" ] = "assertion4"
    self.d[ "ASSERTION_WITH_5_ARGUMENTS" ] = "assertion5"
    self.d[ "ASSERTION_WITH_6_ARGUMENTS" ] = "assertion6"
    
    self.d[ "CELL_FUNCTOR_IMPLEMENTATION" ] = functor_implementation
    self.d[ "FACE_FUNCTOR_IMPLEMENTATION" ] = touch_face_first_time_functor

    self.d[ "GUARD_CELL_OPERATION" ]                 = guard_cell_operation
    self.d[ "GUARD_FACE_OPERATION" ]                 = guard_face_operation

    if reconstructed_array_memory_location==ReconstructedArrayMemoryLocation.Heap:
      self.d[ "CREATE_RECONSTRUCTED_PATCH" ] = """
    #if Dimensions==2
    double* reconstructedPatch = new double[""" + self.d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_2D" ] + """];
    #elif Dimensions==3
    double* reconstructedPatch = new double[""" + self.d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_3D" ] + """];
    #endif
"""    
    elif reconstructed_array_memory_location==ReconstructedArrayMemoryLocation.CallStack:
      self.d[ "CREATE_RECONSTRUCTED_PATCH" ] = """
    #if Dimensions==2
    double reconstructedPatch[""" + self.d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_2D" ] + """];
    #elif Dimensions==3
    double reconstructedPatch[""" + self.d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_3D" ] + """];
    #endif
"""    
    elif reconstructed_array_memory_location==ReconstructedArrayMemoryLocation.AcceleratorMemory:
      self.d[ "CREATE_RECONSTRUCTED_PATCH" ] = """
    double* reconstructedPatch;
    #if Dimensions==2
    reconstructedPatch = ::tarch::multicore::allocateMemory(""" + self.d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_2D" ] + """, ::tarch::multicore::MemoryLocation::Accelerator);
    #elif Dimensions==3
    reconstructedPatch = ::tarch::multicore::allocateMemory(""" + self.d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_3D" ] + """, ::tarch::multicore::MemoryLocation::Accelerator);
    #endif
"""    
    else:  
      printf( "Error: memory allocation mode for patch reconstruction not known")

    self.additional_includes                         = additional_includes
    self.additional_attributes                       = ""


  def get_constructor_body(self):
    return """  _treeNumber = treeNumber;
"""    

  
  def get_destructor_body(self):
    return ""


  def get_body_of_getGridControlEvents(self):
    return "  return std::vector< peano4::grid::GridControlEvent >();\n" 


  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  __Template_TouchFaceFirstTime = """
  if ( {GUARD_FACE_OPERATION} ) {{
    logTraceInWith2Arguments( "touchFaceFirstTime(...)", marker.toString(), marker.isLocal() );
    {FACE_FUNCTOR_IMPLEMENTATION}
    logTraceOut( "touchFaceFirstTime(...)" );
  }}
"""


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

  if ({GUARD_CELL_OPERATION}) {{
    logTraceInWith1Argument( "touchCellFirstTime(...)", marker.toString() );

    {CREATE_RECONSTRUCTED_PATCH}

    //
    // Loop over original patch (k) and copy stuff over.
    //
    dfor(sourceCell,{DOFS_PER_AXIS}) {{
      tarch::la::Vector<Dimensions,int> destinationCell = sourceCell + tarch::la::Vector<Dimensions,int>({OVERLAP});
      int sourceCellSerialised       = peano4::utils::dLinearised(sourceCell,{DOFS_PER_AXIS});
      int destinationCellSerialised  = peano4::utils::dLinearised(destinationCell,{DOFS_PER_AXIS} + 2*{OVERLAP});
      for (int j=0; j<{UNKNOWNS}; j++) {{
        reconstructedPatch[destinationCellSerialised*{UNKNOWNS}+j] = {CELL_ACCESSOR}.value[ sourceCellSerialised*{UNKNOWNS}+j ];
        {ASSERTION_WITH_3_ARGUMENTS}( reconstructedPatch[destinationCellSerialised*{UNKNOWNS}+j]==reconstructedPatch[destinationCellSerialised*{UNKNOWNS}+j], sourceCell, j, _treeNumber );
      }}
    }}
  
    //
    // Bring in the auxiliary patches, i.e. befill halo
    //
    for(int d=0; d<Dimensions; d++) {{
      logTraceInWith1Argument( "touchCellFirstTime(...)::loopOverFace", d );
      //
      // d-loop over all dimensions except d. The vector k's entry d is set
      // to 0. We start with the left/bottom face, i.e. the one closer to the 
      // coordinate system's origin.
      //
      dfore(k,{DOFS_PER_AXIS},d,0) {{
        for (int i=0; i<{OVERLAP}; i++) {{
          tarch::la::Vector<Dimensions,int> destinationCell = k + tarch::la::Vector<Dimensions,int>({OVERLAP});
          tarch::la::Vector<Dimensions,int> sourceCell      = k;
          destinationCell(d) = i;
          sourceCell(d)      = i;
          
          int destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,{DOFS_PER_AXIS} + 2*{OVERLAP});
          int sourceCellSerialised        = serialisePatchIndex(sourceCell,d);

          for (int j=0; j<{UNKNOWNS}; j++) {{
            reconstructedPatch[ destinationCellSerialised*{UNKNOWNS}+j ] = {FACES_ACCESSOR}(d).value[ sourceCellSerialised*{UNKNOWNS}+j ];
            {ASSERTION_WITH_6_ARGUMENTS}( 
              reconstructedPatch[ destinationCellSerialised*{UNKNOWNS}+j ]==reconstructedPatch[ destinationCellSerialised*{UNKNOWNS}+j ], 
              sourceCell, destinationCell, j, d, marker.toString(), _treeNumber 
            );
          }}

          destinationCell(d) = i+{DOFS_PER_AXIS}+{OVERLAP};
          sourceCell(d)      = i+{OVERLAP};

          destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,{DOFS_PER_AXIS} + 2*{OVERLAP});
          sourceCellSerialised        = serialisePatchIndex(sourceCell,d);
          for (int j=0; j<{UNKNOWNS}; j++) {{
            reconstructedPatch[ destinationCellSerialised*{UNKNOWNS}+j ] = {FACES_ACCESSOR}(d+Dimensions).value[ sourceCellSerialised*{UNKNOWNS}+j ];
            {ASSERTION_WITH_6_ARGUMENTS}( 
              reconstructedPatch[ destinationCellSerialised*{UNKNOWNS}+j ]==reconstructedPatch[ destinationCellSerialised*{UNKNOWNS}+j ], 
              sourceCell, destinationCell, j, d, marker.toString(), _treeNumber
            );
          }}
        }}
      }}
      logTraceOut( "touchCellFirstTime(...)::loopOverFace" );
    }}

    double* originalPatch = {CELL_ACCESSOR}.value;
    {CELL_FUNCTOR_IMPLEMENTATION}
    logTraceOut( "touchCellFirstTime(...)" );
  }}
"""


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      result = self.__Template_TouchCellFirstTime.format(**self.d)
      pass 
    if operation_name==ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      result = self.__Template_TouchFaceFirstTime.format(**self.d)
      pass 
    
    return result


  def get_attributes(self):
    return """int  _treeNumber;
""" + self.additional_attributes   


  def get_includes(self):
    return """
#include <functional>
#include "peano4/utils/Loop.h"
""" + self.additional_includes
