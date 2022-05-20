# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel
import peano4.output.TemplatedHeaderFile
import peano4.output.TemplatedHeaderImplementationFilePair
import peano4.output.Jinja2TemplatedHeaderFile
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import peano4.toolbox.multigrid.MatrixFreeJacobi

import jinja2

from abc import abstractmethod

import exahype2

import dastgen2
import peano4.datamodel.DaStGen2


class AMRMarker(object):
  """ 
    A very simple Poisson equation system solver to identify AMR transitions
    
    Some codes need to know where AMR transitions are and want to flag the 
    areas around these transitions. This class implements a simple finite 
    differences diffusion implementation which eventually will become 
    stationary if your mesh does not change anymore. The code solves
    
    @f$ - \Delta u = f @f$
    
    for a tiny rhs f with a Jacobi solver. The solution is subject to the 
    constraints 

    @f$ 0 \leq u \leq 1 @f$

    and Dirichlet boundary conditions. Finally, the PDE has prescribed interior
    values of 1 for every hanging vertex. So we have 1 at AMR transitions (and
    the boundary if that's wanted) and we see the solution u fade away once we
    move away from the boundary, until is eventually becomes 0.
    
    
    ## Use cases
    
    ExaHyPE codes usually use this for some damping or smearing close to the
    AMR boundaries where instabilities due to reflecting waves can arise. There
    might be other use cases.
    
    
    ## Usage
    
    To use the marker within your project, add a new solver to your project:
    
         amr_label = exahype2.solvers.elliptic.AMRMarker( name="AMRMarker" )
         project.add_solver( amr_label )


  """
  
      
  def __init__(self, name, relaxation_coefficient = 0.01, right_hand_side = -0.1, flag_domain_boundary = True, plot = False):
    """
    
  name: string
     A unique name for the solver. This one will be used for all generated 
     classes.
  
  plot: Boolean
     Clarifies whether a dump of the data should be enriched with grid info
     (such as enclave status flags), too.
 
    """
    self._name                   = name
    self._relaxation_coefficient = relaxation_coefficient
    self._flag_domain_boundary   = flag_domain_boundary
    self._right_hand_side        = right_hand_side
    self._plot                   = plot
    
    self.create_data_structures()
    self.create_action_sets()


  def __str__(self):
    return """
Name:                   {}
Type:                   {}
Relaxation coefficient: {}
""".format( 
  self._name,
  self.__class__.__name__,
  self._relaxation_coefficient
)


  __repr__ = __str__

  
  def create_readme_descriptor(self, domain_offset, domain_size):
    return """
### ExaHyPE 2 solver

""" + str(self)   

 
  def create_data_structures(self):
    """
    
       
    """
    self._data_model = peano4.datamodel.DaStGen2( self._unknown_identifier() )
    self._data_model.data.add_attribute( dastgen2.attributes.Double("u") )
    self._data_model.data.add_attribute( dastgen2.attributes.Double("residual") )
    
    

  @abstractmethod
  def create_action_sets(self):
    """
    
     Overwrite in subclasses if you wanna create different
     action sets.
     
     :: Call order and ownership
     
     This operation can be called multiple times. However, only the very
     last call matters. All previous calls are wiped out.
     
     If you have a hierarchy of solvers, every create_data_structure()
     should first(!) call its parent version. This way, you always ensure
     that all data are in place before you continue to alter the more
     specialised versions. So it is (logically) a top-down (general to
     specialised) run through all create_data_structure() variants 
     within the inheritance tree.
     
    """
    self._action_set_smoother = peano4.toolbox.multigrid.MatrixFreeJacobi( self._data_model, self._relaxation_coefficient, "1.0", "{}".format(self._right_hand_side) )
    #self._action_set_initial_conditions                       = exahype2.solvers.fv.actionsets.InitialCondition(self, self._store_cell_data_default_guard(), "true" )



  #def _store_cell_data_default_guard(self):
  #  return "not marker.willBeRefined() " + \
  #         "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction"
  
  
  #def _load_cell_data_default_guard(self):
  #  return "not marker.hasBeenRefined() " + \
  #         "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction " + \
  #         "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridInitialisation"


  #def _store_face_data_default_guard(self):
  #  return "not marker.willBeRefined() " + \
  #         "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction"
  
  
  #def _load_face_data_default_guard(self):
  #  return "not marker.hasBeenRefined() " + \
  #         "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction " + \
  #         "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridInitialisation"
  
  
  def _unknown_identifier(self):
    return self._name+"AMRValue"
  

  def get_name_of_global_instance(self):
    return "InstanceOf" + self._name

  
  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    """
    
      Add all required data to the Peano4 project's datamodel 
      so it is properly built up
      
    """
    if verbose:
      print( "AMR marker" )
      print( "----------" )
      print( str(self._name) )
    datamodel.add_vertex(self._data_model)

 
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    """
      Tell Peano what data to move around
      
      Inform Peano4 step which data are to be moved around via the 
      use_cell and use_face commands. This operation is generic from
      ExaHyPE's point of view, i.e. I use it for all grid sweep types. 
    
    """
    step.use_vertex(self._data_model)

  
  def _get_default_includes(self):
    return """
#include "tarch/la/Vector.h" 

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "repositories/SolverRepository.h"
"""


  def add_actions_to_init_grid(self, step):
    """
    
    
    """
    #step.add_action_set( self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement )
    pass

    
  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    """
    
     The boundary information is set only once. It is therefore important that
     we ues the face label and initialise it properly.
     
    """
    #step.add_action_set( self._action_set_initial_conditions_for_grid_construction )
    #step.add_action_set( self._action_set_update_face_label )
    #step.add_action_set( self._action_set_update_cell_label )
    #if evaluate_refinement_criterion:
    #  step.add_action_set( self._action_set_AMR )
    #else:
    #  step.add_action_set( self._action_set_AMR_commit_without_further_analysis )
    pass

  
  def add_actions_to_plot_solution(self, step, output_path):
    """
    
     We plot if and only if we are asked to do so
     
    """
    if self._plot:
      d = {}
      self._init_dictionary_with_default_parameters(d)
      self.add_entries_to_text_replacement_dictionary(d)

      step.add_action_set( peano4.toolbox.PlotScalarNodalFieldInPeanoBlockFormat( 
        filename=output_path + "amr-marker-" + self._name, 
        vertex_unknown=self._data_model,
        getter="getU()", 
        description="marker-value",
        time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())"
      ))
   
 
  def add_actions_to_perform_time_step(self, step):
    """
    
    AMR
    
    It is important that we do the inter-grid transfer operators before we 
    apply the boundary conditions.
    
    """
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)

    #step.add_action_set( self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement )


  @abstractmethod
  def add_entries_to_text_replacement_dictionary(self,d):
    pass


  def add_implementation_files_to_project(self,namespace,output):
    """
    
     The ExaHyPE2 project will call this operation when it sets
     up the overall environment.
     
     This routine is typically not invoked by a user.

     output: peano4.output.Output
     
    """
    templatefile_prefix = os.path.dirname( os.path.realpath(__file__) ) + "/AMRMarker"

    headerDictionary = {}
    implementationDictionary = {}
    self._init_dictionary_with_default_parameters(headerDictionary)
    self._init_dictionary_with_default_parameters(implementationDictionary)
    self.add_entries_to_text_replacement_dictionary(headerDictionary)
    self.add_entries_to_text_replacement_dictionary(implementationDictionary)

    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".template.h",
      templatefile_prefix + ".template.cpp",
      self._name, 
      namespace,
      ".", 
      implementationDictionary,
      True,
      True)

    output.add( generated_solver_files )
    output.makefile.add_cpp_file( self._name + ".cpp" )


  def _init_dictionary_with_default_parameters(self,d):
    """

      This one is called by all algorithmic steps before I invoke
      add_entries_to_text_replacement_dictionary().

      See the remarks on set_postprocess_updated_patch_kernel to understand why
      we have to apply the (partially befilled) dictionary to create a new entry
      for this very dictionary.


    """
    d["RIGHT_HAND_SIDE"]        = self._right_hand_side
    d["RELAXATION_COEFFICIENT"] = self._relaxation_coefficient
    if self._flag_domain_boundary:
      d["BOUNDARY_VALUE"]       = 1.0
    else:
      d["BOUNDARY_VALUE"]       = 0.0

    
    d["SOLVER_INSTANCE"]                = self.get_name_of_global_instance()
    d["SOLVER_NAME"]                    = self._name


  @property
  def name(self):
    return self._name

