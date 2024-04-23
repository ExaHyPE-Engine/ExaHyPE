# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import os

import peano4
import peano4.datamodel
import peano4.output.TemplatedHeaderFile
import peano4.output.TemplatedHeaderImplementationFilePair
import peano4.output.Jinja2TemplatedHeaderFile
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair

import peano4.toolbox.multigrid.cellbased.ScalarJacobiWithRediscretisation

import jinja2

from abc import abstractmethod

import exahype2

import dastgen2
import peano4.datamodel.DaStGen2


class ConstrainedPoissonEquationForMarkerOnCells(object):
  """ 
    Use Finite Differences Poisson equation system solver to guide cell labels

    The most popular application of this solver is to guide limiters: 
    Some codes need to know where to solve their PDE with an alternative 
    more robust (though likely less effective) solver. For this, they
    solve
    
    @f$ - \Delta u = f @f$
    
    for a fixed right-hand side f with a Jacobi solver. The solution is 
    subject to the constraints 

    @f$ u_{\text{min}} \leq u \leq u_{\text{max}} @f$

    and Dirichlet boundary conditions. 
    
    Typically, the solvers than set the value of $u=1$ in regions where
    they need the limiter. The $u$ value will fade away from these regions,
    and we can implement the following heuristics with 
    @f$ u_{\text{min}} < U_{\text{min}} < U_{\text{max}} < u_{\text{max}}@f$:
    
    - @f$ u<U_{\text{min}} @f$: Solver A is active. Solver B is not active.
    - @f$ U_{\text{min}} \leq u < \frac{1}{2} ( U_{\text{min}} + U_{\text{max}} )@f$: Solver 
      A is active and sets the value of solver B which is not active but its 
      solution exists here.
    - @f$ \frac{1}{2} ( U_{\text{min}} + U_{\text{max}} ) \leq u \leq U_{\text{max}} @f$: Solver 
      B is active and sets the value of solver A which is not active but its 
      solution exists here.
    - @f$ U_{\text{max}} < u @f$: Solver B is active. Solver A is not active.
    

    
    As we use a Jacobi iterative solver, using this marker within ExaHyPE is 
    equivalent to a heat equation solver which becomes stationary eventually if
    the areas where @f$ u=u_{\text{max}} @f$ is manually set/enforced do not 
    change.
    
    The solver works with a constant right-hand side and is defined over two
    attributes per cell: There's the actual value (u in the example above)
    and a marker which switches the Jacobi smoothing on and off per cell.
    In cells where we fix the value, we toggle the marker so Jacobi updates
    are disabled.

    
    ## Usage
        
    
    To use the marker within your project, add a new solver to your project:
    
         marker_solver = exahype2.solvers.elliptic.ConstrainedPoissonEquationOnCells( name="ConstrainedPoissonEquationOnCells" )
         project.add_solver( marker_solver )


  """
  
      
  def __init__(self, 
               name, 
               relaxation_coefficient = 0.6, 
               right_hand_side = -25.0, 
               min_value = 0.0, 
               max_value = 1.0, 
               min_threshold = 0.4,
               max_threshold = 0.6,
               plot = False):
    """
    
  name: string
     A unique name for the solver. This one will be used for all generated 
     classes.
  
  plot: Boolean
     Clarifies whether a dump of the data should be enriched with grid info
     (such as enclave status flags), too.
     
  relaxation_coefficient: Double (0< value <1)
     How quickly should the solution be relaxed against real solution. This
     is the damping parameter in the Jacobi update scheme and thus has to be
     bigger than 0 and smaller than 1. If the coefficient approaches 1, you 
     might see oscillations.
     
  right_hand_side: Double (negative)
     Basically describes how fast the solution is pulled down to 0 once we
     go away from the AMR transition area.
 
    """
    assert min_value<max_value, "arguments invalid"
    assert max_threshold<=max_value, "arguments invalid"
    assert min_threshold>=min_value, "arguments invalid"
    assert min_threshold<max_threshold, "arguments invalid"
    
    self._name                   = name
    self._relaxation_coefficient = relaxation_coefficient
    self._right_hand_side        = right_hand_side
    self._min_value              = min_value
    self._max_value              = max_value
    self._min_threshold          = min_threshold
    self._max_threshold          = max_threshold
    self._plot                   = plot

    self._postprocess_updated_cell = None
    
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
    
    Create cell and face data structures required. For the face data,
    we rely on the multigrid toolbox routines.
       
    """
    Variants = ["AnotB","AdeterminesB","BdeterminesA","BnotA"]
    
    self._cell_data_model = peano4.datamodel.DaStGen2( self._unknown_identifier() )
    self._cell_data_model.data.add_attribute(dastgen2.attributes.Boolean("Invariant") )
    self._cell_data_model.data.add_attribute(dastgen2.attributes.Double("Value") )
    self._cell_data_model.data.add_attribute(dastgen2.attributes.Enumeration( name="Marker",      variants=Variants ) )
        
    self._face_data_model = peano4.toolbox.multigrid.cellbased.ScalarJacobiWithRediscretisation.construct_face_helper_data(self._cell_data_model)
    
    
  @abstractmethod
  def create_action_sets(self):
    """
    
     Overwrite in subclasses if you wanna create different
     action sets.
     
     ## Call order and ownership
     
     This operation can be called multiple times. However, only the very
     last call matters. All previous calls are wiped out.
     
     If you have a hierarchy of solvers, every create_data_structure()
     should first(!) call its parent version. This way, you always ensure
     that all data are in place before you continue to alter the more
     specialised versions. So it is (logically) a top-down (general to
     specialised) run through all create_data_structure() variants 
     within the inheritance tree.
     
     ## Postprocessing algorithm
     
     After we have updated the cell, i.e. computed the Poisson equation update,
     we launch a series of postprocessing steps:
     
     - We impose the value constraints, i.e. ensure the solution is within min
       and max.
       
     
    """
    self._action_set_smoother = peano4.toolbox.multigrid.cellbased.ScalarJacobiWithRediscretisation( 
      cell_data               = self._cell_data_model, 
      face_data               = self._face_data_model,
      relaxation_coefficient  = self._relaxation_coefficient,
      rhs_expr                = self._right_hand_side,
      unknown_setter          = "setValue",
      unknown_getter          = "getValue",
      guard                   = "repositories::isLastGridSweepOfTimeStep() and not fineGridCellAlphaPoissonMarker.getInvariant()"
      )
    
    self._action_set_smoother.additional_includes = self._get_default_includes()
    self._action_set_smoother.d[ "SOLVER_INSTANCE" ] = self.get_name_of_global_instance()
    
    if self._postprocess_updated_cell!=None:
      self._action_set_smoother._Template_TouchCellFirstTime_UpdateSolution += self._postprocess_updated_cell

    self._action_set_smoother._Template_TouchCellFirstTime_UpdateSolution += """
    repositories::{{SOLVER_INSTANCE}}.computeNewMarkerState( 
      fineGridCell{{CELL_NAME}},
      fineGridFaces{{FACE_NAME}} 
    );
        
    repositories::{{SOLVER_INSTANCE}}.constrainValue( fineGridCell{{CELL_NAME}} );
    """

  
  def _unknown_identifier(self):
    return self._name +"PoissonMarker"
  

  def get_name_of_global_instance(self):
    return "instanceOf" + self._name

  
  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    """
    
      Add all required data to the Peano4 project's datamodel 
      so it is properly built up
      
    """
    if verbose:
      print( "Constrained Poisson Equation" )
      print( "----------" )
      print( str(self._name) )
    datamodel.add_cell(self._cell_data_model)
    datamodel.add_face(self._face_data_model)

 
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    """
      Tell Peano what data to move around
      
      Inform Peano4 step which data are to be moved around via the 
      use_cell and use_face commands. This operation is generic from
      ExaHyPE's point of view, i.e. I use it for all grid sweep types. 
    
    """
    step.use_cell(self._cell_data_model)
    step.use_face(self._face_data_model)

  
  def _get_default_includes(self):
    return """
#include "tarch/la/Vector.h" 

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "repositories/SolverRepository.h"

#include "Constants.h"
"""


  def add_actions_to_init_grid(self, step):
    """
    
    
    """
    step.add_action_set( self._action_set_smoother )
    pass

    
  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    """
    
     It is important to plug into the creation, as this is the place where
     we get the creational events.
     
    """
    step.add_action_set( self._action_set_smoother )
    pass

  
  def add_actions_to_plot_solution(self, step, output_path):
    """
    
     We plot if and only if we are asked to do so
     
    """
    step.add_action_set( self._action_set_smoother )
    if self._plot:
      plotter_action_set = peano4.toolbox.PlotCellDataInPeanoBlockFormat( 
        filename     = output_path + "marker-" + self._name, 
        cell_unknown = self._cell_data_model,
        getter       = "defined below",
        number_of_unknows_per_cell = 3,
        description  = "value,marker(AnotB,AdeterminesB,BdeterminesA,BnotA),invariant",
        time_stamp_evaluation="0.5*(repositories::getMinTimeStamp()+repositories::getMaxTimeStamp())",
        additional_includes="""
#include "repositories/SolverRepository.h"       
"""        
      )
      plotter_action_set.Template_TouchCellFirstTime = """
  int cellIndex = _writer->plotPatch(
    marker.x()-0.5 * marker.h(),
    marker.h()
  );

  double data[] = {
                        fineGridCell{{CELL_UNKNOWN_NAME}}.getValue(),
    static_cast<double>(fineGridCell{{CELL_UNKNOWN_NAME}}.getMarker()),
    static_cast<double>(fineGridCell{{CELL_UNKNOWN_NAME}}.getInvariant()),
  };

  _dataWriter->plotCell( cellIndex, data );
"""      
      step.add_action_set( plotter_action_set )
      
   
 
  def add_actions_to_perform_time_step(self, step):
    """
    
    AMR
    
    It is important that we do the inter-grid transfer operators before we 
    apply the boundary conditions.
    
    """
    step.add_action_set( self._action_set_smoother )
    pass


  @abstractmethod
  def add_entries_to_text_replacement_dictionary(self,d):
    pass


  def add_implementation_files_to_project(self,namespace,output, subdirectory=""):
    """
    
     The ExaHyPE2 project will call this operation when it sets
     up the overall environment.
     
     This routine is typically not invoked by a user.

     output: peano4.output.Output
     
    """
    templatefile_prefix = os.path.dirname( os.path.realpath(__file__) ) + "/ConstrainedPoissonEquationForMarkerOnCells"

    if(subdirectory):
        subdirectory += "/"

    implementationDictionary = {}
    self.add_entries_to_text_replacement_dictionary(implementationDictionary)

    implementationDictionary[ "RELAXATION_COEFFICIENT" ] = self._relaxation_coefficient
    implementationDictionary[ "RHS" ]                    = self._right_hand_side
    implementationDictionary[ "MIN_VALUE" ]              = self._min_value
    implementationDictionary[ "MAX_VALUE" ]              = self._max_value
    implementationDictionary[ "MIN_THRESHOLD" ]          = self._min_threshold
    implementationDictionary[ "MAX_THRESHOLD" ]          = self._max_threshold

    implementationDictionary[ "MARKER" ]                 = self._cell_data_model.name
    
    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".template.h",
      templatefile_prefix + ".template.cpp",
      self._name, 
      namespace,
      subdirectory + ".", 
      implementationDictionary,
      True,
      True)

    output.add( generated_solver_files )
    output.makefile.add_cpp_file( subdirectory + self._name + ".cpp", generated=True )


  @property
  def name(self):
    return self._name


  @property
  def postprocess_updated_cell(self):
    return self._postprocess_updated_cell


  @postprocess_updated_cell.setter
  def postprocess_updated_cell(self, kernel):
    """

    Define a postprocessing routine over the data
    
    Typically, you do something similar to
    
         if ( not repositories::{{SOLVER_INSTANCE}}.patchCanUseStatelessPDETerms(marker.x(), marker.h(), timeStamp, timeStepSize) ) {
           fineGridCell{{CELL_NAME}}.setValue(1.0);
         }

    We evaluate some criterion (here, we only look at a global function, but we 
    also have access to all the other solver data in the kernel) and then set
    the cell value to the maximum if the criterion holds. 
    

    ## Attributes

    kernel: String
      C++ code that holds the postprocessing kernel

    """
    self._postprocess_updated_cell = kernel
    self.create_data_structures()
    self.create_action_sets()
