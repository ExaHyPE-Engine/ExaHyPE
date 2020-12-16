# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
from .ADERDG  import ADERDG
from .PDETerms import PDETerms

from peano4.solversteps.ActionSet import ActionSet

import peano4
import exahype2

import jinja2


#
# @todo Boundary treatment is missing. Could go in here
#
class ApplyRiemannSolveToFaces(ActionSet):
  TemplateRiemannSolve = jinja2.Template( """
    if ({{SOLVER_INSTANCE}}.getSolverState()=={{SOLVER_NAME}}::SolverState::RiemannProblemSolve) {
      // @todo Have to think about this one
      ::exahype2::aderdg::solveSpaceTimeRiemannProblem_GaussLegendre_AoS2d(
        [&](
          double * __restrict__ QL,
          double * __restrict__ QR,
          const tarch::la::Vector<Dimensions,double>&  x,
          double                                       t,
          double                                       dt,
          int                                          normal,
          double * __restrict__ FL,
          double * __restrict__ FR
        )->void {
        },
        marker.x(),
        marker.h(),
        {{SOLVER_INSTANCE}}.getMinTimeStamp(), 
        {{SOLVER_INSTANCE}}.getMinTimeStepSize(), 
        {{ORDER}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
        {{SOLVER_INSTANCE}}.QuadraturePoints,
        {{SOLVER_INSTANCE}}.QuadratureWeights,
        marker.getSelectedFaceNumber() % Dimensions,
        fineGridFace{{SOLVER_NAME}}QSolutionExtrapolation.value,
        fineGridFace{{SOLVER_NAME}}QFluxExtrapolation.value,
        fineGridFace{{SOLVER_NAME}}QRiemannSolveResult.value
      );
    }
  """)

  def __init__(self,solver):
    """
    
    solver: NonFusedGenericRusanovFixedTimeStepSize
      Reference to creating class 
    
    """
    self._solver = solver
    pass
  
  
  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_FACE_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)      
      result = self.TemplateRiemannSolve.render(**d)
      pass 
    return result


  def get_includes(self):
    return """
#include <functional>
""" + self._solver._get_default_includes() + self._solver.get_user_includes() 



class NonFusedGenericRusanovFixedTimeStepSize( ADERDG ):
  """
  
    Probably the simplest ADER-DG solver implementation you could think off. 
  
  """
  TemplateUpdateCell = """
    switch ({{SOLVER_INSTANCE}}.getSolverState() ) {
      case {{SOLVER_NAME}}::SolverState::GridConstruction:
        assertionMsg( false, "should not be entered" );
        break;
      case {{SOLVER_NAME}}::SolverState::GridInitialisation:
        assertionMsg( false, "should not be entered" );
        break;
      case {{SOLVER_NAME}}::SolverState::Prediction:
        {
          #if Dimensions==2
          double spaceTimeQ[ ({{ORDER}}+1)*({{ORDER}}+1)*({{ORDER}}+1)*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})  ];
          #else
          double spaceTimeQ[ ({{ORDER}}+1)*({{ORDER}}+1)*({{ORDER}}+1)*({{ORDER}}+1)*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}})  ];
          #endif
          
          ::exahype2::aderdg::spaceTimePredictor_GaussLegendre_AoS2d(
             [&](
               double * __restrict__                        Q, 
               const tarch::la::Vector<Dimensions,double>&  x, 
               double                                       t, 
               double * __restrict__                        F
             )->void {
               {{SOLVER_INSTANCE}}.flux(Q,x,t,F);
             },
             marker.x(),
             marker.h(),
             {{SOLVER_INSTANCE}}.getMinTimeStamp(), 
             {{SOLVER_INSTANCE}}.getMinTimeStepSize(), 
             {{ORDER}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
             {{SOLVER_INSTANCE}}.QuadraturePoints,
             {{SOLVER_INSTANCE}}.QuadratureWeights,
             fineGridCell{{SOLVER_NAME}}Q.value,
             spaceTimeQ
          );

          #if Dimensions==2
          ::exahype2::aderdg::projectSpaceTimeSolutionOntoFace_GaussLegendre_AoS2d(
          #else
          ::exahype2::aderdg::projectSpaceTimeSolutionOntoFace_GaussLegendre_AoS3d(
          #endif
             [&](
               double * __restrict__                        Q, 
               const tarch::la::Vector<Dimensions,double>&  x, 
               double                                       t, 
               double * __restrict__                        F
             )->void {
               {{SOLVER_INSTANCE}}.flux(Q,x,t,F);
             },
             marker.x(),
             marker.h(),
             {{SOLVER_INSTANCE}}.getMinTimeStamp(), 
             {{SOLVER_INSTANCE}}.getMinTimeStepSize(), 
             {{ORDER}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
             {{SOLVER_INSTANCE}}.QuadraturePoints,
             {{SOLVER_INSTANCE}}.QuadratureWeights,
             spaceTimeQ,
             fineGridFaces{{SOLVER_NAME}}QSolutionExtrapolation(0).value,
             fineGridFaces{{SOLVER_NAME}}QSolutionExtrapolation(1).value,
             fineGridFaces{{SOLVER_NAME}}QSolutionExtrapolation(2).value,
             fineGridFaces{{SOLVER_NAME}}QSolutionExtrapolation(3).value,
             #if Dimensions==3
             fineGridFaces{{SOLVER_NAME}}QSolutionExtrapolation(4).value,
             fineGridFaces{{SOLVER_NAME}}QSolutionExtrapolation(5).value,
             #endif
             fineGridFaces{{SOLVER_NAME}}QFluxExtrapolation(0).value,
             fineGridFaces{{SOLVER_NAME}}QFluxExtrapolation(1).value,
             fineGridFaces{{SOLVER_NAME}}QFluxExtrapolation(2).value,
             fineGridFaces{{SOLVER_NAME}}QFluxExtrapolation(3).value
             #if Dimensions==3
             ,
             fineGridFaces{{SOLVER_NAME}}QFluxExtrapolation(4).value,
             fineGridFaces{{SOLVER_NAME}}QFluxExtrapolation(5).value
             #endif
          );

          ::exahype2::aderdg::timeIntegration_GaussLegendre_AoS2d(
             marker.x(),
             marker.h(),
             {{SOLVER_INSTANCE}}.getMinTimeStamp(), 
             {{SOLVER_INSTANCE}}.getMinTimeStepSize(), 
             {{ORDER}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
             {{SOLVER_INSTANCE}}.QuadraturePoints,
             {{SOLVER_INSTANCE}}.QuadratureWeights,
             spaceTimeQ,
             fineGridCell{{SOLVER_NAME}}QNew.value
          );
        }
        break;
      case {{SOLVER_NAME}}::SolverState::RiemannProblemSolve:
        break;
      case {{SOLVER_NAME}}::SolverState::Correction:
        {
          #if Dimensions==2
          ::exahype2::aderdg::addSpaceTimeRiemannSolutionToPrediction_GaussLegendre_AoS2d(
          #else
          ::exahype2::aderdg::addSpaceTimeRiemannSolutionToPrediction_GaussLegendre_AoS3d(
          #endif
            marker.x(),
            marker.h(),
            {{SOLVER_INSTANCE}}.getMinTimeStamp(), 
            {{SOLVER_INSTANCE}}.getMinTimeStepSize(), 
            {{ORDER}}, {{NUMBER_OF_UNKNOWNS}}, {{NUMBER_OF_AUXILIARY_VARIABLES}},
            {{SOLVER_INSTANCE}}.QuadraturePoints,
            {{SOLVER_INSTANCE}}.QuadratureWeights,
            fineGridFaces{{SOLVER_NAME}}QRiemannSolveResult(0).value,
            fineGridFaces{{SOLVER_NAME}}QRiemannSolveResult(1).value,
            fineGridFaces{{SOLVER_NAME}}QRiemannSolveResult(2).value,
            fineGridFaces{{SOLVER_NAME}}QRiemannSolveResult(3).value,
            #if Dimensions==3
            fineGridFaces{{SOLVER_NAME}}QRiemannSolveResult(4).value,
            fineGridFaces{{SOLVER_NAME}}QRiemannSolveResult(5).value,
            #endif
            fineGridCell{{SOLVER_NAME}}QNew.value
          );

          std::copy_n(
            fineGridCell{{SOLVER_NAME}}QNew.value,
            #if Dimensions==2
            ({{ORDER}}+1)*({{ORDER}}+1)*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}),
            #else
            ({{ORDER}}+1)*({{ORDER}}+1)*({{ORDER}}+1)*({{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}}),
            #endif
            fineGridCell{{SOLVER_NAME}}Q.value
          );
        }
        break;
      case {{SOLVER_NAME}}::SolverState::Plotting:
        assertionMsg( false, "should not be entered" );
        break;
    }
  """      


  def __init__(self, name, order, unknowns, auxiliary_variables, polynomials, min_h, max_h, time_step_size, flux=PDETerms.User_Defined_Implementation, ncp=None, plot_grid_properties=False):
    """

      Instantiate ADER-DG in a non-fused variant

    """
    ADERDG.__init__(self, name, order, unknowns, auxiliary_variables, polynomials, min_h, max_h, plot_grid_properties)

    self._face_flux_along_normal = peano4.datamodel.Patch( (2*(order),order,order), unknowns+auxiliary_variables, self._unknown_identifier() + "FluxExtrapolation" )

    self._time_step_size = time_step_size

    self._flux_implementation                 = PDETerms.None_Implementation
    self._ncp_implementation                  = PDETerms.None_Implementation
    self._eigenvalues_implementation          = PDETerms.User_Defined_Implementation
    self._boundary_conditions_implementation  = PDETerms.User_Defined_Implementation
    self._refinement_criterion_implementation = PDETerms.Empty_Implementation
    self._initial_conditions_implementation   = PDETerms.User_Defined_Implementation

    self._template_update_cell     = jinja2.Template( self.TemplateUpdateCell )

    # braucht kein Mensch
    self._rusanov_call = ""
    self._reconstructed_array_memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack

    self.set_implementation(flux=flux,ncp=ncp)
    
    
    #self._face_spacetime_solution.generator.send_condition               = "false"
    #self._face_spacetime_solution.generator.receive_and_merge_condition  = "false"
    pass

  
  def set_implementation(self,flux=None,ncp=None,eigenvalues=None,boundary_conditions=None,refinement_criterion=None,initial_conditions=None):
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
    ADERDG.add_to_Peano4_datamodel( self, datamodel )
    datamodel.add_face(self._face_flux_along_normal)



  def add_use_data_statements_to_Peano4_solver_step(self, step):
    ADERDG.add_use_data_statements_to_Peano4_solver_step( self, step )
    step.use_face(self._face_flux_along_normal)


  def add_actions_to_perform_time_step(self, step):
    ADERDG.add_actions_to_perform_time_step( self, step )
    step.add_action_set( ApplyRiemannSolveToFaces(self) )
