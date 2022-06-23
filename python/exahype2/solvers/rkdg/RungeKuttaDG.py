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

from exahype2.solvers.ButcherTableau              import RungeKutta_steps

from .PDETerms                    import PDETerms
from .kernels                     import get_face_overlap_merge_implementation

import jinja2
import math

from abc import abstractmethod

import exahype2
import exahype2.solvers.rkdg.actionsets

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
    in a regular array (blockstructured data) with (_dg_order+1)^d entries. On top of this 
    current time step, we need the shots into the future. for RK(_rk_order), we need _rk_order
    of these snapshots. They are called estimates. 
    
    On the boundary, I store both the left and right solution projections. We neglect the 
    possibility of local time stepping here - local time stepping is tricky with Runge-Kutta - 
    and therefore only project the current linear combination of estimates for the Runge-Kutta
    step onto the faces. So in the first Runge-Kutta step, it is always the current solution
    which we project onto the face. In the second step, we project a linear combination of the 
    current solution and the outcome of the first trial (Euler step) onto the face, and so 
    forth.
    
    Our DG implementation is not tied to one particular Riemann solver, even though most 
    users will likely use Rusanov. Therefore, it is not clear which data is to be projected
    onto the faces. For plain Rusanov with only a flux function, the solutions are sufficient. 
    Other Riemann solvers or Rusanov's ncp terms however require the derivatives. So I give
    the code the opportunity to work with different data cardinalities.
    
    The actual Riemann problem then is always solved twice: both the left and the right
    adjacent cell solve the Riemann problem and directly add the outcome to their local 
    cell solution. This way, I avoid that we need temporary data structures for the outcome
    of the Riemann solves. This is not an absolutely clean approach, as we should really
    split it up into
    
    - Solve Riemann problem;
    - Project back onto cell;
    - Add reconstructed data onto volume integral.
    
    More details are provided in create_data_structures().
    
    :: Control flow between this class and subclasses
    
    There are three key routines: the constructor, create_data_structures() and 
    create_action_sets(). The constructor sets/memorises some solver variables (such as the 
    name) and then invokes the other two routines.
    
    create_data_structures() establishes all the data structures tied to the 
    grid entities. If you want to alter the configuration of data tied to grid
    entities, you should redefine create_data_structures(). However, any 
    subclass still should call Runge Kutta's create_data_structures() before that. 
    This will ensure that the baseline configuration of all data is in place. 
    After that, you can modify the properties.
    
    create_action_sets() establishes the action sets, i.e. activities that are to 
    be triggered whenever you run a time step, you plot, you initialise the grid.
    Same here: If you want to alter the configuration of the code, call 
    create_action_sets() of the superclass first, before you do fancy stuff. This 
    way, all the important action sets are in place.
    
    :: General control flow and realisation of solver steps
    
    A proper numerical time step is mapped onto a cascade of time step sweeps. Per sweep, 
    we realise the following steps (so step in a computational sense), though their exact
    orchestration depends on the realisation chosen, i.e. some might merge some 
    subcalculations into one grid sweep while others distribute a calculation over multiple
    sweeps.
    
    - We combine the data of the current time step with all the shots into the future. These
      shots are held in one large array. This is not super efficient, as we could build up 
      these arrays one by one, but I just use one large scratchpad even for the first line
      in the Butcher tableau. See create_data_structures() for details. So all the estimates
      and the current solution are combined according to the Butcher tableau.
      
      It is up to the particular subclass to configure the guard of the linear combination 
      such that it is only computed when it is actually needed.
      
    - Next, we project this linear combination onto the faces. At the same time, we compute
      a new estimate for the volume integral. The time step size for this new estimate 
      again depends on the time step size from the Butcher tableau. The two action sets 
      both are volumetric, i.e. run per cell, but we can run them in parallel. The face
      projection has to complete immediately (we need the data on the faces), while the 
      volumetric computation, in principle, doesn't have to finish prior to the next grid
      sweep, as long as the linear combination remains persistent. It is only a temporary
      thing, so will be destroyed immediately once we leave a cell.  
         
         
    :: Mandatory step of subclasses
    
    There are different nuances of subclasses/algorithmic realisations, i.e. few solvers inherit
    directly from RungeKuttaDG. However, even those subclasses still require you do provide some
    additional information.
    
    These are the fields you have to set:
    
    _compute_time_step_size A simple string which defines a double timeStepSize. This snippet will
      be used by both the Riemann solver and the volumetric integration. You don't have to care 
      about the Runge-Kutta scaling of time step sizes (or shifts), as this is all done by the 
      source code, but you have to feed an instruction into the solver how to determine a time step
      size for a cell.
      
    self._compute_new_time_step_size          = "const double newTimeStepSize = timeStepSize;"
             
  """
  
      
  def __init__(self, name, dg_order, rk_order, polynomials, number_of_face_projections, unknowns, auxiliary_variables, min_cell_h, max_cell_h, plot_grid_properties):
    """
    
  Constructor of the Runge-Kutta Discontinuous Galerkin solver
   
  :: Arguments
    
  name: string
     A unique name for the solver. This one will be used for all generated 
     classes. Also the C++ object instance later on will incorporate this 
     name.
     
  dg_order: int
     Order of the Discontinuous Galerkin approximation.
     
  rk_order: int 
     Runge-Kutta order, i.e. time stepping order.
     
  polynomials: Polynomials
     Picks which kind of polynomial to use.
     
  number_of_face_projections: Integer
     How many projections do we need per face. If you are only
     interested in the left and right solution along a face, pass in 1. If you
     want to have the fluxes or derivatives as well, pass in 2. The latter version
     is the type of data structure you need for Rusanov, e.g.
     
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
  
    
  _number_of_derivatives_projected_onto_face: Int
    
  _solver_states: [String] 
      Returns a list of strings of the individual solver states. The code will
      automatically supplement this list with the following states:

        GridConstruction,
        GridInitialisation,
        Plotting,
        PlottingAfterGridInitialisation
      
      So you may implicitly assume that these guys do exist always.
    """
    self._name              = name
    
    self._dg_order          = dg_order
    self._rk_order          = rk_order
    self._polynomials       = polynomials
    self._number_of_face_projections = number_of_face_projections
    
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
    
    self._action_set_postprocess_solution = None   

    self._compute_time_step_size              = None
    self._compute_new_time_step_size          = None

    if polynomials is exahype2.solvers.rkdg.Polynomials.Gauss_Legendre:
      self._basis = GaussLegendreBasis(dg_order+1)
    elif polynomials is exahype2.solvers.rkdg.Polynomials.Gauss_Lobatto:
      self._basis = GaussLobattoBasis(dg_order+1)
    else:
      raise Exception( "No proper basis chosen: {}".format(polynomials) )
    
    self._preprocess_reconstructed_patch      = ""
    self._postprocess_updated_patch           = ""

    self._flux_implementation                 = None
    self._ncp_implementation                  = None
    self._eigenvalues_implementation          = None
    self._source_term_implementation          = None
    self._point_sources_implementation        = None
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

The spacetree will thus span at least """ + str(self.get_coarsest_number_of_cells(domain_size)) + """ cells per coordinate axis. 
The spacetree will thus span at most """ + str(self.get_finest_number_of_cells(domain_size)) + """ cells per coordinate axis. 

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
     
     :: Solver fields built up
     
     _time_step: Patch ( (_dg_order+1)x(_dg_order+1)x(_dg_order+1) )
       Actual polynomial data of the DG patch in the current time step. 

     _estimates: Patch ( (_dg_order+1)x(_dg_order+1)x(_dg_order+1)xon of RK steps )
       These are the Runge-Kutta extrapolations. It is important that the
       rk_order is mixed into the first argument, as the triple is automatically
       truncated to a tuple if the code is translated with 2d.
       
     _linear_combination_of_estimates: Patch ( (_dg_order+1)x(_dg_order+1)x(_dg_order+1) )
       Non-persistent helper data structure.

     _current_time_step_projection: Patch ( 2xnumber_of_face_projectionsx(_dg_order+1)x(_dg_order+1) )
       This is the projection of the polynomial data of the current guess onto the 
       faces. It is important that the first dimension is only a 2. This way, we can
       (logically) handle the projected data again as an overlap of two patches with 
       the width 1.  

     _estimates_projection: Patch ( 2xnumber_of_face_projectionsx(_dg_order+1)x(_dg_order+1) )

     _face_label: FaceLabel
       See class description. General information such as "is boundary". 
     
     _cell_label: CellLabel
       See class description. General information such as "is enclave". 
     
     Per default, I do always store the projections onto faces, and I
     always keep the actual time step data and the cell projections. It is 
     up to the subclasses to alter this storage behaviour.
     
     By default, I do not exchange any face data in-between two grid sweeps.
     You will have to overwrite the behaviour in subclasses. Please note that 
     a face is communicated between two tree/domain partitions if and only if
     it is stored persistently and the exchange flag is set, too. 
         
       
    """
    self._current_time_step                 = peano4.datamodel.Patch( (self._dg_order+1,self._dg_order+1,self._dg_order+1),                      self._unknowns+self._auxiliary_variables, self._unknown_identifier() )
    self._estimates                         = peano4.datamodel.Patch( (self.number_of_Runge_Kutta_steps()*(self._dg_order+1),self._dg_order+1,self._dg_order+1),     self._unknowns+self._auxiliary_variables, self._unknown_identifier() + "Estimates" )
    self._linear_combination_of_estimates   = peano4.datamodel.Patch( (self._dg_order+1,self._dg_order+1,self._dg_order+1),                      self._unknowns+self._auxiliary_variables, self._unknown_identifier() + "LinearCombination")
    self._previous_estimate_projection      = peano4.datamodel.Patch( (2,self._number_of_face_projections*(self._dg_order+1),self._dg_order+1),  self._unknowns+self._auxiliary_variables, self._unknown_identifier() + "EstimateProjection" )
    self._Riemann_solution                  = peano4.datamodel.Patch( (2,self._dg_order+1,self._dg_order+1),  self._unknowns, self._unknown_identifier() + "RiemannSolution" )
           
    self._previous_estimate_projection.generator.merge_method_definition = get_face_overlap_merge_implementation(self._previous_estimate_projection)
    self._previous_estimate_projection.generator.includes     += """
#include "peano4/utils/Loop.h"
#include "repositories/SolverRepository.h" 
"""

    self._current_time_step.generator.store_persistent_condition = self._store_cell_data_default_guard()
    self._current_time_step.generator.load_persistent_condition  = self._load_cell_data_default_guard()
    
    self._previous_estimate_projection.generator.store_persistent_condition = self._store_cell_data_default_guard()
    self._previous_estimate_projection.generator.load_persistent_condition  = self._load_cell_data_default_guard()

    self._linear_combination_of_estimates.store_persistent_condition = "false"
    self._linear_combination_of_estimates.load_persistent_condition = "false"

    self._previous_estimate_projection.generator.store_persistent_condition = self._store_face_data_default_guard()
    self._previous_estimate_projection.generator.load_persistent_condition  = self._load_face_data_default_guard()

    self._previous_estimate_projection.generator.send_condition               = "false"
    self._previous_estimate_projection.generator.receive_and_merge_condition  = "false"

    self._Riemann_solution.generator.store_persistent_condition = self._store_face_data_default_guard()
    self._Riemann_solution.generator.load_persistent_condition  = self._load_face_data_default_guard()

    self._Riemann_solution.generator.send_condition               = "false"
    self._Riemann_solution.generator.receive_and_merge_condition  = "false"



    self._current_time_step.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    
    self._estimates.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    
    self._previous_estimate_projection.generator.includes  += """
#include "../repositories/SolverRepository.h"
"""    
    self._Riemann_solution.generator.includes  += """
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
     
     This operation can be called multiple times over the construction of
     a solver. However, only the very last call matters. All previous calls 
     are wiped out.
     
     If you have a hierarchy of solvers, every create_data_structure()
     should first(!) call its parent version. This way, you always ensure
     that all data are in place before you continue to alter the more
     specialised versions. So it is (logically) a top-down (general to
     specialised) run through all create_data_structure() variants 
     within the inheritance tree.
     
    """
    self._action_set_initial_conditions                                               = exahype2.solvers.rkdg.actionsets.InitialCondition(self, self._store_cell_data_default_guard(), "true" )
    self._action_set_initial_conditions_for_grid_construction                         = exahype2.solvers.rkdg.actionsets.InitialCondition(self, self._store_cell_data_default_guard(), "false")
    self._action_set_AMR                                                              = exahype2.solvers.rkdg.actionsets.AdaptivityCriterion(solver=self, 
                                                                                                                                             guard=self._store_cell_data_default_guard() + " and (repositories::" + self.get_name_of_global_instance() + ".isLastGridSweepOfTimeStep() or repositories::" + self.get_name_of_global_instance() + ".getSolverState()==" + self._name + "::SolverState::GridInitialisation)", 
                                                                                                                                             build_up_new_refinement_instructions=True,  implement_previous_refinement_instructions=True )
    self._action_set_AMR_commit_without_further_analysis                              = exahype2.solvers.rkdg.actionsets.AdaptivityCriterion(solver=self, guard=self._store_cell_data_default_guard(), build_up_new_refinement_instructions=False, implement_previous_refinement_instructions=True )
    self._action_set_linear_combination_of_estimates                                  = exahype2.solvers.rkdg.actionsets.LinearCombinationOfEstimates(self)
    # @todo TW The following two action sets should run in parallel I guess 
    self._action_set_project_linear_combination_onto_faces                            = exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces(solver=self)
    self._action_set_solve_volume_integral                                            = exahype2.solvers.rkdg.actionsets.SolveVolumeIntegral(self)
    self._action_set_handle_boundary                                                  = exahype2.solvers.rkdg.actionsets.HandleBoundary(self, self._store_face_data_default_guard())
    self._action_set_solve_Riemann_problem                                            = exahype2.solvers.rkdg.actionsets.SolveRiemannProblem(self)
    self._action_set_add_volume_and_face_solution                                     = exahype2.solvers.rkdg.actionsets.AddVolumeAndFaceSolution(self)
    self._action_set_compute_final_linear_combination_and_project_solution_onto_faces = exahype2.solvers.rkdg.actionsets.ComputeFinalLinearCombinationAndProjectSolutionOntoFaces(solver=self,guard=self._store_cell_data_default_guard())
    self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement = exahype2.solvers.rkdg.actionsets.DynamicAMR(solver=self,guard=self._store_cell_data_default_guard())
    
    if self._action_set_postprocess_solution==None:
      self._action_set_postprocess_solution                                             = exahype2.solvers.rkdg.actionsets.EmptyPostprocessSolution(solver=self)

    self._action_set_update_face_label = exahype2.grid.UpdateFaceLabel( self._name )  
    self._action_set_update_cell_label = exahype2.grid.UpdateCellLabel( self._name ) 
        

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
#      print( str(self._patch) )
      print( "Patch overlap data" )
      print( "----------" )
#      print( str(self._patch_overlap_old) )
      print( "Patch overlap data" )
      print( "----------" )
#      print( str(self._patch_overlap_new) )
    datamodel.add_cell(self._cell_label)
    datamodel.add_cell(self._current_time_step)
    datamodel.add_cell(self._estimates)
    datamodel.add_cell(self._linear_combination_of_estimates)
    datamodel.add_face(self._face_label)
    datamodel.add_face(self._previous_estimate_projection)
    datamodel.add_face(self._Riemann_solution)

 
  def add_use_data_statements_to_Peano4_solver_step(self, step):
    """
      Tell Peano what data to move around
      
      Inform Peano4 step which data are to be moved around via the 
      use_cell and use_face commands. This operation is generic from
      ExaHyPE's point of view, i.e. I use it for all grid sweep types. 
    
    """
    step.use_cell(self._cell_label)
    step.use_cell(self._current_time_step)
    step.use_cell(self._estimates)
    step.use_cell(self._linear_combination_of_estimates)
    step.use_face(self._face_label)
    step.use_face(self._previous_estimate_projection)
    step.use_face(self._Riemann_solution)
    
  
  def _get_default_includes(self):
    return """
#include "tarch/la/Vector.h" 

#include "peano4/utils/Globals.h"
#include "peano4/utils/Loop.h"

#include "repositories/SolverRepository.h"
"""


  def add_actions_to_init_grid(self, step):
    """

    @todo
        
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
    step.add_action_set( self._action_set_initial_conditions ) 
    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )
    step.add_action_set( self._action_set_AMR )
    step.add_action_set( self._action_set_postprocess_solution )
    
    
  def add_actions_to_create_grid(self, step, evaluate_refinement_criterion):
    """
    
    @todo: 
    
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
    
     @todo
     
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

    #step.add_action_set( self._action_set_couple_resolution_transitions_and_handle_dynamic_mesh_refinement )
    #step.add_action_set( self._action_set_roll_over_update_of_faces )
    step.add_action_set( self._action_set_update_face_label )
    step.add_action_set( self._action_set_update_cell_label )
    #step.add_action_set( self._action_set_project_patch_onto_faces )


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
    """
 
    @todo: 
       
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
    step.add_action_set( self._action_set_linear_combination_of_estimates )
    step.add_action_set( self._action_set_project_linear_combination_onto_faces )
    step.add_action_set( self._action_set_solve_volume_integral )
    step.add_action_set( self._action_set_handle_boundary )
    step.add_action_set( self._action_set_solve_Riemann_problem )
    step.add_action_set( self._action_set_add_volume_and_face_solution )
    step.add_action_set( self._action_set_compute_final_linear_combination_and_project_solution_onto_faces )
    step.add_action_set( self._action_set_AMR )
    step.add_action_set( self._action_set_postprocess_solution )


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
    d["DG_ORDER"]                       = self._dg_order
    d["RK_ORDER"]                       = self._rk_order
    d["RK_STEPS"]                       = self.number_of_Runge_Kutta_steps()

    d["SOLVER_INSTANCE"]                = self.get_name_of_global_instance()
    d["SOLVER_NAME"]                    = self._name
    d["UNKNOWN_IDENTIFIER"]             = self._unknown_identifier()
    d["NUMBER_OF_UNKNOWNS"]             = self._unknowns
    d["NUMBER_OF_AUXILIARY_VARIABLES"]  = self._auxiliary_variables

    d[ "NUMBER_OF_DOFS_PER_CELL_2D" ] = (self._dg_order+1) * (self._dg_order+1);
    d[ "NUMBER_OF_DOFS_PER_CELL_3D" ] = (self._dg_order+1) * (self._dg_order+1) * (self._dg_order+1);
    
    d[ "NUMBER_OF_DOFS_PER_FACE_2D" ] = (self._dg_order+1)
    d[ "NUMBER_OF_DOFS_PER_FACE_3D" ] = (self._dg_order+1) * (self._dg_order+1)

    d["POLYNOMIALS"]                    = str(self._polynomials)

    d[ "ASSERTION_WITH_1_ARGUMENTS" ] = "nonCriticalAssertion1"
    d[ "ASSERTION_WITH_2_ARGUMENTS" ] = "nonCriticalAssertion2"
    d[ "ASSERTION_WITH_3_ARGUMENTS" ] = "nonCriticalAssertion3"
    d[ "ASSERTION_WITH_4_ARGUMENTS" ] = "nonCriticalAssertion4"
    d[ "ASSERTION_WITH_5_ARGUMENTS" ] = "nonCriticalAssertion5"
    d[ "ASSERTION_WITH_6_ARGUMENTS" ] = "nonCriticalAssertion6"

    if (self._min_cell_h > self._max_cell_h ):
      raise Exception( "min/max h are inconsistent" )
    d[ "MAX_CELL_H"] = self._max_cell_h
    d[ "MIN_CELL_H"] = self._min_cell_h

    d[ "SOLVER_CONSTANTS" ] = self.solver_constants_

    d[ "SOLVER_INCLUDES" ]  = self.get_user_solver_includes()
    
    #d[ "SOURCE_TERM_CALL"]                    = jinja2.Template(self._source_term_call, undefined=jinja2.DebugUndefined).render( **d )
    #d[ "RIEMANN_SOLVER_CALL"]                 = jinja2.Template(self._Riemann_solver_call, undefined=jinja2.DebugUndefined).render( **d )
    #d[ "PREPROCESS_RECONSTRUCTED_PATCH" ]     = jinja2.Template(self._preprocess_reconstructed_patch, undefined=jinja2.DebugUndefined).render( **d )
    #d[ "POSTPROCESS_UPDATED_PATCH" ]          = jinja2.Template(self._postprocess_updated_patch, undefined=jinja2.DebugUndefined).render( **d )
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

    d[ "FLUX_IMPLEMENTATION"]                 = self._flux_implementation
    d[ "NCP_IMPLEMENTATION"]                  = self._ncp_implementation
    d[ "SOURCE_TERM_IMPLEMENTATION"]          = self._source_term_implementation
    d[ "POINT_SOURCES_IMPLEMENTATION"]        = self._point_sources_implementation
    self._basis.init_dictionary_with_default_parameters(d)        

  
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


  def number_of_Runge_Kutta_steps(self):
    return RungeKutta_steps(self._rk_order)
    

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


