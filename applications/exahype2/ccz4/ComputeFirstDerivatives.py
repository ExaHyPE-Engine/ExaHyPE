#
# New action set for second order CCZ4 formualtion
#
import peano4
from conda.api import Solver

class ComputeFirstDerivatives(peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor):
    def __init__(self,
                 solver
                 ):
        super( ComputeFirstDerivatives, self ).__init__(patch=solver._patch,
                                                        patch_overlap=solver._patch_overlap_new,
                                                        functor_implementation="""
::exahype2::CellData  patchData( 
  oldQWithHalo, 
  marker.x(), marker.h(), 
  0.0, // timeStamp, 
  0.0, // timeStepSize, 
  newQ 
);
::exahype2::fd::fd4::reconstruct_first_derivatives(patchData,"""+str(solver._patch.dim[0])+","+str(int(solver._patch_overlap_new.dim[0]/2))+","+str(solver._patch.no_of_unknowns)+""",0);
""",
                                                        reconstructed_array_memory_location=solver._reconstructed_array_memory_location,
                                                        guard="not marker.willBeRefined() and not marker.hasBeenRefined()",
                                                        add_assertions_to_halo_exchange=False,
                                                        )
        pass

    
    def user_should_modify_template(self):
        return False
    
    
    #def get_body_of_operation(self,operation_name):
    #    if operation_name==peano4.solversteps.ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
    #        d={}
    #        self.prepare_dictionary_for_action(d)
    #        return self._Template_patch_access.format(**d)
    #    return ""
    

    def get_includes(self):
        return """
    #include "exahype2/CellData.h"
    #include "exahype2/fd/fd4/FD4.h" 
    """
        
    