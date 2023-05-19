# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel
import peano4.output.TemplatedHeaderFile
import peano4.output.TemplatedHeaderImplementationFilePair
import peano4.output.Jinja2TemplatedHeaderFile
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import jinja2

from abc import abstractmethod

import exahype2
import exahype2.solvers.rkfd.actionsets
    
from .PDETerms import PDETerms

from exahype2.solvers.ButcherTableau              import RungeKutta_steps

class CellCenteredFiniteDifferences(object):
  """ 
  
    Abstract solver for patch-based finite diffences

    All solvers in ExaHyPE are cell-centered discretisations.    
    
    
    ## Adaptive mesh refinement
    
    We have, at the moment, no hard-coded AMR operator set available unless
    you work with an overlap of one. In this case, you find operators in the 
    toolbox. Very few pre-manufactured operators there are ready to go for 
    higher overlaps (the injection operator is an example). In general, you 
    will have to inject your own transfer operators.
    
    It depends on the flavour that you want to use for your interpolation and
    restriction. A simple interpolation for an overlap of three and a patch_size
    of five would be
        
            self._interpolation = "tensor_product< " + self._name + ">"
            self.add_solver_constants(   "static constexpr double  NormalInterpolationMatrix1d[]     = {0.0, 1.0, 0.0};" )
            self.add_solver_constants( " ""static constexpr double  TangentialInterpolationMatrix1d[] = {
                    1.0,0.0,0.0,0.0,0.0,
                    1.0,0.0,0.0,0.0,0.0,
                    1.0,0.0,0.0,0.0,0.0,
                    0.0,1.0,0.0,0.0,0.0,               
                    0.0,1.0,0.0,0.0,0.0,
                    
                    0.0,1.0,0.0,0.0,0.0,
                    0.0,0.0,1.0,0.0,0.0,
                    0.0,0.0,1.0,0.0,0.0,
                    0.0,0.0,1.0,0.0,0.0,
                    0.0,0.0,0.0,1.0,0.0,
                    
                    0.0,0.0,0.0,1.0,0.0,
                    0.0,0.0,0.0,1.0,0.0,
                    0.0,0.0,0.0,0.0,1.0,
                    0.0,0.0,0.0,0.0,1.0,
                    0.0,0.0,0.0,0.0,1.0
            };" "" )

    ## Reduce memory movements

    You can switch for individual fields to a heap-based scheme by changing 
    the generator:
    
           self._patch.generator        = peano4.datamodel.PatchToDoubleArrayOnHeap(self._patch,           "double")
           self._patch._patch_estimates = peano4.datamodel.PatchToDoubleArrayOnHeap(self._patch_estimates, "double")

    Alternatively, use the switch_to_heap_storage() routine.

  """
  
      
  def __init__(self, name, patch_size, overlap, rk_order, unknowns, auxiliary_variables, min_meshcell_h, max_meshcell_h, plot_grid_properties, kernel_namespace):
    """
  name: string
     A unique name for the solver. This one will be used for all generated 
     classes. Also the C++ object instance later on will incorporate this 
     name.
     
  patch_size: int
     Size of the patch in one dimension. All stuff here's dimension-generic.
     
  overlap: int
     That's the size of the halo layer which is half of the overlap with a 
     neighbour. A value of 1 means that a patch_size x patch_size patch in 
     2d is surrounded by one additional cell layer. The overlap has to be 
     bigger or equal to one. It has to be smaller or equal to patch_size.
     
  unknowns: int
     Number of unknowns per Finite Volume voxel.
     
  auxiliary_variables: int 
     Number of auxiliary variables per Finite Volume voxel. Eventually, both
     unknowns and auxiliary_variables are merged into one big vector if we 
     work with AoS. But the solver has to be able to distinguish them, as 
     only the unknowns are subject to a hyperbolic formulation.
     
  min_meshcell_h: double
     This size refers to the individual Finite Volume.
    
  max_meshcell_h: double
     This size refers to the individual Finite Volume.
  
  plot_grid_properties: Boolean
     Clarifies whether a dump of the data should be enriched with grid info
     (such as enclave status flags), too.
 
    """
    self._name              = name
    
    self._min_meshcell_h         = min_meshcell_h
    self._max_meshcell_h         = max_meshcell_h 
    self._plot_grid_properties = plot_grid_properties

    self._patch_size           = patch_size  
    self._overlap              = overlap
    self._rk_order             = rk_order
    self._unknowns             = unknowns
    self._auxiliary_variables  = auxiliary_variables

    self._solver_constants         = ""
    self._user_action_set_includes  = ""
    self._user_solver_includes      = ""
    
    self._kernel_namespace = kernel_namespace

    if min_meshcell_h>max_meshcell_h:
       print( "Error: min_meshcell_h (" + str(min_meshcell_h) + ") is bigger than max_meshcell_h (" + str(max_meshcell_h) + ")" )

    self._reconstructed_array_memory_location=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack

    self._solver_template_file_class_name = None

    self._plot_description = ""

    self.select_dofs_to_print = None
    
    self._action_set_initial_conditions                       = None
    self._action_set_initial_conditions_for_grid_construction = None
    self._action_set_AMR                                      = None
    self._action_set_AMR_commit_without_further_analysis      = None
    self._action_set_handle_boundary                          = None
    self._action_set_project_patch_onto_faces                 = None
    self._action_set_roll_over_update_of_faces                = None
    self._action_set_copy_new_faces_onto_old_faces            = None
    self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = None
    self._action_set_postprocess_solution                     = None
    self._action_set_preprocess_solution                      = None

    self._compute_time_step_size                         = "#error Not yet defined"
    self._compute_new_time_step_size                     = "#error Not yet defined"
    self._preprocess_reconstructed_patch                 = ""
    self._postprocess_updated_patch                      = ""

    self._compute_kernel_call       = "#error Not yet defined"
    
    self._compute_eigenvalue        = False
    self._abstract_solver_user_declarations        = ""
    self._abstract_solver_user_definitions         = ""
    self._solver_user_declarations                 = ""
    self._solver_user_definitions                  = ""

    self._start_time_step_implementation           = ""
    self._finish_time_step_implementation          = ""
    self._constructor_implementation               = ""

    self._boundary_conditions_implementation  = None
    self._refinement_criterion_implementation = None
    self._initial_conditions_implementation   = None

    self._interpolation = "#error Not yet defined"
    self._restriction   = "#error Not yet defined"

    self._hold_cell_data_on_heap_with_smart_pointer = False
    self._hold_face_data_on_heap_with_smart_pointer = False

    self.create_data_structures()
    self.create_action_sets()



  def __str__(self):
    result = """
Name:                   """ + self._name + """
Type:                   """ + self.__class__.__name__ + """
Patch size:             """ + str( self._patch_size ) + """  
Runge-Kutta order:      """ + str( self._rk_order ) + """
Overlap:                """ + str( self._overlap ) + """  
Unknowns:               """ + str( self._unknowns ) + """
Auxiliary variables:    """ + str( self._auxiliary_variables ) + """
h_volume_min:           """ + str( self._min_meshcell_h ) + """
h_volume_max:           """ + str( self._max_meshcell_h ) + """
"""
    return result


  __repr__ = __str__


  def get_min_number_of_spacetree_levels(self,domain_size):
    coarsest_tree_level = 0
    while (domain_size*3**(-coarsest_tree_level)/self._patch_size>self._max_meshcell_h):
      coarsest_tree_level+=1 
    return coarsest_tree_level


  def get_max_number_of_spacetree_levels(self,domain_size):
    finest_tree_level = 0
    while (domain_size*3**(-finest_tree_level)/self._patch_size>self._min_meshcell_h):
      finest_tree_level+=1 
    return finest_tree_level


#    self._overlap              = overlap
#    self._unknowns             = unknowns
#    self._auxiliary_variables  = auxiliary_variables


  def get_coarsest_number_of_patches(self,domain_size):
    return 3**self.get_min_number_of_spacetree_levels(domain_size)


  def get_finest_number_of_patches(self,domain_size):
    return 3**self.get_max_number_of_spacetree_levels(domain_size)


  def get_coarsest_number_of_compute_grid_cells(self,domain_size):
    return self.get_coarsest_number_of_patches(domain_size)*self._patch_size


  def get_finest_number_of_compute_grid_cells(self,domain_size):
    return self.get_finest_number_of_patches(domain_size)*self._patch_size

    
  def get_coarsest_compute_grid_cell_size(self,domain_size):
    return domain_size/self.get_coarsest_number_of_compute_grid_cells(domain_size)


  def get_finest_compute_grid_cell_size(self,domain_size):
    return domain_size/self.get_finest_number_of_compute_grid_cells(domain_size)

  
  def create_readme_descriptor(self, domain_offset, domain_size):
    return """
### ExaHyPE 2 solver

""" + str(self) + """

Real type of this solver: """ + str(type(self)) + """

We assume that you use a domain size of (0,""" + str(domain_size) + """)^d. Peano 4 will cut this domain equidistantly 
and recursively into three parts along each coordinate axis. This yields a spacetree.

The spacetree will at least have """ + str(self.get_min_number_of_spacetree_levels(domain_size)) + """ levels.
The spacetree will at most have """ + str(self.get_max_number_of_spacetree_levels(domain_size)) + """ levels.

The spacetree will thus span at least """ + str(self.get_coarsest_number_of_patches(domain_size)) + """ octants/cells (see clarification below) per coordinate axis. This means a """ + str(self.get_coarsest_number_of_patches(domain_size)) + """^d grid of octants.
The spacetree will thus span at most """ + str(self.get_finest_number_of_patches(domain_size)) + """ octants/cells (see clarification below) per coordinate axis. This means a """ + str(self.get_finest_number_of_patches(domain_size)) + """^d grid of octants.

ExaHyPE 2 embeds """ + str(self._patch_size) + """^d patches of compute cells into the finest tree level. 
We refer to elements of this level as octant above.
They are squares/cubes and many papers refer to them as cells, but they are not 
the actual compute data structure. The compute data structure is the cells that
are embedded into these finest level spacetree cells. We therefore prefer the 
term octant for the latter, whereas we use the term (compute) cell for the 
entities that are actually used for the computations, i.e. hold degrees of 
freedom, and are actually visible within Paraview, e.g.

The coarsest possible mesh will consist of """ + str(self.get_coarsest_number_of_compute_grid_cells(domain_size)) + """ compute cells per coordinate axis.
The finest possible mesh will consist of """ + str(self.get_finest_number_of_compute_grid_cells(domain_size)) + """ compute cells per coordinate axis.

The coarsest mesh width of """ + str(self.get_coarsest_compute_grid_cell_size(domain_size)) + """ is thus just smaller than the maximum volume size """ + str(self._max_meshcell_h) + """.
The finest mesh width of """ + str(self.get_finest_compute_grid_cell_size(domain_size)) + """ is thus just smaller than the minimum volume size """ + str(self._min_meshcell_h) + """.

""" 
  

  @property
  def user_action_set_includes(self):
    """
    
    Add further includes to this property, if your action sets require some additional
    routines from other header files.
    
    """
    return self._user_action_set_includes

  
  @property
  def user_solver_includes(self):
    """
    
    Add further includes to this property, if your solver requires some additional
    routines from other header files.
    
    """
    return self._user_solver_includes


  def number_of_Runge_Kutta_steps(self):
    return RungeKutta_steps(self._rk_order)

  
  def add_user_action_set_includes(self,value):
    """
    
    Add further includes to this property, if your action sets require some additional
    routines from other header files.
    
    """
    self._user_action_set_includes += value

  
  def add_user_solver_includes(self,value):
    """
    
    Add further includes to this property, if your solver requires some additional
    routines from other header files.
    
    """
    self._user_solver_includes += value

  
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
    self._patch_estimates      = peano4.datamodel.Patch( (self.number_of_Runge_Kutta_steps()*self._patch_size,self._patch_size,self._patch_size), self._unknowns, self._unknown_identifier() + "RhsEstimates" )
    self._patch_overlap_old    = peano4.datamodel.Patch( (2*self._overlap,self._patch_size,self._patch_size),  self._unknowns+self._auxiliary_variables, self._unknown_identifier() + "Old" )
    self._patch_overlap_new    = peano4.datamodel.Patch( (2*self._overlap,self._patch_size,self._patch_size),  self._unknowns+self._auxiliary_variables, self._unknown_identifier() + "New" )
    self._patch_overlap_update = peano4.datamodel.Patch( (2*self._overlap,self._patch_size,self._patch_size),  self._unknowns+self._auxiliary_variables, self._unknown_identifier() + "Update" )

    if self._hold_cell_data_on_heap_with_smart_pointer:
      self._patch.generator           = peano4.datamodel.PatchToDoubleArrayOnHeap(self._patch,           "double")
      self._patch_estimates.generator = peano4.datamodel.PatchToDoubleArrayOnHeap(self._patch_estimates, "double")
    if self._hold_face_data_on_heap_with_smart_pointer:
      self._patch_overlap_old.generator    = peano4.datamodel.PatchToDoubleArrayOnHeap(self._patch_overlap_old,    "double") 
      self._patch_overlap_new.generator    = peano4.datamodel.PatchToDoubleArrayOnHeap(self._patch_overlap_new,    "double") 
      self._patch_overlap_update.generator = peano4.datamodel.PatchToDoubleArrayOnHeap(self._patch_overlap_update, "double") 
    
    self._patch_overlap_old.generator.merge_method_definition = peano4.toolbox.blockstructured.get_face_merge_implementation(self._patch_overlap_old)
    self._patch_overlap_new.generator.merge_method_definition = peano4.toolbox.blockstructured.get_face_merge_implementation(self._patch_overlap_new)
    self._patch.generator.merge_method_definition             = peano4.toolbox.blockstructured.get_cell_merge_implementation(self._patch)

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
    self._patch.generator.compute_condition          = self._load_cell_data_default_guard() + " or " + self._store_cell_data_default_guard()


    self._patch_estimates.generator.store_persistent_condition = self._store_cell_data_default_guard()
    self._patch_estimates.generator.load_persistent_condition  = self._load_cell_data_default_guard()
    self._patch_estimates.generator.compute_condition          = self._load_cell_data_default_guard() + " or " + self._store_cell_data_default_guard()

    self._patch_overlap_old.generator.send_condition               = "false"
    self._patch_overlap_old.generator.receive_and_merge_condition  = "false"

    self._patch_overlap_new.generator.send_condition               = "false"
    self._patch_overlap_new.generator.receive_and_merge_condition  = "false"

    self._patch_overlap_update.generator.send_condition               = "false"
    self._patch_overlap_update.generator.receive_and_merge_condition  = "false"

    self._patch_overlap_old.generator.store_persistent_condition   = self._store_face_data_default_guard()
    self._patch_overlap_old.generator.load_persistent_condition    = self._load_face_data_default_guard()
    self._patch_overlap_old.generator.compute_condition            = self._load_face_data_default_guard() + " or " + self._store_face_data_default_guard()

    self._patch_overlap_new.generator.store_persistent_condition   = self._store_face_data_default_guard()
    self._patch_overlap_new.generator.load_persistent_condition    = self._load_face_data_default_guard()
    self._patch_overlap_new.generator.compute_condition            = self._load_face_data_default_guard() + " or " + self._store_face_data_default_guard()

    self._patch_overlap_update.generator.store_persistent_condition   = "false"
    self._patch_overlap_update.generator.load_persistent_condition    = "false"
    self._patch_overlap_update.generator.compute_condition            = "false"

    self._patch.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    
    self._patch_estimates.generator.includes  += """
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
     
     :: Recreation vs backup (state discussion)
     
     We faced some issues with action sets that should not be 
     overwritten. For example, the postprocessing should not be overwritten
     as users might want to set it and then later on reset the number of 
     unknowns, e.g. In this case, you would loose your postprocessing if 
     create_action_sets() recreated them. So I decided to make an exception
     here: the postprocessing step is never overwritten by the standard
     classes.
     
     There are further action sets which have a state, which users might 
     want to alter. The most prominent one is the AMR routines, where users
     often alter the interpolation and restriction scheme. Here, things are
     tricky: If we keep the default one, the action set would not pick up 
     if you changed the number of unknowns, e.g. However, if we recreated 
     the action set, we'd miss out on any changed interpolation/restriction
     scheme. Therefore, I have to hold the interpolation and restriction
     scheme seperatae.
     
     :: Injecting your own guards
     
     If you inject your own guards, you should combine them with a storage
     predicate, i.e. _store_cell_data_default_guard() and 
     _load_cell_data_default_guard(). The action sets themselves will not
     combine the guard with further boolean expressions. Also, you have to 
     study carefully if a predicate accepts a unique guard or a set of 
     guards.
     
    """
    self._action_set_initial_conditions                          = exahype2.solvers.rkfd.actionsets.InitialCondition(self, self._store_cell_data_default_guard(), "true" )
    self._action_set_initial_conditions_for_grid_construction    = exahype2.solvers.rkfd.actionsets.InitialCondition(self, self._store_cell_data_default_guard(), "false")
    self._action_set_AMR                                         = exahype2.solvers.rkfd.actionsets.AdaptivityCriterion(solver=self, guard=self._store_cell_data_default_guard(), build_up_new_refinement_instructions=True,  implement_previous_refinement_instructions=True )
    self._action_set_AMR_commit_without_further_analysis         = exahype2.solvers.rkfd.actionsets.AdaptivityCriterion(solver=self, guard=self._store_cell_data_default_guard(), build_up_new_refinement_instructions=False, implement_previous_refinement_instructions=True )
    self._action_set_handle_boundary                             = exahype2.solvers.rkfd.actionsets.HandleBoundary(self, self._store_face_data_default_guard())
    self._action_set_project_patch_onto_faces                    = exahype2.solvers.rkfd.actionsets.ProjectPatchOntoFaces(self)
    self._action_set_roll_over_update_of_faces                   = exahype2.solvers.rkfd.actionsets.RollOverUpdatedFace(self, self._store_face_data_default_guard())
    self._action_set_copy_new_faces_onto_old_faces               = peano4.toolbox.blockstructured.BackupPatchOverlap(self._patch_overlap_new, self._patch_overlap_old, False, self._store_face_data_default_guard(), self._get_default_includes())
    self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = exahype2.solvers.rkfd.actionsets.DynamicAMR( solver=self, interpolation=self._interpolation, restriction=self._restriction )

    self._action_set_compute_final_linear_combination = exahype2.solvers.rkfd.actionsets.ComputeFinalLinearCombination(solver=self,guard=self._store_cell_data_default_guard())

    # This one is different: it is the hook-in point for other solvers, so we create it 
    # only once if it does not exist
    if self._action_set_postprocess_solution == None:
      self._action_set_postprocess_solution                   = exahype2.solvers.rkfd.actionsets.EmptyPostprocessSolution( self )
    if self._action_set_preprocess_solution == None:
      self._action_set_preprocess_solution                    = exahype2.solvers.rkfd.actionsets.EmptyPreprocessSolution( self )
    
    self._action_set_update_face_label = exahype2.grid.UpdateFaceLabel( self._name )  
    self._action_set_update_cell_label = exahype2.grid.UpdateCellLabel( self._name ) 
        
    self._action_set_update_cell = None



  def _store_cell_data_default_guard(self):
    """!
    
    Extend the guard via ands only. Never use an or, as subclasses might 
    extend it as well, and they will append further ends.
    
    """
    return "not marker.willBeRefined() " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction"
  
  
  def _load_cell_data_default_guard(self):
    """!
    
    Extend the guard via ands only. Never use an or, as subclasses might 
    extend it as well, and they will append further ends.
    
    """
    return "not marker.hasBeenRefined() " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridInitialisation"


  def _store_face_data_default_guard(self):
    """!
    
    Extend the guard via ands only. Never use an or, as subclasses might 
    extend it as well, and they will append further ends.
    
    """
    return "not marker.willBeRefined() " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction"
  
  
  def _load_face_data_default_guard(self):
    """!
    
    Extend the guard via ands only. Never use an or, as subclasses might 
    extend it as well, and they will append further ends.
    
    """
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
    datamodel.add_cell(self._patch_estimates)
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
    step.use_cell(self._patch_estimates)
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
     
     The projection onto the faces is a postprocessing step. This is different 
     to DG, where we need face data for the current time step's solution. Here,
     we ensure that all halos are valid for the subsequent time step again.
     
     :: Ordering ::
     
     The order of the action sets is preserved throughout the steps down within
     the tree hierarchy. It is inverted throughout the backrolling. 
     
     This is what we want to achieve:
     
     - Roll updates over on the faces onto Q_old and Q_new.
     - Copy new face data into old face data, as this is the initial sweep, i.e.
       the old face data otherwise might hold rubbish.
     - Restrict the data to the coarser level if we are on a hanging face. 
     
    
    """
    step.add_action_set( self._action_set_preprocess_solution )
    step.add_action_set( self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement )
    step.add_action_set( self._action_set_copy_new_faces_onto_old_faces )
    step.add_action_set( self._action_set_roll_over_update_of_faces )
    step.add_action_set( self._action_set_initial_conditions ) 
    step.add_action_set( self._action_set_project_patch_onto_faces )
    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )
    step.add_action_set( self._action_set_AMR )
    step.add_action_set( self._action_set_postprocess_solution )
    
    
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


  @property
  def plot_description(self,):
    return self._plot_description
          
  
  @plot_description.setter
  def plot_description(self,description):
    """
    
     Use this one to set a description within the output patch file that tells
     the vis solver what the semantics of the entries are. Typicallly, I use 
     a comma-separated list here. 
    
    """
    self._plot_description = description
    
  
  def add_actions_to_plot_solution(self, step, output_path):
    """
    
     Consult the discussion in add_actions_to_init_grid() around the order
     of the individual action sets.
     
     
     ## Adaptive mesh refinement
     
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
     
     
     ## Roll over face data
     
     Das ist ein Kaese, weil das nur einspringt, wenn project wahr ist
     
     
    """
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)

    step.add_action_set( self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement )
    #    step.add_action_set( self._action_set_roll_over_update_of_faces )
    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )
    #    step.add_action_set( self._action_set_project_patch_onto_faces )

    step.add_action_set( peano4.toolbox.blockstructured.PlotPatchesInPeanoBlockFormat( 
      filename=output_path + "solution-" + self._name, 
      patch=self._patch, 
      dataset_name=self._unknown_identifier(), 
      description=self._plot_description,
      guard="repositories::plotFilter.plotPatch(marker) and " + self._load_cell_data_default_guard(),
      additional_includes="""
#include "exahype2/PlotFilter.h"
#include "../repositories/SolverRepository.h"
""",
      precision="PlotterPrecision",
      time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
      select_dofs = self.select_dofs_to_print
    ))

    if self._plot_grid_properties:    
      step.add_action_set( peano4.toolbox.PlotGridInPeanoBlockFormat( 
        filename = output_path + "grid-" + self._name,
        cell_unknown=None,
        guard="repositories::plotFilter.plotPatch(marker) and " + self._load_cell_data_default_guard(),
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

    step.add_action_set( self._action_set_preprocess_solution )
    step.add_action_set( self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement )
    step.add_action_set( self._action_set_roll_over_update_of_faces )
    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )
    step.add_action_set( self._action_set_handle_boundary )
    step.add_action_set( self._action_set_update_cell )
    step.add_action_set( self._action_set_project_patch_onto_faces )
    step.add_action_set( self._action_set_compute_final_linear_combination )
    step.add_action_set( self._action_set_AMR )
    step.add_action_set( self._action_set_postprocess_solution )


  @abstractmethod
  def add_entries_to_text_replacement_dictionary(self,d):
    pass


  def add_implementation_files_to_project(self,namespace,output,dimensions):
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
    output.makefile.add_cpp_file( "Abstract" + self._name + ".cpp", generated=True )
    output.makefile.add_cpp_file( self._name + ".cpp", generated=True )

  def set_solver_constants(self, datastring): self._solver_constants = datastring

  def add_solver_constants(self, datastring): self._solver_constants += datastring


  def _init_dictionary_with_default_parameters(self,d):
    """

      This one is called by all algorithmic steps before I invoke
      add_entries_to_text_replacement_dictionary().

      See the remarks on set_postprocess_updated_patch_kernel to understand why
      we have to apply the (partially befilled) dictionary to create a new entry
      for this very dictionary.


    """
    d["NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS"]     = self._patch.dim[0]
    d["OVERLAP"]                        = self._overlap
    d["HALO_SIZE"]                      = int(self._patch_overlap_old.dim[0]/2)
    d["RK_ORDER"]                       = self._rk_order
    d["RK_STEPS"]                       = self.number_of_Runge_Kutta_steps()
    d["SOLVER_INSTANCE"]                = self.get_name_of_global_instance()
    d["SOLVER_NAME"]                    = self._name
    d["UNKNOWN_IDENTIFIER"]             = self._unknown_identifier()
    d["NUMBER_OF_UNKNOWNS"]             = self._unknowns
    d["NUMBER_OF_AUXILIARY_VARIABLES"]  = self._auxiliary_variables
    d["SOLVER_NUMBER"]                  = 22

    d[ "ASSERTION_WITH_1_ARGUMENTS" ] = "nonCriticalAssertion1"
    d[ "ASSERTION_WITH_2_ARGUMENTS" ] = "nonCriticalAssertion2"
    d[ "ASSERTION_WITH_3_ARGUMENTS" ] = "nonCriticalAssertion3"
    d[ "ASSERTION_WITH_4_ARGUMENTS" ] = "nonCriticalAssertion4"
    d[ "ASSERTION_WITH_5_ARGUMENTS" ] = "nonCriticalAssertion5"
    d[ "ASSERTION_WITH_6_ARGUMENTS" ] = "nonCriticalAssertion6"

    if (self._min_meshcell_h > self._max_meshcell_h ):
      raise Exception( "min/max h are inconsistent" )
    d[ "MAX_GRID_CELL_H"] = self._max_meshcell_h
    d[ "MIN_GRID_CELL_H"] = self._min_meshcell_h

    d[ "SOLVER_CONSTANTS" ] = self._solver_constants

    d[ "SOLVER_INCLUDES" ]  = self.user_solver_includes

    d[ "BOUNDARY_CONDITIONS_IMPLEMENTATION"]  = self._boundary_conditions_implementation
    d[ "REFINEMENT_CRITERION_IMPLEMENTATION"] = self._refinement_criterion_implementation
    d[ "INITIAL_CONDITIONS_IMPLEMENTATION"]   = self._initial_conditions_implementation

    d[ "COMPUTE_KERNEL_CALL"]                 = jinja2.Template(self._compute_kernel_call, undefined=jinja2.DebugUndefined).render( **d )

    d[ "ABSTRACT_SOLVER_USER_DECLARATIONS" ]  = jinja2.Template(self._abstract_solver_user_declarations, undefined=jinja2.DebugUndefined).render( **d )
    d[ "ABSTRACT_SOLVER_USER_DEFINITIONS" ]   = jinja2.Template(self._abstract_solver_user_definitions, undefined=jinja2.DebugUndefined).render( **d )
    d[ "SOLVER_USER_DECLARATIONS" ]           = jinja2.Template(self._solver_user_declarations, undefined=jinja2.DebugUndefined).render( **d )
    d[ "SOLVER_USER_DEFINITIONS" ]            = jinja2.Template(self._solver_user_definitions, undefined=jinja2.DebugUndefined).render( **d )
    d[ "START_TIME_STEP_IMPLEMENTATION" ]     = jinja2.Template(self._start_time_step_implementation, undefined=jinja2.DebugUndefined).render( **d )
    d[ "FINISH_TIME_STEP_IMPLEMENTATION" ]    = jinja2.Template(self._finish_time_step_implementation, undefined=jinja2.DebugUndefined).render( **d )
    d[ "CONSTRUCTOR_IMPLEMENTATION"]          = jinja2.Template(self._constructor_implementation, undefined=jinja2.DebugUndefined).render( **d )
    
    d[ "PREPROCESS_RECONSTRUCTED_PATCH" ]     = jinja2.Template(self._preprocess_reconstructed_patch, undefined=jinja2.DebugUndefined).render( **d )
    d[ "POSTPROCESS_UPDATED_PATCH" ]          = jinja2.Template(self._postprocess_updated_patch, undefined=jinja2.DebugUndefined).render( **d )

    d[ "COMPUTE_TIME_STEP_SIZE" ]             = jinja2.Template(self._compute_time_step_size, undefined=jinja2.DebugUndefined).render( **d )
    d[ "COMPUTE_NEW_TIME_STEP_SIZE" ]         = jinja2.Template(self._compute_new_time_step_size, undefined=jinja2.DebugUndefined).render( **d )
    d[ "COMPUTE_MAX_EIGENVALUE" ]             = self._compute_eigenvalue

    d[ "KERNEL_NAMESPACE"]                    = self._kernel_namespace

   
  @property
  def unknowns(self):
    return self._unknowns

  
  @property
  def patch_size(self):
    return self._patch_size


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


  @property 
  def preprocess_reconstructed_patch(self):
    return self._preprocess_reconstructed_patch
 
 
  @preprocess_reconstructed_patch.setter
  def preprocess_reconstructed_patch(self, kernel):
    """!
        
    Please consult exahype2.solvers.fv.FV.preprocess_reconstructed_patch() for 
    a documentation on this routine.

    """
    self._preprocess_reconstructed_patch  = kernel
    self.create_data_structures()
    self.create_action_sets()


  @property
  def name(self):
    return self._name
  

  @property
  def postprocess_updated_patch(self):
    return self._postprocess_updated_patch


  @postprocess_updated_patch.setter
  def postprocess_updated_patch(self, kernel):
    """

    Define a postprocessing routine over the data

    The postprocessing kernel often looks similar to the following code:

  {
    int index = 0;
    dfor( volume, {{NUMBER_OF_GRID_CELLS_PER_PATCH_PER_AXIS}} ) {
      enforceCCZ4constraints( newQ+index );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }


    Within this kernel, you have at least the following variables available:

    - newQ This is a pointer to the whole data structure (one large
        array).
        The patch is not supplemented by a halo layer.
    - oldQWithHalo This is a pointer to the data snapshot before the
        actual update. This data is combined with the halo layer, i.e. if you
        work with 7x7 patches and a halo of 2, the pointer points to a 11x11
        patch.
    - marker

    Furthermore, you can use all the symbols (via Jinja2 syntax) from
    _init_dictionary_with_default_parameters().

    kernel: String
      C++ code that holds the postprocessing kernel

    """
    self._postprocess_updated_patch = kernel
    self.create_data_structures()
    self.create_action_sets()


  @property
  def overlap(self):
    return self._overlap


  @overlap.setter
  def overlap(self,value):
    if value<1:
      raise Exception( "Halo (overlap) size has to be bigger than zero but was {}".format(value) )
    self._overlap = value
    self.create_data_structures()
    self.create_action_sets()


  @property
  def interpolation(self):
    return self._interpolation


  @interpolation.setter
  def interpolation(self,value):
    """
    
    Set the interpolation scheme. If you rely on a built-in operation, then this
    call is all you have to do. Some ExaHyPE solvers however require each solver
    to provide special matrices/operators for some interpolation/restriction 
    variants. If this is the case, you still have to add these matrices manually
    to your solver.
    
    """
    self._interpolation = value

    self.create_data_structures()
    self.create_action_sets()


  @property
  def restriction(self):
    """
    
    Set the restriction scheme. If you rely on a built-in operation, then this
    call is all you have to do. Some ExaHyPE solvers however require each solver
    to provide special matrices/operators for some interpolation/restriction 
    variants. If this is the case, you still have to add these matrices manually
    to your solver.
    
    """
    return self._restriction


  @restriction.setter
  def restriction(self,value):
    self._restriction = value
    
    self.create_data_structures()
    self.create_action_sets()


  def switch_to_heap_storage(self,
                             hold_cell_data_on_heap_with_smart_pointer,
                             hold_face_data_on_heap_with_smart_pointer
                             ):
    """
    
    By default, we hold all data on the call stacks. You can explicitly switch
    to storage on the heap via smart pointers.
    
    @see create_data_structures()
    
    """
    self._hold_cell_data_on_heap_with_smart_pointer = hold_cell_data_on_heap_with_smart_pointer
    self._hold_face_data_on_heap_with_smart_pointer = hold_face_data_on_heap_with_smart_pointer
    
    self.create_data_structures()
    self.create_action_sets()

