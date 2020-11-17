# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import FV
 
from .PDETerms import PDETerms

import peano4

import jinja2


#! =====================================================
#subroutine rp1_ptwise(num_eqn, num_aux, num_waves, q_l, q_r, aux_l, aux_r,    &
#                       wave, s, amdq, apdq)
#! =====================================================###

#!     # Pointwise Riemann solver for the acoustics equations in 1d, with
#!     #  variable coefficients (heterogeneous media)

#! waves:     2
#! equations: 2
#! aux fields: 2


class PointWiseClawPackFixedTimeStepSize(  FV ):
  """
  
    Very simple solver that can use ClawPack's point-wise Riemann solver.
  

   The actual call of the Riemann solver has to be a simple function call
   which has the following variables available. The FL and FR is the only
   out array. All the other data are in.
   
      double QL[],
      double QR[],
      const tarch::la::Vector<Dimensions,double>&  x,
      double                                       dx,
      double                                       t,
      double                                       dt,
      int                                          normal,
      int                                          unknowns,
      int                                          auxiliaryVariables,
      double                                       FL[],
      double                                       FR[]
      
   My generic kernel requires FR and FL along the normal of the respective
   cell you look at F. That is, I need -FR compared to what ClawPack gives 
   me. Quick trial and error with the acoustics equations tells you that
   it indeed has to be -FR and not -FL.
   
   There's also the opportunity to misuse a degenerated 1d Riemann solver, i.e.
   a solver specified over a 1d array as point-wise solver.  
      
  """
  
  
  TemplateUpdateCell = """
    {{LOOP_OVER_PATH_FUNCTION_CALL}}(
      [&](
        double                                       QL[],
        double                                       QR[],
        const tarch::la::Vector<Dimensions,double>&  x,
        double                                       dx,
        double                                       t,
        double                                       dt,
        int                                          normal,
        double                                       FL[],
        double                                       FR[]
      ) -> void {
        double wave[{{NUMBER_OF_UNKNOWNS}}]; 
        double speed[{{NUMBER_OF_UNKNOWNS}}]; 

        int num_eqn   = {{NUMBER_OF_UNKNOWNS}};
        int num_aux   = {{NUMBER_OF_AUXILIARY_VARIABLES}};
        int num_waves = {{NUMBER_OF_UNKNOWNS}}; 

        {{CLAWPACK_RIEMANN_SOLVER}}_(
          &num_eqn,
          &num_aux,
          &num_waves, 
          QL,                                 // double* q_l 
          QR,                                 // double* q_r
          QL+{{NUMBER_OF_UNKNOWNS}},          // double* aux_l
          QR+{{NUMBER_OF_UNKNOWNS}},          // double* aux_r
          wave,
          speed,
          FL,                                 // double* amdq
          FR                                  // double* apdq
        );

        for (int i=0; i<{{NUMBER_OF_UNKNOWNS}}; i++) {
          FR[i] = -FR[i];
        }
      },
      marker.x(),
      marker.h(),
      {{SOLVER_INSTANCE}}.getMinTimeStamp(),
      {{TIME_STEP_SIZE}},
      {{NUMBER_OF_VOLUMES_PER_AXIS}},
      {{NUMBER_OF_UNKNOWNS}},
      {{NUMBER_OF_AUXILIARY_VARIABLES}},
      reconstructedPatch,
      originalPatch
    );
  """      
  

  
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size, clawpack_Riemann_solver, Riemann_solver_implementation_files = [], plot_grid_properties=False, kernel_implementation = FV.CellUpdateImplementation_NestedLoop):
    """
    
      Instantiate a generic FV scheme with an overlap of 1.
      
      
      Riemann_solver_implementation_files: list of strings
        These are the Fortran files the code should add to the compile.
        
        
      is_degenerated_1d_solver: Boolean
        I prefer ClawPack solvers which are written point-wisely. Alternatively, I 
        can misuse 1d solvers.  
      
    """
    #super(GenericRusanovFVFixedTimeStepSize,self).__init__(name, patch_size, unknowns, auxiliary_variables, min_h, max_h, flux, ncp, plot_grid_properties)
    FV.__init__(self, name, patch_size, 1, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)
    
    self._time_step_size = time_step_size
    
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation
    self._kernel_implementation               = kernel_implementation

    self.set_implementation()
    self.set_update_cell_implementation(kernel_implementation=kernel_implementation)

    self._patch_overlap.generator.send_condition               = self._predicate_face_carrying_data() + " and observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction"
    self._patch_overlap.generator.receive_and_merge_condition  = self._predicate_face_carrying_data() + " and " \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction and " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridInitialisation"

    self.clawpack_Riemann_solver             = clawpack_Riemann_solver 
    self.Riemann_solver_implementation_files = Riemann_solver_implementation_files
    
    self._template_update_cell      = jinja2.Template( self.TemplateUpdateCell ); 
    pass
  
  


  def set_implementation(self,boundary_conditions=None,refinement_criterion=None,initial_conditions=None):
    """
    
      Pass in None if you don't want to overwrite any default.
      
    """
    if boundary_conditions!=None:
      self._boundary_conditions_implementation        = boundary_conditions
    if refinement_criterion!=None:
      self._refinement_criterion_implementation       = refinement_criterion
    if initial_conditions!=None: 
      self._initial_conditions_implementation         = initial_conditions

    
  def set_update_cell_implementation(self,
    kernel_implementation   = FV.CellUpdateImplementation_NestedLoop,
    memory_location         = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack
  ):
    if memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarchWithoutDelete or \
       memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapWithoutDelete or \
       memory_location==peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorWithoutDelete:
      raise Exception( "memory mode without appropriate delete chosen, i.e. this will lead to a memory leak" )

    self._reconstructed_array_memory_location = memory_location
    self._kernel_implementation               = kernel_implementation

  
  def add_entries_to_text_replacement_dictionary(self,d):
    """
     
     d: Dictionary of string to string
        in/out argument
    
    """
    d[ "TIME_STEP_SIZE" ]         = self._time_step_size
    d[ "CLAWPACK_RIEMANN_SOLVER"] = self.clawpack_Riemann_solver

    d[ "BOUNDARY_CONDITIONS_IMPLEMENTATION"]  = self._boundary_conditions_implementation
    d[ "REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
    d[ "INITIAL_CONDITIONS_IMPLEMENTATION"]   = self._initial_conditions_implementation

  
  def get_user_includes(self):
    return """
#include "exahype2/fv/Generic.h"
"""    
  
  
  def add_implementation_files_to_project(self,namespace,output):
    """
    
      Invoke add_implementation_files_to_project() of superclass and
      then add all the Fortran files to the makefile.
      
    """
    FV.add_implementation_files_to_project(self,namespace,output)
    for f in self.Riemann_solver_implementation_files:
      output.makefile.add_Fortran_file(f)
  
    