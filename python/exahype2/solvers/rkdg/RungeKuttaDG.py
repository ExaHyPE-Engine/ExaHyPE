# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel
import peano4.output.TemplatedHeaderFile
import peano4.output.TemplatedHeaderImplementationFilePair
import peano4.output.Jinja2TemplatedHeaderFile
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

from peano4.solversteps.ActionSet import ActionSet

import jinja2
import math

from abc import abstractmethod

from enum import IntEnum

from exahype2.solvers.rkdg.LagrangeBasis import GaussLegendreBasis, GaussLobattoBasis


class Polynomials(IntEnum):
  """
  
   Superclass has to be IntEnum, as I use this one within Jinja2 templates
   where I struggled to compare against enum variants. I however can always
   compare against integers.
  
  """
  Gauss_Legendre = 0,
  Gauss_Lobatto = 1
  


class RungeKuttaDG(object):
  """ 
    An abstract class for any RKDG solver of any order 
    
    This is the base class of all Runge-Kutta DG solvers. It defines what kind of action 
    sets do exist, i.e. what in principle can be done while we run through the grid. It 
    also provides some kind of very basic infrastructure, i.e. what the name of a solver
    is, what data is to be held per face or cell, or where in the multiscale mesh
    we actually have to hold data.

    The RungeKuttaDG class cannot/should not be instantiated. Its children actually 
    decide which steps are required or invoked in which order.

    They do so by setting the guards: Each data structure has a guard which controls
    if it is to be stored. Each action set has a guard that says if it should be 
    called in this particular mesh traversal.
    If you want to redefine/reset these guards,
    you have to redefine create_data_structures() and reset the guards after
    you have called the superclass' operation. I recommend that you refrain from
    defining completely new guards. Use the predefined guards instead and
    refine them by adding more and and or clauses.

    :: Data structures

    Each cell hosts a DG polynomial of order _dg_order. Technically, we store this polynomial
    in a regular array (blockstructured data) with (_dg_order+1)^d entries. 





  """
  
      
  def __init__(self, name, dg_order, rk_order, polynomials, unknowns, auxiliary_variables, min_cell_h, max_cell_h, plot_grid_properties):
    """
    
  Constructor of the Runge-Kutta Discontinuous Galerkin solver
   
  :: Arguments
    
  name: string
     A unique name for the solver. This one will be used for all generated 
     classes. Also the C++ object instance later on will incorporate this 
     name.
     
  dg_order: int
     Order of the Discontinuous Galerkin approximation
     
  rk_order: int 
     Runge-Kutta order, i.e. time stepping order
     
  polynomials: Polynomials
     Picks which kind of polynomial to use 
     
  unknowns: int
     Number of unknowns per Finite Volume voxel.
     
  auxiliary_variables: int 
     Number of auxiliary variables per Finite Volume voxel. Eventually, both
     unknowns and auxiliary_variables are merged into one big vector if we 
     work with AoS. But the solver has to be able to distinguish them, as 
     only the unknowns are subject to a hyperbolic formulation.
     
  min_cell_h: double
     This size refers to the individual discretisation cell.
    
  max_cell_h: double
     This size refers to the individual discretisation cell.
  
  plot_grid_properties: Boolean
     Clarifies whether a dump of the data should be enriched with grid info
     (such as enclave status flags), too.
 
 
  :: Attributes
 
  All the constructor parameters are stored in object attributes. There's a few
  more which are of interest for subclasses:
  
  _solver_template_file_class_name: String
     This can be used by subclasses to pick which template is to be used to create
     the abstract solver and the actual solver blueprint.
  
    """
    self._name              = name
    
    self._dg_order          = dg_order
    self._rk_order          = rk_order
    self._polynomials       = polynomials
    
    self._unknowns             = unknowns
    self._auxiliary_variables  = auxiliary_variables

    self._min_cell_h         = min_cell_h
    self._max_cell_h         = max_cell_h 
    self._plot_grid_properties = plot_grid_properties

    self.solver_constants_         = ""
    self.user_action_set_includes  = ""
    self.user_solver_includes      = ""
    
    if min_cell_h>max_cell_h:
       raise Exception( "Error: min_cell_h (" + str(min_cell_h) + ") is bigger than max_cell_h (" + str(max_cell_h) + ")" )

    self._solver_template_file_class_name = None

    self.plot_description = ""
        
    self.create_data_structures()
    self.create_action_sets()


  def __str__(self):
    result = """
Name:                   """ + self._name + """
Type:                   """ + self.__class__.__name__ + """
RK/DG order:            """ + str(self._rk_order) + "/" + str(self._dg_order) + """
Unknowns:               """ + str( self._unknowns ) + """
Auxiliary variables:    """ + str( self._auxiliary_variables ) + """
h_cell_min:             """ + str( self._min_cell_h ) + """
h_cell_max:             """ + str( self._max_cell_h ) + """
"""
    return result


  __repr__ = __str__


  def get_min_number_of_spacetree_levels(self,domain_size):
    coarsest_tree_level = 0
    while (domain_size*3**(-coarsest_tree_level)>self._max_cell_h):
      coarsest_tree_level+=1 
    return coarsest_tree_level


  def get_max_number_of_spacetree_levels(self,domain_size):
    finest_tree_level = 0
    while (domain_size*3**(-finest_tree_level)>self._min_cell_h):
      finest_tree_level+=1 
    return finest_tree_level


  def get_coarsest_number_of_cells(self,domain_size):
    return 3**self.get_min_number_of_spacetree_levels(domain_size)


  def get_finest_number_of_cells(self,domain_size):
    return 3**self.get_max_number_of_spacetree_levels(domain_size)

    
  def create_readme_descriptor(self, domain_offset, domain_size):
    return """
### ExaHyPE 2 solver

""" + str(self) + """

Real type of this solver: """ + str(type(self)) + """

We assume of domain size of (0,""" + str(domain_size) + """)^d. Peano 4 will cut this domain equidistantly 
and recursively into three parts along each coordinate axis. This yields a spacetree.

The spacetree will at least have """ + str(self.get_min_number_of_spacetree_levels(domain_size)) + """ levels.
The spacetree will at most have """ + str(self.get_max_number_of_spacetree_levels(domain_size)) + """ levels.

The spacetree will thus span at least """ + str(self.get_coarsest_number_of_cells(domain_size)) + """ cells per coordinate axis. This means a """ + str(self.get_coarsest_number_of_patches(domain_size)) + """^d grid of patches.
The spacetree will thus span at most """ + str(self.get_finest_number_of_cells(domain_size)) + """ cells per coordinate axis. This means a """ + str(self.get_finest_number_of_patches(domain_size)) + """^d grid of patches.

ExaHyPE 2 embeds polynomials of order """ + str(self._dg_order) + """ into the finest tree level.

We use RK(""" + str(self._rk_order) + """) for the time stepping.

""" 
  

  def get_user_action_set_includes(self):
    return self.user_action_set_includes

  
  def get_user_solver_includes(self):
    return self.user_solver_includes

  
  @abstractmethod
  def create_data_structures(self):
    """
    
     Recall in subclasses if you wanna change the number of unknowns
     or auxiliary variables. See class description's subsection on 
     data flow.
     
     :: Call order and ownership
     
     This operation can be called multiple times. However, only the very
     last call matters. All previous calls are wiped out.
     
     If you have a hierarchy of solvers, every create_data_structure()
     should first(!) call its parent version. This way, you always ensure
     that all data are in place before you continue to alter the more
     specialised versions. So it is (logically) a top-down (general to
     specialised) run through all create_data_structure() variants 
     within the inheritance tree.
     
     :: Arguments
     
     _patch: Patch (NxNxN)
       Actual patch data. We use Finite Volumes, so this is 
       always the current snapshot, i.e. the valid data at one point.
    
     _patch_overlap_old, _patch_overlap_new: Patch (2xNxN)
       This is a copy/excerpt from the two adjacent finite volume
       snapshots plus the old data as backup. If I want to implement
       local timestepping, I don't have to backup the whole patch 
       (see _patch), but I need a backup of the face data to be able
       to interpolate in time.
       
     _patch_overlap_update: Patch (2xNxN)
       This is hte new update. After the time step, I roll this 
       information over into _patch_overlap_new, while I backup the 
       previous _patch_overlap_new into _patch_overlap_old. If I 
       worked with regular meshes only, I would not need this update
       field and could work directly with _patch_overlap_new. However,
       AMR requires me to accumulate data within new while I need 
       the new and old data temporarily. Therefore, I employ this 
       accumulation/roll-over data which usually is not stored
       persistently.
       
    """
    self._patch                = peano4.datamodel.Patch( (self._patch_size,self._patch_size,self._patch_size), self._unknowns+self._auxiliary_variables, self._unknown_identifier() )
    self._patch_overlap_old    = peano4.datamodel.Patch( (2,self._patch_size,self._patch_size),                self._unknowns+self._auxiliary_variables, self._unknown_identifier() + "Old" )
    self._patch_overlap_new    = peano4.datamodel.Patch( (2,self._patch_size,self._patch_size),                self._unknowns+self._auxiliary_variables, self._unknown_identifier() + "New" )
    self._patch_overlap_update = peano4.datamodel.Patch( (2,self._patch_size,self._patch_size),                self._unknowns+self._auxiliary_variables, self._unknown_identifier() + "Update" )
    
    self._patch_overlap_old.generator.merge_method_definition = peano4.toolbox.blockstructured.get_face_overlap_merge_implementation(self._patch_overlap_old)
    self._patch_overlap_new.generator.merge_method_definition = peano4.toolbox.blockstructured.get_face_overlap_merge_implementation(self._patch_overlap_new)
    
    self._patch_overlap_old.generator.includes     += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 
"""
    self._patch_overlap_new.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 
"""
   
    self._patch.generator.store_persistent_condition = self._store_cell_data_default_guard()
    self._patch.generator.load_persistent_condition  = self._load_cell_data_default_guard()
    
    self._patch_overlap_old.generator.send_condition               = "false"
    self._patch_overlap_old.generator.receive_and_merge_condition  = "false"

    self._patch_overlap_new.generator.send_condition               = "false"
    self._patch_overlap_new.generator.receive_and_merge_condition  = "false"

    self._patch_overlap_update.generator.send_condition               = "false"
    self._patch_overlap_update.generator.receive_and_merge_condition  = "false"

    self._patch_overlap_old.generator.store_persistent_condition   = self._store_face_data_default_guard()
    self._patch_overlap_old.generator.load_persistent_condition    = self._load_face_data_default_guard()

    self._patch_overlap_new.generator.store_persistent_condition   = self._store_face_data_default_guard()
    self._patch_overlap_new.generator.load_persistent_condition    = self._load_face_data_default_guard()

    self._patch_overlap_update.generator.store_persistent_condition   = "false"
    self._patch_overlap_update.generator.load_persistent_condition    = "false"

    self._patch.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    
    self._patch_overlap_old.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    
    self._patch_overlap_new.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    
    self._patch_overlap_update.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    

    self._cell_label = exahype2.grid.create_cell_label( self._name )
    self._face_label = exahype2.grid.create_face_label( self._name )  
    

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
    self._action_set_initial_conditions                       = exahype2.solvers.fv.actionsets.InitialCondition(self, self._store_cell_data_default_guard(), "true" )
    self._action_set_initial_conditions_for_grid_construction = exahype2.solvers.fv.actionsets.InitialCondition(self, self._store_cell_data_default_guard(), "false")
    self._action_set_AMR                                      = exahype2.solvers.fv.actionsets.AdaptivityCriterion(solver=self, guard=self._store_cell_data_default_guard(), build_up_new_refinement_instructions=True,  implement_previous_refinement_instructions=True )
    self._action_set_AMR_commit_without_further_analysis      = exahype2.solvers.fv.actionsets.AdaptivityCriterion(solver=self, guard=self._store_cell_data_default_guard(), build_up_new_refinement_instructions=False, implement_previous_refinement_instructions=True )
    self._action_set_handle_boundary                          = exahype2.solvers.fv.actionsets.HandleBoundary(self, self._store_face_data_default_guard())
    self._action_set_project_patch_onto_faces                 = exahype2.solvers.fv.actionsets.ProjectPatchOntoFaces(self, self._store_cell_data_default_guard())
    self._action_set_roll_over_update_of_faces                = exahype2.solvers.fv.actionsets.RollOverUpdatedFace(self, self._store_face_data_default_guard())
    self._action_set_copy_new_faces_onto_old_faces            = peano4.toolbox.blockstructured.BackupPatchOverlap(self._patch_overlap_new, self._patch_overlap_old, False, self._store_face_data_default_guard(), self._get_default_includes())
    self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = exahype2.solvers.fv.actionsets.DynamicAMR( solver=self )

    self._action_set_update_face_label = exahype2.grid.UpdateFaceLabel( self._name )  
    self._action_set_update_cell_label = exahype2.grid.UpdateCellLabel( self._name ) 
        
    self._action_set_update_cell                                                      = None

    self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement.switch_interpolation_scheme( "linear_precomputed_operators<" + str(self._patch_size) +">" )


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

  
  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    """
    
      Add all required data to the Peano4 project's datamodel 
      so it is properly built up
      
    """
    if verbose:
      print( "Patch data" )
      print( "----------" )
      print( str(self._patch) )
      print( "Patch overlap data" )
      print( "----------" )
      print( str(self._patch_overlap_old) )
      print( "Patch overlap data" )
      print( "----------" )
      print( str(self._patch_overlap_new) )
    datamodel.add_cell(self._cell_label)
    datamodel.add_cell(self._patch)
    datamodel.add_face(self._patch_overlap_old)
    datamodel.add_face(self._patch_overlap_new)
    datamodel.add_face(self._patch_overlap_update)
    datamodel.add_face(self._face_label)

 
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    """
      Tell Peano what data to move around
      
      Inform Peano4 step which data are to be moved around via the 
      use_cell and use_face commands. This operation is generic from
      ExaHyPE's point of view, i.e. I use it for all grid sweep types. 
    
    """
    step.use_cell(self._patch)
    step.use_cell(self._cell_label)
    step.use_face(self._patch_overlap_old)
    step.use_face(self._patch_overlap_new)
    step.use_face(self._patch_overlap_update)
    step.use_face(self._face_label)

  
  def _get_default_includes(self):
    return """
#include "tarch/la/Vector.h" 

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "repositories/SolverRepository.h"
"""


  def add_actions_to_init_grid(self, step):
    """
    
     The AMR stuff has to be the very first thing. Actually, the AMR routines' 
     interpolation doesn't play any role here. But the restriction indeed is
     very important, as we have to get the face data for BCs et al. The action 
     set order is inverted while we ascend within the tree again. Therefore, we
     add the AMR action set first which means it will be called last when we go
     from fine to coarse levels within the tree.
     
     :: Ordering ::
     
     The order of the action sets is preserved throughout the steps down within
     the tree hierarchy. It is inverted throughout the backrolling. 
     
     This is what we want to achieve:
     
     - Roll updates over on the faces onto Q_old and Q_new.
     - Copy new face data into old face data, as this is the initial sweep, i.e.
       the old face data otherwise might hold rubbish.
     - Restrict the data to the coarser level if we are on a hanging face. 
     
    
    """
    step.add_action_set( self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement )
    step.add_action_set( self._action_set_copy_new_faces_onto_old_faces )
    step.add_action_set( self._action_set_roll_over_update_of_faces )
    step.add_action_set( self._action_set_initial_conditions ) 
    step.add_action_set( self._action_set_project_patch_onto_faces )
    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )
    step.add_action_set( self._action_set_AMR )
    
    
  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    """
    
     The boundary information is set only once. It is therefore important that
     we ues the face label and initialise it properly.
     
    """
    step.add_action_set( self._action_set_initial_conditions_for_grid_construction )
    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )
    if evaluate_refinement_criterion:
      step.add_action_set( self._action_set_AMR )
    else:
      step.add_action_set( self._action_set_AMR_commit_without_further_analysis )
          
  
  def set_plot_description(self,description):
    """
    
     Use this one to set a description within the output patch file that tells
     the vis solver what the semantics of the entries are. Typicallly, I use 
     a comma-separated list here. 
    
    """
    self.plot_description = description
    
  
  def add_actions_to_plot_solution(self, step, output_path):
    """
    
     Consult the discussion in add_actions_to_init_grid() around the order
     of the individual action sets.
     
     It is important that we have the coupling/dynamic AMR part in here, as 
     there might be pending AMR refinement requests that now are realised.
     For the same reason, we need the update of the face label and the update
     of the cell label in here: The AMR might just propagate over into the
     plotting, i.e. we might create new grid entities throughout the plot.
     These entities (faces and cells) have to be initialised properly. 
     Otherwise, their un-initialised data will propagate through to the next
     time step.
     
     To make the restriction work, we have to project the solutions onto the 
     faces.
     
    """
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)

    step.add_action_set( self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement )
    step.add_action_set( self._action_set_roll_over_update_of_faces )
    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )
    step.add_action_set( self._action_set_project_patch_onto_faces )
    #step.add_action_set( self._action_set_AMR_commit_without_further_analysis )

    step.add_action_set( peano4.toolbox.blockstructured.PlotPatchesInPeanoBlockFormat( 
      filename=output_path + "solution-" + self._name, 
      patch=self._patch, 
      dataset_name=self._unknown_identifier(), 
      description=self.plot_description,
      guard="repositories::plotFilter.plotPatch(marker) and " + self._load_cell_data_default_guard(),
      additional_includes="""
#include "exahype2/PlotFilter.h"
#include "../repositories/SolverRepository.h"
""",
      precision="PlotterPrecision",
      time_stamp_evaluation="repositories::getMinTimeStamp()"
    ))

    if self._plot_grid_properties:    
      step.add_action_set( peano4.toolbox.PlotGridInPeanoBlockFormat( 
        filename = output_path + "grid-" + self._name,
        cell_unknown=None,
        guard_guard="repositories::plotFilter.plotPatch(marker) and " + self._load_cell_data_default_guard(),
        additional_includes="""
#include "exahype2/PlotFilter.h"
#include "../repositories/SolverRepository.h"
"""
      ))
      
    pass
   
 
  def add_actions_to_perform_time_step(self, step):
    """
    
    AMR
    
    It is important that we do the inter-grid transfer operators before we 
    apply the boundary conditions.
    
    """
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)

    step.add_action_set( self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement )
    step.add_action_set( self._action_set_roll_over_update_of_faces )
    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )
    step.add_action_set( self._action_set_handle_boundary )
    step.add_action_set( self._action_set_update_cell )
    step.add_action_set( self._action_set_project_patch_onto_faces )
    step.add_action_set( self._action_set_AMR )


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


  def _init_dictionary_with_default_parameters(self,d):
    """

      This one is called by all algorithmic steps before I invoke
      add_entries_to_text_replacement_dictionary().

      See the remarks on set_postprocess_updated_patch_kernel to understand why
      we have to apply the (partially befilled) dictionary to create a new entry
      for this very dictionary.


    """
    d["NUMBER_OF_VOLUMES_PER_AXIS"]     = self._patch.dim[0]
    d["HALO_SIZE"]                      = int(self._patch_overlap_old.dim[0]/2)
    d["SOLVER_INSTANCE"]                = self.get_name_of_global_instance()
    d["SOLVER_NAME"]                    = self._name
    d["UNKNOWN_IDENTIFIER"]             = self._unknown_identifier()
    d["NUMBER_OF_UNKNOWNS"]             = self._unknowns
    d["NUMBER_OF_AUXILIARY_VARIABLES"]  = self._auxiliary_variables
    d["SOLVER_NUMBER"]                  = 22

    if self._patch_overlap_old.dim[0]/2!=1:
      print( "ERROR: Finite Volume solver currently supports only a halo size of 1")

    d[ "ASSERTION_WITH_1_ARGUMENTS" ] = "nonCriticalAssertion1"
    d[ "ASSERTION_WITH_2_ARGUMENTS" ] = "nonCriticalAssertion2"
    d[ "ASSERTION_WITH_3_ARGUMENTS" ] = "nonCriticalAssertion3"
    d[ "ASSERTION_WITH_4_ARGUMENTS" ] = "nonCriticalAssertion4"
    d[ "ASSERTION_WITH_5_ARGUMENTS" ] = "nonCriticalAssertion5"
    d[ "ASSERTION_WITH_6_ARGUMENTS" ] = "nonCriticalAssertion6"

    if (self._min_volume_h > self._max_volume_h ):
      raise Exception( "min/max h are inconsistent" )
    d[ "MAX_VOLUME_H"] = self._max_volume_h
    d[ "MIN_VOLUME_H"] = self._min_volume_h

    d[ "SOLVER_CONSTANTS" ] = self.solver_constants_

    d[ "SOLVER_INCLUDES" ]  = self.get_user_solver_includes()


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
