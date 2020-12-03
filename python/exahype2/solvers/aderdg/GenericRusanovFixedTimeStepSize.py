# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .ADERDG import ADERDG
 
from .PDETerms import PDETerms

import peano4

import jinja2


class GenericRusanovFixedTimeStepSize( ADERDG, AbstractAoSWithOverlap1 ):
  """
  
    Probably the simplest solver you could think off. There's a few
    interesting things to try out with this one nevertheless: You 
    can inject symbolic flux/pde term implementations, or you can 
    alter the implementation variant of the cell update kernel. The
    latter can be combined with different dynamic memory allocation 
    schemes and thus offers quite some opportunities to tweak and 
    tune things. 
  
  """
    
  
  
  RusanovCallWithFluxAndEigenvalues = """
        ::exahype2::fv::splitRusanov1d(
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       F[]
          ) -> void {
            {{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, normal, F );
          },
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal
          ) -> double {
            return {{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
          }, 
"""


  RusanovCallWithNCPAndEigenvalues = """
        ::exahype2::fv::splitRusanov1d(
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       F[]
          ) -> void {
          },
          [] (
            double                                       Q[],
            double                                       gradQ[][Dimensions],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal,
            double                                       BgradQ[]
          ) -> void {
            {{SOLVER_INSTANCE}}.nonconservativeProduct( Q, gradQ, faceCentre, volumeH, t, normal, BgradQ );
          },
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal
          ) -> double {
            return {{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
          }, 
"""



  def __init__(self, name, order, unknowns, auxiliary_variables, polynomials, min_h, max_h, time_step_size, flux=PDETerms.User_Defined_Implementation, ncp=None, plot_grid_properties=False, kernel_implementation = AbstractAoSWithOverlap1.CellUpdateImplementation_NestedLoop):
    """

      Instantiate a generic ADER-DG scheme

    """
    #super(GenericRusanovFVFixedTimeStepSize,self).__init__(name, patch_size, unknowns, auxiliary_variables, min_h, max_h, flux, ncp, plot_grid_properties)
    ADERDG.__init__(self, self, name, order, unknowns, auxiliary_variables, polynomials, min_h, max_h, plot_grid_properties)
    #AbstractAoSWithOverlap1.__init__(self, kernel_implementation=kernel_implementation)

    self._time_step_size = time_step_size

    self._template_adjust_cell      = self._get_template_adjust_cell()
    self._template_AMR              = self._get_template_AMR()
    self._template_handle_boundary  = self._get_template_handle_boundary()

    self._rusanov_call = ""

    self.set_implementation(flux,ncp)
    self.set_update_cell_implementation()

    self._patch_overlap.generator.send_condition               = self._predicate_face_carrying_data() + " and observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction"
    self._patch_overlap.generator.receive_and_merge_condition  = self._predicate_face_carrying_data() + " and " \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction and " + \
      "observers::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridInitialisation"

    pass
  

  def set_implementation(self,flux=PDETerms.User_Defined_Implementation,ncp=None,eigenvalues=PDETerms.User_Defined_Implementation,boundary_conditions=PDETerms.User_Defined_Implementation,refinement_criterion=PDETerms.User_Defined_Implementation,initial_conditions=PDETerms.User_Defined_Implementation):
    """
    
     Wrapper around superclass
     
    """
    AbstractAoSWithOverlap1.set_implementation(self,flux,ncp,eigenvalues,boundary_conditions,refinement_criterion,initial_conditions)
    
    if self._flux_implementation!=PDETerms.None_Implementation and self._ncp_implementation==PDETerms.None_Implementation:
      self._rusanov_call = self.RusanovCallWithFluxAndEigenvalues
    elif self._flux_implementation==PDETerms.None_Implementation and self._ncp_implementation!=PDETerms.None_Implementation:
      self._rusanov_call = self.RusanovCallWithNCPAndEigenvalues
    else:
      raise Exception("ERROR: Combination of fluxes/operators not supported. flux: {} ncp: {}".format(flux, ncp))

    self._construct_template_update_cell()


  def _construct_template_update_cell(self):
    self._template_update_cell      = jinja2.Template( self._get_template_update_cell( self._rusanov_call + """
          QL, QR, x, dx, t, dt, normal, """ + 
      str(self._unknowns) + """, """ + str(self._auxiliary_variables) + """, FL, FR
        );
""" ))
    
  
  def set_update_cell_implementation(self,
    function_call   = AbstractAoSWithOverlap1.CellUpdateImplementation_NestedLoop,
    memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarch
  ):
    if memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack and \
       memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.Heap and \
       memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarch and \
       memory_location!=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.Accelerator:
      print( "WARNING: Selected memory allocation which does not delete allocated memory!" )
    
    self._reconstructed_array_memory_location = memory_location
    self._construct_template_update_cell()
    
  
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
#include "exahype2/fv/Rusanov.h"
"""    
  
  
  
