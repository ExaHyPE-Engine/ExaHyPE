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
import exahype2.solvers.fv.actionsets

    
class FV(object):
  """ 
    An abstract finite volume solver step sizes that works on patch-based
    AMR with a halo layer of one.
  
    We use two overlaps in this case: the standard one and one we call new. In the
    time stepping, we use the new one to project our data to. Then we roll it over
    at the end of the iteration. This way, we ensure that the one from the previous
    iteration is not overwritten by some adjacent cell halfway through the 
    computation.

    
    Data flow:
    
    The class has a number of guards. They are boolean expression which control 
    whether data is stored and communicated. If you want to redefine these guards,
    you have to redefine create_data_structures() and reset the predicates after
    you have called the superclass' operation. I recommend that you refrain from
    defining completely new predicates. Use the predefined predicates instead and
    refine them by adding more and and or clauses.
  
  
    Parallelisation:
    
    I do equip both Q and NewQ with proper merge routines. However, all merge guards
    set to "never" by default. If you need some data exchange, you have to activate
    them manually.
    
    
    Add further data structures:
    
    To add more data structures, plug into create_data_structures() and 
    create_action_sets().
    
  """
  
      
  def __init__(self, name, patch_size, overlap, unknowns, auxiliary_variables, min_volume_h, max_volume_h, plot_grid_properties):
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
     
  min_volume_h: double
     This size refers to the individual Finite Volume.
    
  max_volume_h: double
     This size refers to the individual Finite Volume.
  
  plot_grid_properties: Boolean
     Clarifies whether a dump of the data should be enriched with grid info
     (such as enclave status flags), too.
 
    """
    self._name              = name
    
    self._min_volume_h         = min_volume_h
    self._max_volume_h         = max_volume_h 
    self._plot_grid_properties = plot_grid_properties

    self._patch_size           = patch_size  
    self._unknowns             = unknowns
    self._auxiliary_variables  = auxiliary_variables

    self.solver_constants_ = ""
    
    if min_volume_h>max_volume_h:
       print( "Error: min_volume_h (" + str(min_volume_h) + ") is bigger than max_volume_h (" + str(max_volume_h) + ")" )

    self._reconstructed_array_memory_location=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack

    self._solver_template_file_class_name = None

    self.plot_description = ""
    
    self.create_data_structures()
    self.create_action_sets()


  def __str__(self):
    result = """
Name:                   """ + self._name + """
Type:                   """ + self.__class__.__name__ + """
Patch size:             """ + str( self._patch_size ) + """  
Unknowns:               """ + str( self._unknowns ) + """
Auxiliary variables:    """ + str( self._auxiliary_variables ) + """
h_volume_min:           """ + str( self._min_volume_h ) + """
h_volume_max:           """ + str( self._max_volume_h ) + """
"""
    return result


  __repr__ = __str__
  

  @abstractmethod
  def create_data_structures(self):
    """
    
     Recall in subclasses if you wanna change the number of unknowns
     or auxiliary variables. See class description's subsection on 
     data flow.
     
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
    #self._patch_overlap_update.generator.merge_method_definition = peano4.toolbox.blockstructured.get_face_overlap_merge_implementation(self._patch_overlap_update)
    
    self._patch_overlap_old.generator.includes     += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 
"""
    self._patch_overlap_new.generator.includes += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 
"""
   
    self._patch.generator.store_persistent_condition = self._store_cell_data_default_predicate()
    self._patch.generator.load_persistent_condition  = self._load_cell_data_default_predicate()
    
    self._patch_overlap_old.generator.send_condition               = "false"
    self._patch_overlap_old.generator.receive_and_merge_condition  = "false"

    self._patch_overlap_new.generator.send_condition               = "true"
    self._patch_overlap_new.generator.receive_and_merge_condition  = "true"

    self._patch_overlap_update.generator.send_condition               = "false"
    self._patch_overlap_update.generator.receive_and_merge_condition  = "false"

    self._patch_overlap_old.generator.store_persistent_condition   = self._store_face_data_default_predicate()
    self._patch_overlap_old.generator.load_persistent_condition    = self._load_face_data_default_predicate()

    self._patch_overlap_new.generator.store_persistent_condition   = self._store_face_data_default_predicate()
    self._patch_overlap_new.generator.load_persistent_condition    = self._load_face_data_default_predicate()

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
     
    """
    self._action_set_initial_conditions                       = exahype2.solvers.fv.actionsets.InitialCondition(self, self._store_cell_data_default_predicate(), "true" )
    self._action_set_initial_conditions_for_grid_construction = exahype2.solvers.fv.actionsets.InitialCondition(self, self._store_cell_data_default_predicate(), "false")
    self._action_set_AMR                                      = exahype2.solvers.fv.actionsets.AMROnPatch(solver=self, predicate=self._store_cell_data_default_predicate(), build_up_new_refinement_instructions=True, implement_previous_refinement_instructions=True )
    self._action_set_AMR_commit_without_further_analysis      = exahype2.solvers.fv.actionsets.AMROnPatch(solver=self, predicate=self._store_cell_data_default_predicate(), build_up_new_refinement_instructions=True, implement_previous_refinement_instructions=True )
    self._action_set_handle_boundary                          = exahype2.solvers.fv.actionsets.HandleBoundary(self, self._store_face_data_default_predicate() )
    self._action_set_project_patch_onto_faces                 = exahype2.solvers.fv.actionsets.ProjectPatchOntoFaces(self, self._store_cell_data_default_predicate())
    self._action_set_roll_over_update_of_faces                = exahype2.solvers.fv.actionsets.RollOverUpdatedFace(self, self._store_face_data_default_predicate())
    self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = exahype2.solvers.fv.actionsets.DynamicAMR( solver=self )

    self._action_set_update_face_label = exahype2.grid.UpdateFaceLabel( self._name )  
    self._action_set_update_cell_label = exahype2.grid.UpdateCellLabel( self._name ) 
        
    self._action_set_update_cell                                                      = None


  def _store_cell_data_default_predicate(self):
    return "not marker.isRefined() " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction"
  
  
  def _load_cell_data_default_predicate(self):
    return "not marker.isRefined() " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridInitialisation"


  def _store_face_data_default_predicate(self):
    return "not marker.isRefined() " + \
           "and repositories::" + self.get_name_of_global_instance() + ".getSolverState()!=" + self._name + "::SolverState::GridConstruction"
  
  
  def _load_face_data_default_predicate(self):
    return "not marker.isRefined() " + \
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


  @abstractmethod
  def get_user_includes(self):
    """
  
    Use this to add include statements to the generated C++ code. Is there for
    subclasses to hook in.
  
    """
    return ""
  
  

  def add_actions_to_init_grid(self, step):
    """
    
     The AMR stuff has to be the very first thing. Actually, the AMR routines' 
     interpolation doesn't play any role here. But the restriction indeed is
     very important, as we have to get the face data for BCs et al. The action 
     set order is inverted while we ascend within the tree again. Therefore, we
     add the AMR action set first which means it will be called last when we go
     from fine to coarse levels within the tree.
    
    """
    step.add_action_set( self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement )
    step.add_action_set( self._action_set_initial_conditions ) 
    step.add_action_set( self._action_set_project_patch_onto_faces )
    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )
    step.add_action_set( self._action_set_roll_over_update_of_faces )

    
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
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    
    step.add_action_set( self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement )
    step.add_action_set( self._action_set_AMR_commit_without_further_analysis )

    step.add_action_set( peano4.toolbox.blockstructured.PlotPatchesInPeanoBlockFormat( 
      filename=output_path + "solution-" + self._name, 
      patch=self._patch, 
      dataset_name=self._unknown_identifier(), 
      description=self.plot_description,
      guard_predicate="repositories::plotFilter.plotPatch(marker) and " + self._load_cell_data_default_predicate(),
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
        guard_predicate="repositories::plotFilter.plotPatch(marker) and " + self._load_cell_data_default_predicate(),
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

    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )
    step.add_action_set( self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement )
    step.add_action_set( self._action_set_handle_boundary )
    step.add_action_set( self._action_set_update_cell )
    step.add_action_set( self._action_set_project_patch_onto_faces )
    step.add_action_set( self._action_set_AMR )
    step.add_action_set( self._action_set_roll_over_update_of_faces )


  @abstractmethod
  def add_entries_to_text_replacement_dictionary(self,d):
    pass


  def add_implementation_files_to_project(self,namespace,output):
    """
     The ExaHyPE2 project will call this operation when it sets
     up the overall environment.

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

  def get_solver_constants(self): return self.solver_constants_


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

    d[ "INCLUDES"] = self.get_user_includes()

    d[ "SOLVER_CONSTANTS" ] = self.solver_constants_
