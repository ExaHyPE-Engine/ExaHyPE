
# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

 
import peano4.toolbox.blockstructured
import peano4.datamodel
import peano4.output
import exahype2.grid
import exahype2.solvers.new_ader.actionsets
from exahype2.solvers.new_ader.LagrangeBasis import GaussLegendreBasis, GaussLobattoBasis
from .PDETerms import PDETerms
from .kernels import get_face_overlap_merge_implementation

import os
from abc import abstractmethod

from enum import IntEnum
import jinja2
from exahype2.solvers.new_ader.actionsets.Prediction import Prediction
from exahype2.solvers.new_ader.actionsets.Correction import Correction


class Polynomials(IntEnum):
  Gauss_Legendre = 0,
  Gauss_Lobatto = 1
  

    

class ADERDG( object ):
  def __init__(self, name, order, polynomials, unknowns, auxiliary_variables, min_cell_h, max_cell_h, is_linear, plot_grid_properties=False):

    self._name = name
    
    self._order = order
    self._polynomials = polynomials
    self._unknowns = unknowns
    self._auxiliary_variables = auxiliary_variables

    self._min_cell_h         = min_cell_h
    self._max_cell_h         = max_cell_h 
    if min_cell_h>max_cell_h:
      raise Exception( "Error: min_cell_h (" + str(min_cell_h) + ") is bigger than max_cell_h (" + str(max_cell_h) + ")" )
    
    self._is_linear = is_linear
#    if is_linear==True: self._is_linear="true"
#    else: self._is_linear=="false"

    self._plot_grid_properties = plot_grid_properties    

    self.solver_constants_         = ""
    self.user_action_set_includes  = ""
    self.user_solver_includes      = ""
    self.plot_description = ""    
    
    self._compute_time_step_size              = None
    self._compute_new_time_step_size          = None
    
    if polynomials is Polynomials.Gauss_Legendre:
      self._basis = GaussLegendreBasis(order+1)
    elif polynomials is Polynomials.Gauss_Lobatto:
      self._basis = GaussLobattoBasis(order+1)
    else:
      raise Exception( "No proper basis chosen: {}".format(polynomials) )

    self._flux_implementation                 = None
    self._ncp_implementation                  = None
    self._eigenvalues_implementation          = None
    self._source_term_implementation          = None
    self._point_sources_implementation         = None
    self._boundary_conditions_implementation  = None
    self._refinement_criterion_implementation = None
    self._initial_conditions_implementation   = None

    self._compute_time_step_size              = "#error Not yet defined"
    
    self._abstract_solver_user_declarations        = ""
    self._abstract_solver_user_definitions         = ""
    self._solver_user_declarations                 = ""
    self._solver_user_definitions                  = ""

    self._start_time_step_implementation           = ""
    self._finish_time_step_implementation          = ""
    
    self._constructor_implementation = ""
    
    # ADER-DG CFL factors        
    # Up to order 4, computed via von Neumann analysis [1]; above, determined empirically by M. Dumbser)
    # [1] M. Dumbser, D. S. Balsara, E. F. Toro, and C.-D. Munz, 
    # ‘A unified framework for the construction of one-step finite volume and discontinuous Galerkin schemes on unstructured meshes’, Journal of Computational Physics, vol. 227, no. 18, pp. 8209–8253, Sep. 2008.
    self._cflAder = [
      1.0, 0.33, 0.17,
      0.1, 0.069, 0.045,
      0.038, 0.03, 0.02,
      0.015, 0.008821428571428572, 0.005235326086956522,
      0.0031307249999999996, 0.0018842326388888888, 0.001140285614224138, 
      0.0006933672202620968]
    
    # safety factor for CFL (Courant-Friedrichs-Lewy) condition
    self._cflSafetyFactor = 0.0
    
    

    self._action_set_postprocess_solution = None

    self.create_data_structures()
    self.create_action_sets()
    
            
  @abstractmethod
  def create_data_structures(self):
    
    self._number_of_face_projections = 1;

    #current time step contains the solution, projections are respectively the projection of the solution and of the fluxes as predicted by the space-time predictor
    self._current_time_step             = peano4.datamodel.Patch( (self._order+1,self._order+1,self._order+1), self._unknowns+self._auxiliary_variables, self._unknown_identifier() )
    self._estimates_projection          = peano4.datamodel.Patch( (2,self._order+1,self._order+1),  self._unknowns+self._auxiliary_variables, self._unknown_identifier() + "Estimates" )
    self._flux_estimates_projection     = peano4.datamodel.Patch( (2,self._order+1,self._order+1),  self._unknowns, self._unknown_identifier() + "FluxEstimates" )    
        
    #custom merge implementation takes into account that the faces project such that the left cell projects to the left half of the face and the right cell projects to the right half
    self._estimates_projection.generator.merge_method_definition = get_face_overlap_merge_implementation(self._estimates_projection)
    self._estimates_projection.generator.includes     += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 
"""

    self._flux_estimates_projection.generator.merge_method_definition = get_face_overlap_merge_implementation(self._flux_estimates_projection)
    self._flux_estimates_projection.generator.includes     += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 
"""

    self._current_time_step.generator.store_persistent_condition = self._store_cell_data_default_guard()
    self._current_time_step.generator.load_persistent_condition  = self._load_cell_data_default_guard()
    
    self._estimates_projection.generator.store_persistent_condition = self._store_face_data_default_guard()
    self._estimates_projection.generator.load_persistent_condition  = self._load_face_data_default_guard()

    #get overwritten in subclasses to take into account when to send or receive faces
    self._estimates_projection.generator.send_condition               = "false"
    self._estimates_projection.generator.receive_and_merge_condition  = "false"

    self._flux_estimates_projection.generator.store_persistent_condition = self._store_face_data_default_guard()
    self._flux_estimates_projection.generator.load_persistent_condition  = self._load_face_data_default_guard()

    self._flux_estimates_projection.generator.send_condition               = "false"
    self._flux_estimates_projection.generator.receive_and_merge_condition  = "false"
    
    self._current_time_step.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""

    self._estimates_projection.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    

    self._flux_estimates_projection.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""

    self._cell_label = exahype2.grid.create_cell_label( self._name )
    self._face_label = exahype2.grid.create_face_label( self._name )      



  @abstractmethod
  def create_action_sets(self):

    #sets initial conditions for the solution
    self._action_set_initial_conditions = exahype2.solvers.new_ader.actionsets.InitialCondition(self, self._store_cell_data_default_guard(), "true" )
    self._action_set_initial_conditions_for_grid_construction  = exahype2.solvers.new_ader.actionsets.InitialCondition(self, self._store_cell_data_default_guard(), "false")
    
    #handles adaptive mesh refinement
    self._action_set_AMR = exahype2.solvers.new_ader.actionsets.AdaptivityCriterion(solver=self, 
         guard=self._store_cell_data_default_guard() + " and (repositories::" + self.get_name_of_global_instance() + ".isLastGridSweepOfTimeStep() or repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation)", 
         build_up_new_refinement_instructions=True,  implement_previous_refinement_instructions=True )
    self._action_set_AMR_commit_without_further_analysis  = exahype2.solvers.new_ader.actionsets.AdaptivityCriterion(solver=self, guard=self._store_cell_data_default_guard(), build_up_new_refinement_instructions=False, implement_previous_refinement_instructions=True )
        
    self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = exahype2.solvers.new_ader.actionsets.DynamicAMR(solver=self,guard=self._store_cell_data_default_guard())
    if self._action_set_postprocess_solution==None:
          self._action_set_postprocess_solution                                       = exahype2.solvers.new_ader.actionsets.EmptyPostprocessSolution(solver=self)

    
    #updates faces and cells (whether at boundary, refined etc.)
    self._action_set_update_face_label = exahype2.grid.UpdateFaceLabel( self._name )  
    self._action_set_update_cell_label = exahype2.grid.UpdateCellLabel( self._name ) 

    #all the actual computation of the solution happens in these
    # predictor extrapolates the solution from time t onto time-nodes, then projects this 
    #   extrapolated solution to the faces.
    # correction uses the extrapolated face solutions from the predictor and the neighboring
    #   cells to compute a solution at the next timestep
    self._action_set_prediction = Prediction(self, self._store_cell_data_default_guard())
    self._action_set_correction = Correction(self, self._store_cell_data_default_guard())



  def create_readme_descriptor(self, domain_offset, domain_size):
    return """ ExaHyPE 2 Ader-DG solver of order: """ + str(self._order) + """
    with a domain size of: """ + str(domain_size) + """ and a domain offset of: """ + str(domain_offset)


  def get_user_action_set_includes(self):
    return self.user_action_set_includes

  
  def get_user_solver_includes(self):
    return self.user_solver_includes



  def _store_cell_data_default_guard(self):
    return "not marker.willBeRefined() " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction"
  
  
  def _load_cell_data_default_guard(self):
    return "not marker.hasBeenRefined() " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridInitialisation"


  def _store_face_data_default_guard(self):
    return "not marker.willBeRefined() " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction"
  
  
  def _load_face_data_default_guard(self):
    return "not marker.hasBeenRefined() " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridInitialisation"
  
  
  def _unknown_identifier(self):
    return self._name+"Q"
  

  def get_name_of_global_instance(self):
    return "InstanceOf" + self._name
       
  def _get_default_includes(self):
    return """
#include "tarch/la/Vector.h" 

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "repositories/SolverRepository.h"
"""
       
       
  def add_to_Peano4_datamodel( self, datamodel, verbose ):       
    """
    if verbose:
      print( "Patch data" )
      print( "----------" )
      print( str(self._current_time_step) )
      print( "Patch overlap data" )
      print( "----------" )
      print( str(self._current_time_step_projection) )
    """  
    datamodel.add_cell(self._cell_label)
    datamodel.add_cell(self._current_time_step)
    datamodel.add_face(self._face_label)
    datamodel.add_face(self._estimates_projection)
    datamodel.add_face(self._flux_estimates_projection)
  
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    step.use_cell(self._cell_label)
    step.use_cell(self._current_time_step)
    step.use_face(self._face_label)
    step.use_face(self._estimates_projection)
    step.use_face(self._flux_estimates_projection)

  def add_actions_to_init_grid(self, step):
    step.add_action_set( self._action_set_initial_conditions )
    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )
    step.add_action_set( self._action_set_AMR )
    step.add_action_set( self._action_set_postprocess_solution )

  
  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    step.add_action_set( self._action_set_initial_conditions_for_grid_construction )    
    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )
    if evaluate_refinement_criterion:
      step.add_action_set( self._action_set_AMR )
    else:
      step.add_action_set( self._action_set_AMR_commit_without_further_analysis )

  
  def add_actions_to_plot_solution(self, step, output_path):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    
    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )    
    
    mapping = []    
    for z in self._basis.quadrature_points():
      for y in self._basis.quadrature_points():
        for x in self._basis.quadrature_points():
          mapping.append( (x,y,z) )

    
    step.add_action_set( peano4.toolbox.blockstructured.PlotPatchesInPeanoBlockFormat( 
      filename=output_path + "solution-" + self._name, 
      patch=self._current_time_step, 
      dataset_name=self._unknown_identifier(), 
      description=self.plot_description,
      mapping=mapping,
      guard="repositories::plotFilter.plotPatch(marker) and " + self._load_cell_data_default_guard(),
      additional_includes="""
#include "exahype2/PlotFilter.h"
#include "../repositories/SolverRepository.h"
""",
      precision="PlotterPrecision",
      time_stamp_evaluation="repositories::getMinTimeStamp()", plot_cell_data=False
    ))

    if self._plot_grid_properties:    
      step.add_action_set( peano4.toolbox.PlotGridInPeanoBlockFormat( 
        filename = output_path + "grid-" + self._name,
        cell_unknown=None,
        guard_guard="repositories::plotFilter.plotPatch(marker) and " + self._load_cell_data_default_guard(),
        additional_includes="""
#include "exahype2/PlotFilter.h"
#include "../repositories/SolverRepository.h"
""",
        plot_cell_data=False
      ))      
    pass
  
  def add_actions_to_perform_time_step(self, step):  
    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )
    step.add_action_set( self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement )
        
    step.add_action_set(self._action_set_prediction)
    step.add_action_set(self._action_set_correction)

    step.add_action_set( self._action_set_AMR )
    step.add_action_set( self._action_set_postprocess_solution )

  
    
  def set_plot_description(self,description):
    self.plot_description = description


  def add_implementation_files_to_project(self,namespace,output):
    """
    
     The ExaHyPE2 project will call this operation when it sets
     up the overall environment.
     
     This routine is typically not invoked by a user.

     output: peano4.output.Output
     
    """
    templatefile_prefix = os.path.dirname( os.path.realpath(__file__) ) + "/"

    if self._solver_template_file_class_name is None:
      templatefile_prefix += self.__class__.__name__
    else:
      templatefile_prefix += self._solver_template_file_class_name

    abstractHeaderDictionary = {}
    implementationDictionary = {}
    self._init_dictionary_with_default_parameters(abstractHeaderDictionary)
    self._init_dictionary_with_default_parameters(implementationDictionary)
    self.add_entries_to_text_replacement_dictionary(abstractHeaderDictionary)
    self.add_entries_to_text_replacement_dictionary(implementationDictionary)

    generated_abstract_header_file = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + "Abstract.template.h",
      templatefile_prefix + "Abstract.template.cpp",
      "Abstract" + self._name, 
      namespace,
      ".", 
      abstractHeaderDictionary,
      True,
      True)
    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".template.h",
      templatefile_prefix + ".template.cpp",
      self._name, 
      namespace,
      ".", 
      implementationDictionary,
      False,
      True)

    output.add( generated_abstract_header_file )
    output.add( generated_solver_files )
    output.makefile.add_cpp_file( "Abstract" + self._name + ".cpp" )
    output.makefile.add_cpp_file( self._name + ".cpp" )

  def set_solver_constants(self, datastring): self.solver_constants_ = datastring

  def add_solver_constants(self, datastring): self.solver_constants_ += datastring  
  
  
  @property
  def patch_size(self):
    return self._patch_size


  @property
  def unknowns(self):
    return self._unknowns


  @property
  def auxiliary_variables(self):
    return self._auxiliary_variables


  @patch_size.setter
  def patch_size(self, value):
    self._patch_size = value
    self.create_data_structures()
    self.create_action_sets()


  @patch_size.setter
  def unknowns(self, value):
    self._unknowns = value
    self.create_data_structures()
    self.create_action_sets()


  @patch_size.setter
  def auxiliary_variables(self, value):
    self._auxiliary_variables = value
    self.create_data_structures()
    self.create_action_sets()
  
  
  
  @abstractmethod
  def add_entries_to_text_replacement_dictionary(self,d):
    pass  

  def _init_dictionary_with_default_parameters(self,d):
    d["ORDER"]                          = self._order
    d["SOLVER_INSTANCE"]                = self.get_name_of_global_instance()
    d["SOLVER_NAME"]                    = self._name
    d["UNKNOWN_IDENTIFIER"]             = self._unknown_identifier()
    d["NUMBER_OF_UNKNOWNS"]             = self._unknowns
    d["NUMBER_OF_AUXILIARY_VARIABLES"]  = self._auxiliary_variables
    
    d["CFL_SAFETY_FACTOR"]  = self._cflSafetyFactor
    d["CFL_ADER"]           = self._cflAder[self._order]
    
    d[ "NUMBER_OF_DOFS_PER_CELL_2D" ] = (self._order+1) * (self._order+1);
    d[ "NUMBER_OF_DOFS_PER_CELL_3D" ] = (self._order+1) * (self._order+1) * (self._order+1);

    d["USE_GAUSS_LOBATTO"] = ("true" if self._polynomials is Polynomials.Gauss_Lobatto else "false")
    d["POLYNOMIAL_TYPE"] = ("lobatto" if self._polynomials is Polynomials.Gauss_Lobatto else "legendre")

    d[ "ASSERTION_WITH_1_ARGUMENTS" ] = "nonCriticalAssertion1"
    d[ "ASSERTION_WITH_2_ARGUMENTS" ] = "nonCriticalAssertion2"
    d[ "ASSERTION_WITH_3_ARGUMENTS" ] = "nonCriticalAssertion3"
    d[ "ASSERTION_WITH_4_ARGUMENTS" ] = "nonCriticalAssertion4"
    d[ "ASSERTION_WITH_5_ARGUMENTS" ] = "nonCriticalAssertion5"
    d[ "ASSERTION_WITH_6_ARGUMENTS" ] = "nonCriticalAssertion6"
    
    d["IS_LINEAR"] = self._is_linear

    if (self._min_cell_h > self._max_cell_h ):
      raise Exception( "min/max h are inconsistent" )
    d[ "MAX_CELL_H"] = self._max_cell_h
    d[ "MIN_CELL_H"] = self._min_cell_h

    d[ "SOLVER_CONSTANTS" ] = self.solver_constants_
    d[ "SOLVER_INCLUDES" ]  = self.get_user_solver_includes()
    
    d[ "BOUNDARY_CONDITIONS_IMPLEMENTATION"]  = self._boundary_conditions_implementation
    d[ "REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
    d[ "INITIAL_CONDITIONS_IMPLEMENTATION"]   = self._initial_conditions_implementation
    d[ "ABSTRACT_SOLVER_USER_DECLARATIONS" ]  = jinja2.Template(self._abstract_solver_user_declarations, undefined=jinja2.DebugUndefined).render( **d )
    d[ "ABSTRACT_SOLVER_USER_DEFINITIONS" ]   = jinja2.Template(self._abstract_solver_user_definitions, undefined=jinja2.DebugUndefined).render( **d )
    d[ "SOLVER_USER_DECLARATIONS" ]           = jinja2.Template(self._solver_user_declarations, undefined=jinja2.DebugUndefined).render( **d )
    d[ "SOLVER_USER_DEFINITIONS" ]            = jinja2.Template(self._solver_user_definitions, undefined=jinja2.DebugUndefined).render( **d )
    d[ "START_TIME_STEP_IMPLEMENTATION" ]     = jinja2.Template(self._start_time_step_implementation, undefined=jinja2.DebugUndefined).render( **d )
    d[ "FINISH_TIME_STEP_IMPLEMENTATION" ]    = jinja2.Template(self._finish_time_step_implementation, undefined=jinja2.DebugUndefined).render( **d )
    d[ "CONSTRUCTOR_IMPLEMENTATION"]          = jinja2.Template(self._constructor_implementation, undefined=jinja2.DebugUndefined).render( **d )
    d[ "COMPUTE_TIME_STEP_SIZE" ]             = jinja2.Template(self._compute_time_step_size, undefined=jinja2.DebugUndefined).render( **d )    
    d[ "COMPUTE_NEW_TIME_STEP_SIZE" ]         = jinja2.Template(self._compute_new_time_step_size, undefined=jinja2.DebugUndefined).render( **d )
    
    d["USE_FLUX"] = self._flux_implementation
    d["USE_NCP"]  = self._ncp_implementation
    d["USE_SOURCE"] = self._source_term_implementation
        
    self._basis.init_dictionary_with_default_parameters(d)
    
  def set_implementation(self,
    flux=None,
    ncp=None,
    eigenvalues=None,
    boundary_conditions=None,
    refinement_criterion=None,
    initial_conditions=None,
    source_term=None,
    point_source=None,
    additional_action_set_includes = "",
    additional_user_includes       = ""
  ):
    """
      If you pass in User_Defined, then the generator will create C++ stubs
      that you have to befill manually. If you pass in None_Implementation, it
      will create nop, i.e. no implementation or defaults. Any other string
      is copied 1:1 into the implementation. If you pass in None, then the
      set value so far won't be overwritten.
    """
    if boundary_conditions  is not None:  self._boundary_conditions_implementation        = boundary_conditions
    if refinement_criterion is not None:  self._refinement_criterion_implementation       = refinement_criterion
    if initial_conditions   is not None:  self._initial_conditions_implementation         = initial_conditions

    if flux                 is not None:  self._flux_implementation                       = flux
    if ncp                  is not None:  self._ncp_implementation                        = ncp
    if eigenvalues          is not None:  self._eigenvalues_implementation                = eigenvalues
    if source_term          is not None:  self._source_term_implementation                = source_term
    if point_source         is not None:  self._point_sources_implementation               = source_term
    
    self.user_action_set_includes += additional_action_set_includes
    self.user_solver_includes     += additional_user_includes

    self.create_data_structures()
    self.create_action_sets()
    
    
    
    
    
    
    
    
      