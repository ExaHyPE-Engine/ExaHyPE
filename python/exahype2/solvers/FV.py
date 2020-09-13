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
  at the end of the iteration. This way, we ensure that the one from teh previous
  iteration is not overwritten by some adjacent cell halfway through the 
  computation.
  
  Attributes
  -----------
  
  The guard variables are used within the templates and switch them on/off. By 
  default, they all are true, i.e. the actions are triggered in every grid 
  traversal an action in theory could be active.
  
  """
  def __init__(self, name, patch_size, overlap, unknowns, min_h, max_h, plot_grid_properties):
    """
    
      namespace: [string]
        Sequence of strings representing the (nested) namespace. Pass in 
        ["examples", "exahype2", "finitevolumes"] for example.
    
      enclaves: Boolean
  
    """
    self._name  = name
    self._patch = peano4.datamodel.Patch( (patch_size,patch_size,patch_size), unknowns, self._unknown_identifier() )
    self._patch_overlap     = peano4.datamodel.Patch( (2,patch_size,patch_size), unknowns, self._unknown_identifier() )
    self._patch_overlap_new = peano4.datamodel.Patch( (2,patch_size,patch_size), unknowns, self._unknown_identifier() + "New" )
    self._patch_overlap.generator.merge_method_definition = peano4.toolbox.blockstructured.get_face_overlap_merge_implementation(self._patch_overlap)
    
    self._patch_overlap.generator.includes += """
#include "peano4/utils/Loop.h" 
"""

    self._guard_copy_new_face_data_into_face_data = "true"
    self._guard_adjust_cell  = "true"
    self._guard_AMR          = "not marker.isRefined()"
    self._guard_project_patch_onto_faces = "true"
    self._guard_update_cell  = "true"
    self._guard_touch_face_first_time_in_time_step = "fineGridFaceLabel.getBoundary()"

    self._min_h                = min_h
    self._max_h                = max_h 
    self._plot_grid_properties = plot_grid_properties
    
    if min_h>max_h:
       print( "Error: min_h (" + str(min_h) + ") is bigger than max_h (" + str(max_h) + ")" )
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


  """ @todo Make Jinja2 template """
  AdjustCellTemplate = ""
  
  
  """ @todo Make Jinja2 template """
  _AMRTemplate = ""


  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    d["IS_GRID_CREATION"] = "true"
    
    step.add_action_set( peano4.toolbox.blockstructured.ProjectPatchOntoFaces(
      self._patch,
      self._patch_overlap,
      self._guard_project_patch_onto_faces, 
      self._get_default_includes() + self.get_user_includes()
    ))
    step.add_action_set( peano4.toolbox.blockstructured.ApplyFunctorOnPatch(
      self._patch,self.AdjustCellTemplate.format(**d),
      self._guard_adjust_cell,
      self._get_default_includes() + self.get_user_includes()
    ))
    if evaluate_refinement_criterion:
      step.add_action_set( exahype2.grid.AMROnPatch(
        self._patch,self.AMRTemplate.format(**d),
        "not marker.isRefined()", 
        self._get_default_includes() + self.get_user_includes()
      ))
    pass
  
  
  def add_actions_to_plot_solution(self, step):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    
    step.add_action_set( peano4.toolbox.blockstructured.PlotPatchesInPeanoBlockFormat("solution" + self._name,self._patch, self._unknown_identifier()) )

    if self._plot_grid_properties:    
        step.add_action_set( peano4.toolbox.PlotGridInPeanoBlockFormat( "grid" + self._name,None ))

    step.add_action_set( peano4.toolbox.blockstructured.ProjectPatchOntoFaces(
      self._patch,self._patch_overlap,
      self._guard_project_patch_onto_faces,
      self._get_default_includes() + self.get_user_includes()
    ))
    #step.add_action_set( peano4.toolbox.blockstructured.ApplyFunctorOnPatch(
    #  self._patch,
    #  self.AdjustCellTemplate.format(**d),
    #  self._guard_adjust_cell,
    #  self._get_default_includes() + self.get_user_includes()
    #))
    pass
  
  """ @todo Make Jinja2 template """
  HandleBoundaryTemplate = ""


  """ 
   This is a Jinja 2 template 
  """
  HandleCellTemplate = jinja2.Template( "" )
 
 
  def add_actions_to_perform_time_step(self, step):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    d["IS_GRID_CREATION"] = "false"


    step.add_action_set( peano4.toolbox.blockstructured.ReconstructPatchAndApplyFunctor(
      self._patch,
      self._patch_overlap,
      self.HandleCellTemplate.render(**d),
      self.HandleBoundaryTemplate.format(**d),
      self._guard_update_cell,
      self._guard_touch_face_first_time_in_time_step,
      self._get_default_includes() + self.get_user_includes() + """#include "exahype2/NonCriticalAssertions.h" 
"""
    )) 
    step.add_action_set( peano4.toolbox.blockstructured.ProjectPatchOntoFaces(
      self._patch,
      self._patch_overlap_new,
      self._guard_project_patch_onto_faces,
      self._get_default_includes() + self.get_user_includes()
    ))
    step.add_action_set( peano4.toolbox.blockstructured.ApplyFunctorOnPatch(
      self._patch,self.AdjustCellTemplate.format(**d),
      self._guard_adjust_cell,
      self._get_default_includes() + self.get_user_includes()
    ))
    step.add_action_set( exahype2.grid.AMROnPatch(
      self._patch,self.AMRTemplate.format(**d),  
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
    d["NUMBER_OF_VOLUMES_PER_AXIS"] = self._patch.dim[0]
    d["HALO_SIZE"]                  = int(self._patch_overlap.dim[0]/2)
    d["SOLVER_INSTANCE"]            = self.get_name_of_global_instance()
    d["SOLVER_NAME"]                = self._name
    d["UNKNOWN_IDENTIFIER"]         = self._unknown_identifier()
    d["NUMBER_OF_UNKNOWNS"]         = self._patch.no_of_unknowns
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
 
