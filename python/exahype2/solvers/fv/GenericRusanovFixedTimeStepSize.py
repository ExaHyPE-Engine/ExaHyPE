# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .FV                       import FV
from .AbstractAoSWithOverlap1  import AbstractAoSWithOverlap1
 
from .PDETerms import PDETerms

import peano4


class GenericRusanovFixedTimeStepSize( FV, AbstractAoSWithOverlap1 ):
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
            {% if use_gpu %}
            {{SOLVER_NAME}}::flux( Q, faceCentre, volumeH, t, normal, F, tarch::multicore::TargetDevice::MayRunOnGPU );
            {% else %}
            {{SOLVER_INSTANCE}}.flux( Q, faceCentre, volumeH, t, normal, F );
            {% endif %}
          },
          [] (
            double                                       Q[],
            const tarch::la::Vector<Dimensions,double>&  faceCentre,
            const tarch::la::Vector<Dimensions,double>&  volumeH,
            double                                       t,
            double                                       dt,
            int                                          normal
          ) -> double {
            {% if use_gpu %}
            return {{SOLVER_NAME}}::maxEigenvalue( Q, faceCentre, volumeH, t, normal, tarch::multicore::TargetDevice::MayRunOnGPU );
            {% else %}
            return {{SOLVER_INSTANCE}}.maxEigenvalue( Q, faceCentre, volumeH, t, normal);
            {% endif %}
          }, 
"""

  
  
  
  def __init__(self, name, patch_size, unknowns, auxiliary_variables, min_h, max_h, time_step_size, flux=PDETerms.None_Implementation, ncp=PDETerms.None_Implementation, plot_grid_properties=False):
    """
    
      Instantiate a generic FV scheme with an overlap of 1.
      
      
    """
    #super(GenericRusanovFVFixedTimeStepSize,self).__init__(name, patch_size, unknowns, auxiliary_variables, min_h, max_h, flux, ncp, plot_grid_properties)
    FV.__init__(self, name, patch_size, 1, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties)
    AbstractAoSWithOverlap1.__init__(self, flux, ncp)
    
    self._time_step_size = time_step_size
    
    self._template_adjust_cell      = self._get_template_adjust_cell()
    self._template_AMR              = self._get_template_AMR()
    self._template_handle_boundary  = self._get_template_handle_boundary()
    
    rusanov_call = ""
    if flux!=PDETerms.None_Implementation and ncp==PDETerms.None_Implementation:
      rusanov_call = self.RusanovCallWithFluxAndEigenvalues
    else:
      print( "ERROR: Combination of fluxes/operators not supported" )
      
      
    self._template_update_cell      = self._get_template_update_cell( rusanov_call + """
          QL, QR, x, dx, t, dt, normal, """ + 
      str(self._unknowns) + """, """ + str(self._auxiliary_variables) + """, FL, FR
        );
""" )
  
  
    #self._reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.Heap
    self._reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.HeapThroughTarch

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
  
  
  