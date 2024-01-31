# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
from .ActionSet import ActionSet


class UserActionSet(ActionSet):
    def __init__(self, name, descend_invocation_order=0, parallel=True):
        super(UserActionSet, self).__init__(descend_invocation_order, parallel)
        self.name = name
        pass

    def user_should_modify_template(self):
        return True

    def get_constructor_body(self):
        return "// @todo Please implement\n"

    def get_destructor_body(self):
        return "// @todo Please implement\n"

    def get_body_of_getGridControlEvents(self):
        return "// @todo Please implement\nreturn std::vector< peano4::grid::GridControlEvent >();\n"


  def get_destructor_body(self):
    return "// @todo Please implement\n"


  def get_body_of_getGridControlEvents(self):
    return "// @todo Please implement\nreturn std::vector< peano4::grid::GridControlEvent >();\n" 


  def get_body_of_operation(self,operation_name):
    if operation_name=="touchCellFirstTime" and self.action_type != None:
      d={}
      self.prepare_dictionary_for_action(d)
      return self._Template_patch_access.format(**d)
    else:
      return """
// Feel free to comment in and to add arguments if you want to trace them.
// Ensure the logTraceOut is activated, too, if you switch on traceIn.     
// logTraceIn( \""""
            + operation_name
            + """()" );
// @todo Please implement
// logTraceOut( \""""
            + operation_name
            + """()" );
"""
        )

    def get_action_set_name(self):
        """
        Returns a description (word) for the mapping which is also used as class name
        for the generated type. As a consequence, the result should be one word (if
        possible) and uppercase. Also, every subclass should overwrite this routine.
        """
        return self.name

    def get_attributes(self):
        return "// @todo Please implement\n"


  def get_attributes(self):
    return "// @todo Please implement\n"


  def get_includes(self):
    if self.action_type=="FD4_first_derivative_calculation":
      return """
#include "exahype2/CellData.h"
#include "exahype2/fd/fd4/FD4.h" 
"""
    else:
      return "// @todo Please implement\n"


  def prepare_dictionary_for_action(self,d):
    d[ "GUARD" ]              = "not marker.willBeRefined() and not marker.hasBeenRefined()"

    d[ "UNKNOWNS" ]           = str(self.referring_solver._patch.no_of_unknowns)
    d[ "DOFS_PER_AXIS" ]      = str(self.referring_solver._patch.dim[0])
    d[ "OVERLAP" ]            = str(int(self.referring_solver._patch_overlap_new.dim[0]/2))
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_ORIGINAL_PATCH_2D" ]      = str(self.referring_solver._patch.no_of_unknowns * self.referring_solver._patch.dim[0]**2)
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_ORIGINAL_PATCH_3D" ]      = str(self.referring_solver._patch.no_of_unknowns * self.referring_solver._patch.dim[0]**3)
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_2D" ] = str(self.referring_solver._patch.no_of_unknowns * (int(self.referring_solver._patch_overlap_new.dim[0]) + self.referring_solver._patch.dim[0])**2)
    d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_3D" ] = str(self.referring_solver._patch.no_of_unknowns * (int(self.referring_solver._patch_overlap_new.dim[0]) + self.referring_solver._patch.dim[0])**3)
    d[ "FACES_ACCESSOR" ]     = "fineGridFaces"  + self.referring_solver._patch_overlap_new.name
    d[ "CELL_ACCESSOR" ]      = "fineGridCell" + self.referring_solver._patch.name

    d[ "ASSERTION_WITH_1_ARGUMENTS" ] = "assertion1"
    d[ "ASSERTION_WITH_2_ARGUMENTS" ] = "assertion2"
    d[ "ASSERTION_WITH_3_ARGUMENTS" ] = "assertion3"
    d[ "ASSERTION_WITH_4_ARGUMENTS" ] = "assertion4"
    d[ "ASSERTION_WITH_5_ARGUMENTS" ] = "assertion5"
    d[ "ASSERTION_WITH_6_ARGUMENTS" ] = "assertion6"
    d[ "ASSERTION_WITH_7_ARGUMENTS" ] = "assertion7"

    if self.action_type == "FD4_first_derivative_calculation":
      d[ "ADDITIONAL_PROCESSOR" ] = "::exahype2::fd::fd4::AlgorithmStep_patchwise_reconstruct_first_derivatives(patchData,"+str(self.referring_solver._patch.dim[0])+","+str(int(self.referring_solver._patch_overlap_new.dim[0]/2))+","+str(self.referring_solver._patch.no_of_unknowns)+",0);"
    else:
      d[ "ADDITIONAL_PROCESSOR" ] = " "

    if self.referring_solver._reconstructed_array_memory_location==ReconstructedArrayMemoryLocation.Heap or self.referring_solver._reconstructed_array_memory_location==ReconstructedArrayMemoryLocation.HeapWithoutDelete:
      d[ "CREATE_RECONSTRUCTED_PATCH" ] = """
    #if Dimensions==2
      double* oldQWithHalo = new double[""" + d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_2D" ] + """];
    #elif Dimensions==3
      double* oldQWithHalo = new double[""" + d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_3D" ] + """];
    #endif
"""
    elif self.referring_solver._reconstructed_array_memory_location==ReconstructedArrayMemoryLocation.CallStack:
      d[ "CREATE_RECONSTRUCTED_PATCH" ] = """
    #if Dimensions==2
      double oldQWithHalo[""" + d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_2D" ] + """];
    #elif Dimensions==3
      double oldQWithHalo[""" + d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_3D" ] + """];
    #endif
"""
    elif self.referring_solver._reconstructed_array_memory_location==ReconstructedArrayMemoryLocation.HeapThroughTarch or self.referring_solver._reconstructed_array_memory_location==ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete:
      d[ "CREATE_RECONSTRUCTED_PATCH" ] = """
    #if Dimensions==2
      double* oldQWithHalo = ::tarch::allocateMemory<double>(""" + d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_2D" ] + """, ::tarch::MemoryLocation::Heap);
    #elif Dimensions==3
      double* oldQWithHalo = ::tarch::allocateMemory<double>(""" + d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_3D" ] + """, ::tarch::MemoryLocation::Heap);
    #endif
"""
    elif self.referring_solver._reconstructed_array_memory_location==ReconstructedArrayMemoryLocation.ManagedSharedAcceleratorDeviceMemoryThroughTarch or self.referring_solver._reconstructed_array_memory_location==ReconstructedArrayMemoryLocation.ManagedSharedAcceleratorDeviceMemoryThroughTarchWithoutDelete:
      d[ "CREATE_RECONSTRUCTED_PATCH" ] = """
    #if Dimensions==2
      double* oldQWithHalo = ::tarch::allocateMemory<double>(""" + d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_2D" ] + """, ::tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
    #elif Dimensions==3
      double* oldQWithHalo = ::tarch::allocateMemory<double>(""" + d[ "NUMBER_OF_DOUBLE_VALUES_IN_RECONSTRUCTED_PATCH_3D" ] + """, ::tarch::MemoryLocation::ManagedSharedAcceleratorDeviceMemory);
    #endif
"""
    else:
      print( "Error: memory allocation mode for patch reconstruction not known")



  _Template_patch_access="""
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

  if ({GUARD}) {{
    logTraceInWith1Argument( "touchCellFirstTime(...)", marker.toString() );

    {CREATE_RECONSTRUCTED_PATCH}

    dfor(sourceCell,{DOFS_PER_AXIS}) {{
      tarch::la::Vector<Dimensions,int> destinationCell = sourceCell + tarch::la::Vector<Dimensions,int>({OVERLAP});
      int sourceCellSerialised       = peano4::utils::dLinearised(sourceCell,{DOFS_PER_AXIS});
      int destinationCellSerialised  = peano4::utils::dLinearised(destinationCell,{DOFS_PER_AXIS} + 2*{OVERLAP});
      for (int j=0; j<{UNKNOWNS}; j++) {{
        oldQWithHalo[destinationCellSerialised*{UNKNOWNS}+j] = {CELL_ACCESSOR}.value[ sourceCellSerialised*{UNKNOWNS}+j ];
        {ASSERTION_WITH_4_ARGUMENTS}( oldQWithHalo[destinationCellSerialised*{UNKNOWNS}+j]==oldQWithHalo[destinationCellSerialised*{UNKNOWNS}+j], sourceCell, j, _treeNumber, marker.toString() );
      }}
    }}

    //
    // Bring in the auxiliary patches, i.e. befill halo
    //
    for(int d=0; d<Dimensions; d++) {{
      logTraceInWith1Argument( "touchCellFirstTime(...)::loopOverFace", d );
      dfore(k,{DOFS_PER_AXIS},d,0) {{
        for (int i=0; i<{OVERLAP}; i++) {{
          tarch::la::Vector<Dimensions,int> destinationCell = k + tarch::la::Vector<Dimensions,int>({OVERLAP});
          tarch::la::Vector<Dimensions,int> sourceCell      = k;
          destinationCell(d) = i;
          sourceCell(d)      = i;
          
          int destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,{DOFS_PER_AXIS} + 2*{OVERLAP});
          int sourceCellSerialised        = serialisePatchIndex(sourceCell,d);

          for (int j=0; j<{UNKNOWNS}; j++) {{
            oldQWithHalo[ destinationCellSerialised*{UNKNOWNS}+j ] = {FACES_ACCESSOR}(d).value[ sourceCellSerialised*{UNKNOWNS}+j ];
          }}

          destinationCell(d) = i+{DOFS_PER_AXIS}+{OVERLAP};
          sourceCell(d)      = i+{OVERLAP};

          destinationCellSerialised   = peano4::utils::dLinearised(destinationCell,{DOFS_PER_AXIS} + 2*{OVERLAP});
          sourceCellSerialised        = serialisePatchIndex(sourceCell,d);
          for (int j=0; j<{UNKNOWNS}; j++) {{
            oldQWithHalo[ destinationCellSerialised*{UNKNOWNS}+j ] = {FACES_ACCESSOR}(d+Dimensions).value[ sourceCellSerialised*{UNKNOWNS}+j ];
          }}
        }}
      }}
      logTraceOut( "touchCellFirstTime(...)::loopOverFace" );
    }}

    double* newQ = {CELL_ACCESSOR}.value;

    ::exahype2::CellData patchData( oldQWithHalo, marker.x(), marker.h(), 0.0,0.01, newQ );

    {ADDITIONAL_PROCESSOR}

    logTraceOut( "touchCellFirstTime(...)" );
  }}

"""

