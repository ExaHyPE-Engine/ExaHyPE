# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel
import peano4.output.TemplatedHeaderFile
import peano4.output.TemplatedHeaderImplementationFilePair
import peano4.output.Jinja2TemplatedHeaderFile
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import exahype2.grid.AMROnPatch

import jinja2

from abc import abstractmethod


class FV(object):
  """ 
    An abstract finite volume solver step sizes that works on patch-based
    AMR with a halo layer of one.
  
    We use two overlaps in this case: the standard one and one we call new. In the
    time stepping, we use the new one to project our data to. Then we roll it over
    at the end of the iteration. This way, we ensure that the one from the previous
    iteration is not overwritten by some adjacent cell halfway through the 
    computation.
  
  
  
    Attributes:
  
    The guard variables are used within the templates and switch them on/off. By 
    default, they all are true, i.e. the actions are triggered in every grid 
    traversal an action in theory could be active.
  
    
    _guard_copy_new_face_data_into_face_data: string
      This is a predicate, i.e. identifies in C code when to trigger 
      the underlying activity.
      The routine triggers the roll-over, i.e. the actual commit of 
      a new time step. It is logically called after a time step and
      hence in touch -face-last-time in the secondary traversal.

    _guard_handle_boundary: string
      This is a predicate, i.e. identifies in C code when to trigger 
      the underlying activity.
      Updates the boundary. By definition, boundary cells in the domain
      are skeleton cells. Furthermore, the initialiation/setting of 
      boundary conditions only has to happen once per time step. We 
      hence trigger this routine for skeletons in the primary sweeps.

    _guard_project_patch_onto_faces: string
      This is a predicate, i.e. identifies in C code when to trigger 
      the underlying activity.
      This routine guards the mapping of an updated cell content onto a
      cell's faces. The destination in QNew, i.e. is not "seen" by 
      others unless the faces' new data are committed. Therefore, I 
      trigger this routine in the primary sweep for skeleton cells and
      in the secondary  sweep for enclave cells. The primary calls 
      ensure that data are mapped onto the faces before faces are 
      exchanged with neighbour partitions.
      
    _guard_handle_cell: string
      This is a predicate, i.e. identifies in C code when to trigger 
      the underlying activity.
      Actual FV kernel invocation.
      
    _patch_overlap.generator.send_condition: string
      This is a predicate, i.e. identifies in C code when to trigger 
      the underlying activity.
      Send out the faces in the grid initialisation. There's no need to 
      send out data in other iterations, as all data exchange is 
      realised through the QNew field. This is different to the plain
      FV realisation, where data exchange happens through Q: The enclave
      tasking exchanges QNew and then rolls data over from QNew into Q.
      The plain FV scheme rolls over QNew into Q and then exchanges the
      data.
    
    _patch_overlap.generator: string
      See documentation of _patch_overlap.generator.send_condition.
      
    _patch_overlap.generator.merge_method_definition: string
      See documentation of _patch_overlap.generator.send_condition.

    _guard_AMR: string
      This is a predicate, i.e. identifies in C code when to trigger 
      the underlying activity.
      AMR is active throughout the grid construction. After that, it is
      only available for skeleton cells.
    
    _patch_overlap_new.generator.send_condition: string
      This is a predicate, i.e. identifies in C code when to trigger 
      the underlying activity.
      See discussion of _patch_overlap.generator.send_condition for 
      details. As we effectively disable the data exchange for Q and
      instead ask for data exchange of QNew, we have to add merge 
      operations to QNew.
      
    _patch_overlap_new.generator.receive_and_merge_condition: string
      See _patch_overlap_new.generator.send_condition.
      
    _patch_overlap_new.generator.merge_method_definition: string
      See _patch_overlap_new.generator.send_condition.
  
  
  
  
  """
  def __init__(self, name, patch_size, overlap, unknowns, auxiliary_variables, min_h, max_h, plot_grid_properties):
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
     
  min_h: double
  
  max_h: double
  
  plot_grid_properties: Boolean
     Clarifies whether a dump of the data should be enriched with grid info
     (such as enclave status flags), too.
 
    """
    self._name              = name
    self._patch             = peano4.datamodel.Patch( (patch_size,patch_size,patch_size),    unknowns+auxiliary_variables, self._unknown_identifier() )
    self._patch_overlap     = peano4.datamodel.Patch( (2,patch_size,patch_size), unknowns+auxiliary_variables, self._unknown_identifier() )
    self._patch_overlap_new = peano4.datamodel.Patch( (2,patch_size,patch_size), unknowns+auxiliary_variables, self._unknown_identifier() + "New" )
    self._patch_overlap.generator.merge_method_definition = peano4.toolbox.blockstructured.get_face_overlap_merge_implementation(self._patch_overlap)
    
    self._patch_overlap.generator.includes += """
#include "peano4/utils/Loop.h" 
"""

    ##
    ## Sollte alles auf not marker.isRefined() fuer cells stehen
    ##
    self._guard_copy_new_face_data_into_face_data  = "not marker.isRefined()"
    self._guard_adjust_cell                        = "not marker.isRefined()"
    self._guard_AMR                                = "not marker.isRefined()"
    self._guard_project_patch_onto_faces           = "not marker.isRefined()"
    self._guard_update_cell                        = "not marker.isRefined()"
    self._guard_handle_boundary                    = "not marker.isRefined() and fineGridFaceLabel.getBoundary()"

    self._min_h                = min_h
    self._max_h                = max_h 
    self._plot_grid_properties = plot_grid_properties
    
    self._unknowns             = unknowns
    self._auxiliary_variables  = auxiliary_variables
    
    if min_h>max_h:
       print( "Error: min_h (" + str(min_h) + ") is bigger than max_h (" + str(max_h) + ")" )

    self._template_adjust_cell     = jinja2.Template( "" )
    self._template_AMR             = jinja2.Template( "" )
    self._template_handle_boundary = jinja2.Template( "" )
    self._template_update_cell     = jinja2.Template( "" )

    self._reconstructed_array_memory_location=peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack
    pass
  
  
  
  def _unknown_identifier(self):
    return self._name+"Q"
  

  def get_name_of_global_instance(self):
    return "InstanceOf" + self._name

  
  def add_to_Peano4_datamodel( self, datamodel ):
    """
    
      Add all required data to the Peano4 project's datamodel 
      so it is properly built up
      
    """
    datamodel.add_cell(self._patch)
    datamodel.add_face(self._patch_overlap)
    datamodel.add_face(self._patch_overlap_new)
 
 
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    """
      Tell Peano what data to move around
      
      Inform Peano4 step which data are to be moved around via the 
      use_cell and use_face commands. This operation is generic from
      ExaHyPE's point of view, i.e. I use it for all grid sweep types. 
    
    """
    step.use_cell(self._patch)
    step.use_face(self._patch_overlap)
    step.use_face(self._patch_overlap_new)

  
  def _get_default_includes(self):
    return """
#include "tarch/la/Vector.h" 

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "SolverRepository.h"

#include "exahype2/PatchUtils.h"
#include "exahype2/fv/BoundaryConditions.h"
"""


  @abstractmethod
  def get_user_includes(self):
    """
  
    Use this to add include statements to the generated C++ code. Is there for
    subclasses to hook in.
  
    """
    return ""


  def add_actions_to_init_grid(self, step):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)

    step.add_action_set( peano4.toolbox.blockstructured.ApplyFunctorOnPatch(
      self._patch,self._template_adjust_cell.render(**d),
      self._guard_adjust_cell,
      self._get_default_includes() + self.get_user_includes()
    ))
    step.add_action_set( peano4.toolbox.blockstructured.ProjectPatchOntoFaces(
      self._patch,
      self._patch_overlap,
      self._guard_project_patch_onto_faces, 
      self._get_default_includes() + self.get_user_includes()
    ))
    

  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    d["IS_GRID_CREATION"] = "true"
    
    step.add_action_set( peano4.toolbox.blockstructured.ApplyFunctorOnPatch(
      self._patch,self._template_adjust_cell.render(**d),
      self._guard_adjust_cell,
      self._get_default_includes() + self.get_user_includes()
    ))
    if evaluate_refinement_criterion:
      step.add_action_set( exahype2.grid.AMROnPatch(
        self._patch,self._template_AMR.render(**d),
        "not marker.isRefined()", 
        self._get_default_includes() + self.get_user_includes()
      ))
    pass
  
  
  def add_actions_to_plot_solution(self, step):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    
    step.add_action_set( peano4.toolbox.blockstructured.PlotPatchesInPeanoBlockFormat("solution-" + self._name,self._patch, self._unknown_identifier()) )

    if self._plot_grid_properties:    
        step.add_action_set( peano4.toolbox.PlotGridInPeanoBlockFormat( "grid" + self._name,None ))

    step.add_action_set( peano4.toolbox.blockstructured.ProjectPatchOntoFaces(
      self._patch,self._patch_overlap,
      self._guard_project_patch_onto_faces,
      self._get_default_includes() + self.get_user_includes()
    ))
    pass
   
 
  def add_actions_to_perform_time_step(self, step):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    d["IS_GRID_CREATION"] = "false"


    step.add_action_set( peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor(
      self._patch,
      self._patch_overlap,
      self._template_update_cell.render(**d),
      self._template_handle_boundary.render(**d),
      self._guard_update_cell,
      self._guard_handle_boundary,
      self._get_default_includes() + self.get_user_includes() + """#include "exahype2/NonCriticalAssertions.h" 
""",
      self._reconstructed_array_memory_location
    )) 
    step.add_action_set( peano4.toolbox.blockstructured.ProjectPatchOntoFaces(
      self._patch,
      self._patch_overlap_new,
      self._guard_project_patch_onto_faces,
      self._get_default_includes() + self.get_user_includes()
    ))
    step.add_action_set( peano4.toolbox.blockstructured.ApplyFunctorOnPatch(
      self._patch,self._template_adjust_cell.render(**d),
      self._guard_adjust_cell,
      self._get_default_includes() + self.get_user_includes()
    ))
    step.add_action_set( exahype2.grid.AMROnPatch(
      self._patch,self._template_AMR.render(**d),  
      self._guard_AMR,
      self._get_default_includes() + self.get_user_includes()
    ))
    step.add_action_set( peano4.toolbox.blockstructured.BackupPatchOverlap(
      self._patch_overlap_new,
      self._patch_overlap,
      False,
      self._guard_copy_new_face_data_into_face_data,
      self._get_default_includes() + self.get_user_includes()
    ))
    pass


  @abstractmethod
  def add_entries_to_text_replacement_dictionary(self,d):
    pass  

  
  def add_implementation_files_to_project(self,namespace,output):
    """
    
      makefile is an instance of peano4.output.Makefile, so you can 
      add your files to this instance.
      
    """

    templatefile_prefix = os.path.dirname( os.path.realpath(__file__) ) + "/" + self.__class__.__name__
    
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
      True)
    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".template.h",
      templatefile_prefix + ".template.cpp",
      self._name, 
      namespace,
      ".", 
      implementationDictionary,
      False)

    output.add( generated_abstract_header_file )
    output.add( generated_solver_files )
    output.makefile.add_cpp_file( "Abstract" + self._name + ".cpp" )
    output.makefile.add_cpp_file( self._name + ".cpp" )


  def _init_dictionary_with_default_parameters(self,d):
    """
    
    """
    d["NUMBER_OF_VOLUMES_PER_AXIS"]     = self._patch.dim[0]
    d["HALO_SIZE"]                      = int(self._patch_overlap.dim[0]/2)
    d["SOLVER_INSTANCE"]                = self.get_name_of_global_instance()
    d["SOLVER_NAME"]                    = self._name
    d["UNKNOWN_IDENTIFIER"]             = self._unknown_identifier()
    d["NUMBER_OF_UNKNOWNS"]             = self._unknowns
    d["NUMBER_OF_AUXILIARY_VARIABLES"]  = self._auxiliary_variables
        
    if self._patch_overlap.dim[0]/2!=1:
      print( "ERROR: Finite Volume solver currently supports only a halo size of 1")
    d[ "ASSERTION_WITH_1_ARGUMENTS" ] = "nonCriticalAssertion1"
    d[ "ASSERTION_WITH_2_ARGUMENTS" ] = "nonCriticalAssertion2"
    d[ "ASSERTION_WITH_3_ARGUMENTS" ] = "nonCriticalAssertion3"
    d[ "ASSERTION_WITH_4_ARGUMENTS" ] = "nonCriticalAssertion4"
    d[ "ASSERTION_WITH_5_ARGUMENTS" ] = "nonCriticalAssertion5"
    d[ "ASSERTION_WITH_6_ARGUMENTS" ] = "nonCriticalAssertion6"
    d[ "MAX_H"] = self._min_h
    d[ "MIN_H"] = self._max_h
 
