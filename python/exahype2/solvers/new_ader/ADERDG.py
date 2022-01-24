
# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org

 
import peano4.toolbox.blockstructured
import peano4.output
import exahype2.grid

import os
from abc import abstractmethod

from enum import IntEnum
from sympy.printing import str




class Polynomials(IntEnum):
  Gauss_Legendre = 0,
  Gauss_Lobatto = 1
  

    

class ADERDG( object ):
  def __init__(self, name, order, polynomials, unknowns, auxiliary_variables, min_cell_h, max_cell_h, plot_grid_properties=False):

    self._name = name
    
    self._order = order
    self._unknowns = unknowns
    self._auxiliary_variables = auxiliary_variables

    self._min_cell_h         = min_cell_h
    self._max_cell_h         = max_cell_h 
    if min_cell_h>max_cell_h:
      raise Exception( "Error: min_cell_h (" + str(min_cell_h) + ") is bigger than max_cell_h (" + str(max_cell_h) + ")" )

    self._plot_grid_properties = plot_grid_properties    
    
    if polynomials == Polynomials.Gauss_Legendre:
      self._polynomials = polynomials
    elif polynomials == Polynomials.Gauss_Lobatto:
      self._polynomials = polynomials
    else:
      raise Exception("""Currently only Gauss-Legendre and Gauss-Lobatto bases are available,
       to choose from among these please set polynomials to 0 or 1 respectively.""")

    self.solver_constants_         = ""
    self.user_action_set_includes  = ""
    self.user_solver_includes      = ""
    self.plot_description = ""    
    
    self.create_data_structures()
    self.create_action_sets()


            
  @abstractmethod
  def create_data_structures(self):
    #TODO
    return

  @abstractmethod
  def create_action_sets(self):
    self._action_set_update_face_label = exahype2.grid.UpdateFaceLabel( self._name )  
    self._action_set_update_cell_label = exahype2.grid.UpdateCellLabel( self._name ) 
        
    self._action_set_update_cell                                                      = None
    self._action_set_solve_Riemann_problem                                            = None



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
    #TODO
    return
  
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    #TODO
    return  
  
  def add_actions_to_init_grid(self, step):  
    #TODO
    return
  
  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    #TODO
    return
  
  def add_actions_to_plot_solution(self, step, output_path):
    #TODO
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    step.add_action_set( peano4.toolbox.blockstructured.PlotPatchesInPeanoBlockFormat( 
      filename=output_path + "solution-" + self._name, 
      patch=self._current_time_step, 
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
    #TODO
    return
  
    
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
  
  @abstractmethod
  def add_entries_to_text_replacement_dictionary(self,d):
    pass  

  def _init_dictionary_with_default_parameters(self,d):
    d["ORDER"]                       = self._order
    d["SOLVER_INSTANCE"]                = self.get_name_of_global_instance()
    d["SOLVER_NAME"]                    = self._name
    d["UNKNOWN_IDENTIFIER"]             = self._unknown_identifier()
    d["NUMBER_OF_UNKNOWNS"]             = self._unknowns
    d["NUMBER_OF_AUXILIARY_VARIABLES"]  = self._auxiliary_variables

    d["POLYNOMIALS"]                    = str(self._polynomials)

    d[ "ASSERTION_WITH_1_ARGUMENTS" ] = "nonCriticalAssertion1"
    d[ "ASSERTION_WITH_2_ARGUMENTS" ] = "nonCriticalAssertion2"
    d[ "ASSERTION_WITH_3_ARGUMENTS" ] = "nonCriticalAssertion3"
    d[ "ASSERTION_WITH_4_ARGUMENTS" ] = "nonCriticalAssertion4"
    d[ "ASSERTION_WITH_5_ARGUMENTS" ] = "nonCriticalAssertion5"
    d[ "ASSERTION_WITH_6_ARGUMENTS" ] = "nonCriticalAssertion6"

    if (self._min_cell_h > self._max_cell_h ):
      raise Exception( "min/max h are inconsistent" )
    d[ "MAX_CELL_H"] = self._min_cell_h
    d[ "MIN_CELL_H"] = self._min_cell_h

    d[ "SOLVER_CONSTANTS" ] = self.solver_constants_
    d[ "SOLVER_INCLUDES" ]  = self.get_user_solver_includes()
    
    
    
    
    
    
    
    
    
    
      