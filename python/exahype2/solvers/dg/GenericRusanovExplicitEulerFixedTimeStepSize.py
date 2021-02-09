# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .DG       import DG
from .DG       import AbstractDGActionSet
from .PDETerms import PDETerms

from peano4.solversteps.ActionSet import ActionSet

import peano4
import exahype2

import jinja2


class ApplyRiemannSolveToFaces(AbstractDGActionSet):
  TemplateRiemannSolve = jinja2.Template( """
/*
    if (repositories::{{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::RiemannProblemSolve) {
      // @todo Have to think about this one


      //not marker.isRefined() and fineGridFaceLabel.getBoundary()
*/      
  """)

  def __init__(self,solver):
    AbstractDGActionSet.__init__(self,solver)
  
  
  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)      
      result = self.TemplateRiemannSolve.render(**d)
      pass 
    return result


class UpdateCell(AbstractDGActionSet):
  TemplateUpdateCell = jinja2.Template( """
    switch (repositories::{{SOLVER_INSTANCE}}.getSolverState() ) {
      case {{SOLVER_NAME}}::SolverState::GridConstruction:
        assertionMsg( false, "should not be entered" );
        break;
      case {{SOLVER_NAME}}::SolverState::GridInitialisation:
        assertionMsg( false, "should not be entered" );
        break;
      case {{SOLVER_NAME}}::SolverState::VolumeAndBoundaryIntegral:
        {
          dg::
        }
        break;
      case {{SOLVER_NAME}}::SolverState::TimeStep:
        {
        }
        break;
      case {{SOLVER_NAME}}::SolverState::Plotting:
        assertionMsg( false, "should not be entered" );
        break;
    }
  """)    
  
  def __init__(self,solver):
    AbstractDGActionSet.__init__(self,solver)
    
    
  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)      
      result = self.TemplateUpdateCell.render(**d)
      pass 
    return result


  def get_includes(self):
    return AbstractDGActionSet.get_includes(self) + """
#include "exahype2/aderdg/PredictorAoS.h"
#include "exahype2/aderdg/CorrectorAoS.h"
"""



class GenericRusanovExplicitEulerFixedTimeStepSize( DG ):
  """
  
    Probably the simplest ADER-DG solver implementation you could think off. 
  
  """


  def __init__(self, 
    name, order, unknowns, auxiliary_variables, polynomials, min_h, max_h, time_step_size, 
    flux=PDETerms.User_Defined_Implementation, ncp=None, sources=None,
    plot_grid_properties=False
  ):
    """

      Instantiate ADER-DG in a non-fused variant

    """
    DG.__init__(self, name, order, unknowns, auxiliary_variables, polynomials, min_h, max_h, plot_grid_properties)

    #self._face_flux_along_normal = peano4.datamodel.Patch( (2*(order+1),order+1,order+1), unknowns+auxiliary_variables, self._unknown_identifier() + "FluxExtrapolation" )

    self._time_step_size = time_step_size

    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.User_Defined_Implementation
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation
    self._sources_implementation              = PDETerms.None_Implementation

    self._action_set_update_cell = UpdateCell(self)
    self._action_set_update_face = ApplyRiemannSolveToFaces(self)

    # braucht kein Mensch
    self._rusanov_call = ""
    self._reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack

    self.set_implementation(flux=flux,ncp=ncp,sources=sources)
    
    
    #self._face_spacetime_solution.generator.send_condition               = "false"
    #self._face_spacetime_solution.generator.receive_and_merge_condition  = "false"
    pass

  
  def set_implementation(self,flux=None,ncp=None,sources=None,eigenvalues=None,boundary_conditions=None,refinement_criterion=None,initial_conditions=None):
    """
      If you pass in User_Defined, then the generator will create C++ stubs 
      that you have to befill manually. If you pass in None_Implementation, it 
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. If you pass in None, then the 
      set value so far won't be overwritten.
      
      Please note that not all options are supported by all solvers. You 
      cannot set ncp and fluxes for the ClawPack Riemann solvers, e.g.
    """
    if flux!=None:
      self._flux_implementation        = flux
    if ncp!=None:
      self._ncp_implementation         = ncp
    if eigenvalues!=None:    
      self._eigenvalues_implementation = eigenvalues
    if sources!=None:    
      self._sources_implementation     = sources  
    if boundary_conditions!=None:
      self._boundary_conditions_implementation        = boundary_conditions
    if refinement_criterion!=None:
      self._refinement_criterion_implementation       = refinement_criterion
    if initial_conditions!=None: 
      self._initial_conditions_implementation         = initial_conditions
    
    #if self._flux_implementation!=PDETerms.None_Implementation and self._ncp_implementation==PDETerms.None_Implementation:
    #  self._rusanov_call = self.RusanovCallWithFlux
    #elif self._flux_implementation==PDETerms.None_Implementation and self._ncp_implementation!=PDETerms.None_Implementation:
    #  self._rusanov_call = self.RusanovCallWithNCP
    #elif self._flux_implementation!=PDETerms.None_Implementation and self._ncp_implementation!=PDETerms.None_Implementation:
    #  self._rusanov_call = self.RusanovCallWithFluxAndNCP
    #else:
    #  raise Exception("ERROR: Combination of fluxes/operators not supported. flux: {} ncp: {}".format(flux, ncp))

    #self._construct_template_update_cell()
    pass
    
  
  def add_entries_to_text_replacement_dictionary(self,d):
    """
     
     d: Dictionary of string to string
        in/out argument
    
    """
    d[ "TIME_STEP_SIZE" ]                     = self._time_step_size

    d[ "FLUX_IMPLEMENTATION"]                 = self._flux_implementation
    d[ "NCP_IMPLEMENTATION"]                  = self._ncp_implementation
    d[ "SOURCES_IMPLEMENTATION"]              = self._sources_implementation
    d[ "EIGENVALUES_IMPLEMENTATION"]          = self._eigenvalues_implementation
    d[ "BOUNDARY_CONDITIONS_IMPLEMENTATION"]  = self._boundary_conditions_implementation
    d[ "REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
    d[ "INITIAL_CONDITIONS_IMPLEMENTATION"]   = self._initial_conditions_implementation

    pass


  def get_user_includes(self):
    return """
#include "exahype2/aderdg/Generic.h" 

#include <algorithm>
"""    


  def add_to_Peano4_datamodel( self, datamodel ):
    DG.add_to_Peano4_datamodel( self, datamodel )
    #datamodel.add_face(self._face_flux_along_normal)



  def add_use_data_statements_to_Peano4_solver_step(self, step):
    DG.add_use_data_statements_to_Peano4_solver_step( self, step )
    #step.use_face(self._face_flux_along_normal)


  def add_actions_to_perform_time_step(self, step):
    DG.add_actions_to_perform_time_step( self, step )
    step.add_action_set( ApplyRiemannSolveToFaces(self) )
