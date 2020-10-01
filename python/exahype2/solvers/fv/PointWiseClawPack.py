# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import FV
from .AbstractAoSWithOverlap1  import AbstractAoSWithOverlap1
 
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


class PointWiseClawPackFixedTimeStepSize(  FV, AbstractAoSWithOverlap1 ):
  """
  
    Very simple solver that can use ClawPack's point-wise Riemann solver.
  
  """
    
  
  """
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
      
  """
  RiemannSolverCall = """
    FL[0] = 0.0; FL[1] = 0.0;
    FR[0] = 0.0; FR[1] = 0.0;
"""


  
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size, clawpack_Riemann_solver, plot_grid_properties=False):
    """
    
      Instantiate a generic FV scheme with an overlap of 1.
      
    """
    #super(GenericRusanovFVFixedTimeStepSize,self).__init__(name, patch_size, unknowns, auxiliary_variables, min_h, max_h, flux, ncp, plot_grid_properties)
    FV.__init__(self, name, patch_size, 1, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)
    AbstractAoSWithOverlap1.__init__(self, flux=PDETerms.None_Implementation,ncp=PDETerms.None_Implementation)
    
    self._time_step_size = time_step_size
    
    self._template_adjust_cell      = self._get_template_adjust_cell()
    self._template_AMR              = self._get_template_AMR()
    self._template_handle_boundary  = self._get_template_handle_boundary()
    
    self.set_implementation()
    self.set_update_cell_implementation()

    self._patch_overlap.generator.send_condition               = self._predicate_face_carrying_data() + " and observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction"
    self._patch_overlap.generator.receive_and_merge_condition  = self._predicate_face_carrying_data() + " and " \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction and " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridInitialisation"

    pass
  
  
  def set_update_cell_implementation(self,
    function_call   = AbstractAoSWithOverlap1.CellUpdateImplementation_NestedLoop,
    memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarch
  ):
    if memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack and \
       memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.Heap and \
       memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarch and \
       memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.AcceleratorMemory:
      print( "WARNING: Selected memory allocation which does not delete allocated memory!" )
    
    self._reconstructed_array_memory_location = memory_location

    self._template_update_cell      = jinja2.Template( 
      self._get_template_update_cell( self.RiemannSolverCall )
    );
    #self._template_update_cell      = jinja2.Template( self._get_template_update_cell( self._rusanov_call + """
    #      QL, QR, x, dx, t, dt, normal, """ + 
    #  str(self._unknowns) + """, """ + str(self._auxiliary_variables) + """, FL, FR
    #    );
    #""" ))


  def set_implementation(self,boundary_conditions=PDETerms.User_Defined_Implementation,refinement_criterion=PDETerms.User_Defined_Implementation,initial_conditions=PDETerms.User_Defined_Implementation):
    """
      Don't use the superclass counterpart where you can also set fluxes and ncps.
      One day, I'll have to revise the code such that this variant is not available
      to all FV solvers.
    """
    AbstractAoSWithOverlap1.set_implementation(self,
      flux=PDETerms.None_Implementation,
      ncp=PDETerms.None_Implementation,
      eigenvalues=PDETerms.None_Implementation,
      boundary_conditions=boundary_conditions,refinement_criterion=refinement_criterion,initial_conditions=initial_conditions
    )    
    pass
    
  
  def add_entries_to_text_replacement_dictionary(self,d):
    """
     
     d: Dictionary of string to string
        in/out argument
    
    """
    self._add_generic_Rusanov_FV_entries_to_text_replacement_dictionary(d)

    d[ "TIME_STEP_SIZE" ] = self._time_step_size
    
    pass
  
  
  def get_user_includes(self):
    return """
#include "exahype2/fv/Generic.h"
#include "exahype2/fv/Rusanov.h"
"""    
  
  
    