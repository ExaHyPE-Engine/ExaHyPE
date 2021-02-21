# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel

import peano4.output.Jinja2TemplatedHeaderFile
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

from peano4.solversteps.ActionSet import ActionSet

import jinja2
import math

from abc import abstractmethod

from enum import IntEnum

from exahype2.solvers.dg.LagrangeBasis import GaussLegendreBasis, GaussLobattoBasis


class Polynomials(IntEnum):
  """
  
   Superclass has to be IntEnum, as I use this one within Jinja2 templates
   where I struggled to compare against enum variants. I however can always
   compare against integers.
  
  """
  Gauss_Legendre = 0,
  Gauss_Lobatto = 1
  
  
class AbstractADERDGActionSet( ActionSet ):
  def __init__(self,solver):
    """
   
    solver: ADERDG
      Reference to creating class 
   
    """
    self._solver = solver
    pass
  
  
  def get_action_set_name(self):
    return __name__.replace(".py", "").replace(".", "_")


  def user_should_modify_template(self):
    return False


  def get_includes(self):
    return """
#include <functional>
""" + self._solver._get_default_includes() + self._solver.get_user_includes() 


class AMR(AbstractADERDGActionSet):
  TemplateAMR = """
  if (not marker.isRefined()) { 
    ::exahype2::RefinementCommand refinementCriterion = ::exahype2::getDefaultRefinementCommand();

    if (tarch::la::max( marker.h() ) > repositories::{{SOLVER_INSTANCE}}.getMaxMeshSize() ) {
      refinementCriterion = ::exahype2::RefinementCommand::Refine;
    } 
    else {
      int index = 0;
      tarch::la::Vector<Dimensions,double> x;
      dfor( quadraturePoint, {{ORDER}}+1 ) {
        for (int d=0; d<Dimensions; d++) {
          x(d) = repositories::{{SOLVER_INSTANCE}}.QuadraturePoints[quadraturePoint(d)] * marker.h()(d) + marker.getOffset()(d);
        }
        refinementCriterion = refinementCriterion and repositories::{{SOLVER_INSTANCE}}.refinementCriterion(
          fineGridCell{{UNKNOWN_IDENTIFIER}}.value + index,
          x,
          marker.h(),
          repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp()
        );
        index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
      }
     
      if (refinementCriterion==::exahype2::RefinementCommand::Refine and tarch::la::max( marker.h() ) < repositories::{{SOLVER_INSTANCE}}.getMinMeshSize() ) {
        refinementCriterion = ::exahype2::RefinementCommand::Keep;
      } 
      else if (refinementCriterion==::exahype2::RefinementCommand::Coarsen and 3.0* tarch::la::max( marker.h() ) > repositories::{{SOLVER_INSTANCE}}.getMaxMeshSize() ) {
        refinementCriterion = ::exahype2::RefinementCommand::Keep;
      } 
    }
    
    _localRefinementControl.addCommand( marker.x(), marker.h(), refinementCriterion );
  } 
"""


  def __init__(self,solver):
    AbstractADERDGActionSet.__init__(self, solver)

  
  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)      
      result = jinja2.Template( self.TemplateAMR ).render(**d)
      pass 
    if operation_name==peano4.solversteps.ActionSet.OPERATION_BEGIN_TRAVERSAL:
      result = """
  _localRefinementControl.clear();
"""
    if operation_name==peano4.solversteps.ActionSet.OPERATION_END_TRAVERSAL:
      result = """
  repositories::refinementControl.merge( _localRefinementControl );
"""
    return result


  def get_body_of_getGridControlEvents(self):
    return """
  return repositories::refinementControl.getGridControlEvents();
""" 

  
  def get_attributes(self):
    return """
    ::exahype2::RefinementControl         _localRefinementControl;
"""



class AdjustCell(AbstractADERDGActionSet):
  TemplateAdjustCell = """
  if (not marker.isRefined()) { 
    int index = 0;
    tarch::la::Vector<Dimensions,double> x;
    dfor( quadraturePoint, {{ORDER}}+1 ) {
      for (int d=0; d<Dimensions; d++) {
        x(d) = repositories::{{SOLVER_INSTANCE}}.QuadraturePoints[quadraturePoint(d)] * marker.h()(d) + marker.getOffset()(d);
      }
      repositories::{{SOLVER_INSTANCE}}.adjustSolution(
        fineGridCell{{UNKNOWN_IDENTIFIER}}.value + index,
        x,
        repositories::{{SOLVER_INSTANCE}}.getMinTimeStamp()
      );
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  } 
"""

  
  def __init__(self,solver):
    AbstractADERDGActionSet.__init__(self, solver)

  
  def get_body_of_operation(self,operation_name):
    result = "\n"
    if operation_name==ActionSet.OPERATION_TOUCH_CELL_FIRST_TIME:
      d = {}
      self._solver._init_dictionary_with_default_parameters(d)
      self._solver.add_entries_to_text_replacement_dictionary(d)      
      result = jinja2.Template(self.TemplateAdjustCell).render(**d)
      pass 
    return result


class ADERDG(object):
  """ 
    An abstract ADER-DG solver

    Our ADER-DG solver hijacks the patch-based data structures. Topologically,
    this makes sense, as the Cartesian mesh is just distorted - depending on 
    whether you use Gauss-Lobatto or Gauss-Legendre nodes. A big semantic 
    difference can be found for the overlaps: We use an overlap of two but the 
    layer closer to the actual patch hosts a nodal representation of the 
    solution. The outer ghost layer holds a nodel representation of the solution
    gradient along the boundary.
    
    Further to the width of the overlaps, our face data always is space-time.
    For a global time stepping, this is not required, as we can directly apply
    the space-time integral. For all other routines, it is essential.
    
    We use two overlaps: one with gradient and solution, and one with the 
    outcome of the Riemann solve.
  
  """
  



  def __init__(self, name, order, unknowns, auxiliary_variables, polynomials, min_h, max_h, plot_grid_properties):
    """
  name: string
     A unique name for the solver. This one will be used for all generated 
     classes. Also the C++ object instance later on will incorporate this 
     name.
     
  order: int
     Order of the chosen polynomials.
     
  unknowns: int
     Number of unknowns per Finite Volume voxel.
     
  auxiliary_variables: int 
     Number of auxiliary variables per Finite Volume voxel. Eventually, both
     unknowns and auxiliary_variables are merged into one big vector if we 
     work with AoS. But the solver has to be able to distinguish them, as 
     only the unknowns are subject to a hyperbolic formulation.
     
  polynomials: Polynomials
     Type of polynomials used within the cell.
  
  min_h: double
  
  max_h: double
  
  plot_grid_properties: Boolean
     Clarifies whether a dump of the data should be enriched with grid info
     (such as enclave status flags), too.
 
    """
    if order<=0:
      raise Exception( "Order has to be positive. Order 0 is a Finite Volume scheme. Use FV solver instead")
    
    self._name                   = name
    self._order                  = order
    self._min_h                  = min_h
    self._max_h                  = max_h 
    self._plot_grid_properties   = plot_grid_properties
    
    self._preprocess_reconstructed_patch      = ""
    self._postprocess_updated_patch           = ""

    # @todo raus    
    self._adaptive_time_stepping = False 
    
    # ADER-DG CFL factors        
    # Up to order 4, computed via von Neumann analysis [1]; above, determined empirically by M. Dumbser)
    # [1] M. Dumbser, D. S. Balsara, E. F. Toro, and C.-D. Munz, 
    # ‘A unified framework for the construction of one-step finite volume and discontinuous Galerkin schemes on unstructured meshes’, 
    # Journal of Computational Physics, vol. 227, no. 18, pp. 8209–8253, Sep. 2008.
    self._ader_pnpm_stability_factor = [1.0,   0.33,  0.17, 0.1,  0.069, 0.045, 0.038, 0.03, 0.02, 0.015][order]
    
    self._unknowns             = unknowns
    self._auxiliary_variables  = auxiliary_variables
    
    if polynomials is Polynomials.Gauss_Legendre:
      self._basis = GaussLegendreBasis(order+1)
    elif polynomials is Polynomials.Gauss_Lobatto:
      self._basis = GaussLobattoBasis(order+1)
    else:
      raise Exception( "No proper basis chosen" )
    
    if min_h>max_h:
       print( "Error: min_h (" + str(min_h) + ") is bigger than max_h (" + str(max_h) + ")" )

    self.plot_description = ""
    self.plot_metadata    = ""
    
    self.create_data_structures()
    self.create_action_sets()


  def __str__(self):
    result = """
Name:                   """ + self._name + """
Type:                   """ + self.__class__.__name__ + """
Order:                  """ + str( self._order ) + """  
Unknowns:               """ + str( self._unknowns ) + """
Auxiliary variables:    """ + str( self._auxiliary_variables ) + """
h_min:                  """ + str( self._min_h ) + """
h_max:                  """ + str( self._max_h ) + """
In-situ preprocessing:  """ 
    if self._preprocess_reconstructed_patch:
      result += """yes
"""
    else:
      result += """no
"""
    result += "In-situ postprocessing: """ 
    if self._postprocess_updated_patch:
      result += """yes
"""
    else:
      result += """no
"""
    return result


  __repr__ = __str__


  def create_data_structures(self):
    """
    
     By default, we hold all data persistent
     
    """
    self._DG_polynomial                    = peano4.datamodel.Patch( (self._order+1,self._order+1,self._order+1), self._unknowns+self._auxiliary_variables, self._unknown_identifier() )
    
    self._DG_polynomial.generator.store_persistent_condition = self._store_cell_data_default_predicate()
    self._DG_polynomial.generator.load_persistent_condition  = self._load_cell_data_default_predicate()

    self._DG_polynomial.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    
    
    #> note(dominic):
    #> a second snapshot vector is only needed in case we need to perform a rollback to a previous solution state (fused adaptive time stepping).
    #self._DG_polynomial_new                = peano4.datamodel.Patch( (order+1,order+1,order+1),     unknowns+auxiliary_variables, self._unknown_identifier() + "New" )
    self._face_spacetime_solution          = peano4.datamodel.Patch( (2*(self._order+1),self._order+1,self._order+1), self._unknowns+self._auxiliary_variables, self._unknown_identifier() + "SolutionExtrapolation" )
    # > note(dominic): 
    # > Riemann solve is performed on cell now. Hence, Riemann_result is not needed on the face anymore.
    #self._Riemann_result                   = peano4.datamodel.Patch( (1*(order+1),order+1,order+1), unknowns+auxiliary_variables, self._unknown_identifier() + "RiemannSolveResult" )
    # > note(dominic): Fused scheme would need to use some sort of double buffering to prevent data races
    # > One could thus have two buffers on the face that are swapped / copied every iteration.
    # > Alternatively, one could (i) reintroduce the Riemann result and store a ready flag, (ii) or store the space-time predictor/hull-extrapolated predictor on the cell patch
    # > and map it to the face when the neighbour merge is performed.
    # > note(dominic): Enclave tasking needs to store the results on the Cell patch
    #self._face_spacetime_solution_new      = peano4.datamodel.Patch( (2*(order+1),order+1,order+1), unknowns+auxiliary_variables, self._unknown_identifier() + "SolutionExtrapolationNew" ) # replicated 
    #self._DG_polynomial_overlap.generator.merge_method_definition     = peano4.toolbox.blockstructured.get_face_overlap_merge_implementation(self._DG_polynomial_overlap)
    #self._DG_polynomial_overlap_new.generator.merge_method_definition = peano4.toolbox.blockstructured.get_face_overlap_merge_implementation(self._DG_polynomial_overlap)
    
    self._face_spacetime_solution.generator.includes     += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 

#include <algorithm>
"""


    # @todo Sollte false sein
    self._face_spacetime_solution.generator.send_condition               = "true"
    self._face_spacetime_solution.generator.receive_and_merge_condition  = "true"

    self._face_spacetime_solution.generator.merge_method_definition     = self._create_face_merge_code()

      
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

  
  def _create_face_merge_code(self):
    template = jinja2.Template( """
  const int  faceNormal  = marker.getSelectedFaceNumber() % Dimensions;
  const bool isLeftLocal = marker.outerNormal()(faceNormal)>0;  
  
  std::copy_n( 
    neighbour.value + (isLeftLocal ? {{SIZE_OR_POLYNOMIAL}} : 0),
    {{SIZE_OR_POLYNOMIAL}},
    value + (isLeftLocal ? 0: {{SIZE_OR_POLYNOMIAL}})
  );

""")
    d = {
      "SIZE_OR_POLYNOMIAL": 20
    }
    return template.render( **d )
      
      
  def create_action_sets(self):
    self._action_set_adjust_cell     = AdjustCell(self)
    self._action_set_AMR             = AMR(self)
    self._action_set_update_cell     = None
  
  
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
      print( "Cell data" )
      print( "----------" )
      print( str(self._DG_polynomial) )
      print( "Face data" )
      print( "----------" )
      print( str(self._face_spacetime_solution) )
    datamodel.add_cell(self._DG_polynomial)
    datamodel.add_face(self._face_spacetime_solution)
 
 
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    """
      Tell Peano what data to move around
      
      Inform Peano4 step which data are to be moved around via the 
      use_cell and use_face commands. This operation is generic from
      ExaHyPE's point of view, i.e. I use it for all grid sweep types. 
    
    """
    step.use_cell(self._DG_polynomial)
#    step.use_cell(self._DG_polynomial_new)
    step.use_face(self._face_spacetime_solution)

  
  def _get_default_includes(self):
    return """
#include "tarch/la/Vector.h" 

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "repositories/SolverRepository.h"

#include "exahype2/PatchUtils.h"
#include "exahype2/aderdg/RiemannAoS.h"
#include "exahype2/aderdg/RusanovNonlinearAoS.h"
"""


  @abstractmethod
  def get_user_includes(self):
    """
  
    Use this to add include statements to the generated C++ code. Is there for
    subclasses to hook in.
  
    """
    return """
#include "repositories/SolverRepository.h"
"""
  
  
  def add_actions_to_init_grid(self, step):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    step.add_action_set( self._action_set_adjust_cell )

    
  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    step.add_action_set( self._action_set_adjust_cell )
    if evaluate_refinement_criterion:
      step.add_action_set( self._action_set_AMR )
    pass
  
  
  def set_plot_description(self,description,meta_data = ""):
    """
    
    description: String
       Use this one to set a description within the output patch file that tells
       the vis solver what the semantics of the entries are. Typically, I use 
       a comma-separated list here.
    
    meta_data: String
       Arbitrary text. 
    
    """
    self.plot_description = description
    self.plot_metadata    = meta_data
    
  
  def add_actions_to_plot_solution(self, step, output_path):
    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    
    mapping = []    
    for z in self._basis.quadrature_points():
      for y in self._basis.quadrature_points():
        for x in self._basis.quadrature_points():
          mapping.append( (x,y,z) )
    
    step.add_action_set( peano4.toolbox.blockstructured.PlotPatchesInPeanoBlockFormat( 
      filename=output_path + "solution-" + self._name, 
      patch=self._DG_polynomial,     
      dataset_name=self._unknown_identifier(), 
      description=self.plot_description,
      metadata=self.plot_metadata,
      mapping=mapping,
      plot_cell_data=False
    ))

    if self._plot_grid_properties:    
      step.add_action_set( peano4.toolbox.PlotGridInPeanoBlockFormat( output_path + "grid-" + self._name,None ))

    pass
   
 
  def add_actions_to_perform_time_step(self, step):
    """
    :note: ADER-DG does different things in different iterations. Therefore,
           adjust solution operationns must be handled by _action_set_update_cell.
    """

    d = {}
    self._init_dictionary_with_default_parameters(d)
    self.add_entries_to_text_replacement_dictionary(d)
    step.add_action_set( self._action_set_update_cell )
    #step.add_action_set( self._action_set_adjust_cell ) # ADER-DG does different things in different iterations. Integrate directly in update cell instead.
    step.add_action_set( self._action_set_AMR )
    pass


  @abstractmethod
  def add_entries_to_text_replacement_dictionary(self,d):
    pass  

  
  def add_implementation_files_to_project(self,namespace,output):
    """
    
     The ExaHyPE2 project will call this operation when it sets 
     up the overall environment.
     
     
     
     output: peano4.output.Output
      
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
    
    I use my non-critical assertion in the numerical codes. That is, if these
    assertions fail, they do not terminate immediately. Instead, they ask the 
    code to dump the output and to terminate then.
    
    """
    d["SOLVER_INSTANCE"]               = self.get_name_of_global_instance()
    d["SOLVER_NAME"]                   = self._name
    d["UNKNOWN_IDENTIFIER"]            = self._unknown_identifier()
    d["NUMBER_OF_UNKNOWNS"]            = self._unknowns
    d["NUMBER_OF_AUXILIARY_VARIABLES"] = self._auxiliary_variables
    d["ADAPTIVE_TIME_STEPPING"]        = self._adaptive_time_stepping
    d["ADER_PNPM_STABILITY_FACTOR"]    = self._ader_pnpm_stability_factor

    self._basis._init_dictionary_with_default_parameters(d)        
        
    d[ "ASSERTION_WITH_1_ARGUMENTS" ] = "nonCriticalAssertion1"
    d[ "ASSERTION_WITH_2_ARGUMENTS" ] = "nonCriticalAssertion2"
    d[ "ASSERTION_WITH_3_ARGUMENTS" ] = "nonCriticalAssertion3"
    d[ "ASSERTION_WITH_4_ARGUMENTS" ] = "nonCriticalAssertion4"
    d[ "ASSERTION_WITH_5_ARGUMENTS" ] = "nonCriticalAssertion5"
    d[ "ASSERTION_WITH_6_ARGUMENTS" ] = "nonCriticalAssertion6"
    
    d[ "MAX_H"] = self._min_h
    d[ "MIN_H"] = self._max_h
 
