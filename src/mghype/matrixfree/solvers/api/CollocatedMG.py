import dastgen2.attributes
import peano4
import jinja2
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2
import os

from .Solver                        import Solver
from .actionsets.CollocatedMGSolver.InitDofs               import InitDofsCollocatedMG
from .actionsets.CollocatedMGSolver.ResetAndUpdateResidual import ResetAndUpdateResidual
from .actionsets.CollocatedMGSolver.UpdateSolution         import UpdateSolution
from .actionsets.CollocatedMGSolver.Restriction            import Restriction
from .actionsets.CollocatedMGSolver.Prolongation           import Prolongation
from .actionsets.PlotVertexDataInPeanoBlockFormat          import PlotVertexDataInPeanoBlockFormat

from abc import abstractmethod
from peano4.solversteps.ActionSet import ActionSet

class CollocatedMG(Solver):
  """!

  A full mathematical description will follow later, but for now we outline some algorithmic steps here.

  We borrow heavily from the original Collocated Solver. See \ref matrixfree_collocated_solver.

  We intialise by setting the values for u and the RHS in implementation files. So far this has been tested
  with the standard poisson set up ( \f$ 8\pi^2 sin(x)sin(y) \f$ on the rhs, with zeroes  in the initial guess).
  We do this only on the finest level, and every other level receives 0.

  We enumerate each level according to its coarseness. Ie the top level is marked as 0, and we increase this number
  as we refine.

  ## Algorithmic Blueprint

  1. On the finest level:
    - Solve with Jacobi for a pre-determined number of pre-smoothing steps. Use standard mass matrix
    - On the final sweep, we take the final residual and write this into the rhs of the vertex and one level above. We set its initial guess to 0.
    - We then move up one level coarser

  2. On the next level up:
    - Solve with Jacobi for a pre-determined number of pre-smoothing steps. Use identity instead of mass matrix
    - On the final sweep, we take the final residual and write this into the rhs of the vertex and one level above. We set its initial guess to 0.
    - We then move up one level coarser

  3. We continue until we are on the coarsest level. We then solve with Jacobi for the predetermined number of pre-smoothing steps.

  4. We then solve on the coarsest level with the predetermined number of post-smoothing steps.
    - After this, we move one level finer

  5. On the next level down:
    - When we touch a vertex for the first time, we add prolongation of the solution from the level above to our solution.
    - We then solve with the predetermined number of post-smoothing steps
    - After this, we move one level finer
  
  6. We continue until we reach the finest level, at which point we add prolongation from the level above, and run for predetermined number of post-smoothing steps. We then go back to step 1.

  Every time we run a sweep on the finest level, we update the global residuals. We terminate when the solution on the
  finest level conforms to the given stopping criterion.

  ## Implementation

  We have 4 fundamental action sets, which we discuss here in order of invocation. Each of their operations is guarded by a bool function that lives in the abstract solver class.

  ### UpdateSolution
  
  \see mghype.matrixfree.solvers.api.actionsets.CollocatedMGSolver.UpdateSolution

  This is where we take the residual and diag, and set the solution on vertices to \f$ u + du = u + \omega / (diag * residual) \f$. We want to avoid 
  zero-divsion, as reset the diagonal every sweep, meaning we need to wait until the second pre/post-smoothing sweep to update the solution. We also
  need to make sure we finalise the solution updates before we restrict/prolongate as well. updateSolution() in the abstract solver class handles all
  of this behaviour.

  To make sure the solver works in multiple trees, we tell the vertices that lie on parallel domain boundaries to share residual and diagonal with
  each other, to make sure contributions from neighbouring cells are registered properly. See get_vertex_merge_method in this class.

  This action set also makes sure that the RHS is correctly consolidated - we write our restrictions into the vertex field NewRhs. When we touch
  a vertex for the first time after restriction, we make sure that the rhs is set to the value of what we restricted (ie the newrhs).

  Ditto for delta after prolongation.

  ### Prolongation
  \see mghype.matrixfree.solvers.api.actionsets.CollocatedMGSolver.Prolongation

  When we touch a vertex on the coarse level, we set the delta field to be \f$ U - OldU \f$. When we touch a vertex on the fine level, we prolongate
  the coarse delta into the fine level delta. On the next sweep, we update the fine solution to be \f$ u + delta \f$.

  The abstract solver just needs to check if its smoothing state is set to prolongation, so the logic is much simpler.

  ### ResetAndUpdateResidual
  
  \see mghype.matrixfree.solvers.api.actionsets.CollocatedMGSolver.ResetAndUpdateResidual

  When we touch the vertex for the first time, we reset residual and diag to 0, or apply boundary conditions.
  When we touch the cell for the first time, we perform the jacobi iteration and write the residual into the \f$ 2^D \f$ adjacent vertices.

  The abstract solver class does this if updateResidual() returns true, which it does when:
    - The refinement level matches the active level
    - The smoothing state is presmoothing or postsmoothing

  ### Restriction

  \see mghype.matrixfree.solvers.api.actionsets.CollocatedMGSolver.Restriction

  When we touch a coarse vertex for the first time, we reset everything to 0, i.e. rhs and solution.
  When we touch a fine vertex for the first time, we hand over to the mghype::matrixfree::solvers::cgmultigrid::restrictToNextLevel
  which writes the fine level residual into the newRhs of the level above.

  ## Infrastructure

  You can now specify the number of levels that you want to use in the solver itself. The finest level is set by the refinement of
  the mesh and is currently determined at runtime when the mesh is descended for the first time. If we find that the finest level
  isn't at least one larger than the coarest level, we throw a std::runtime_error.

  """
  def __init__(self,
               name,
               unknowns_per_vertex,
               dimensions,
               min_h,
               max_h,
               local_assembly_matrix,
               local_assembly_matrix_scaling,
               mass_matrix,
               mass_matrix_scaling,
               correction_matrix,
               correction_matrix_scaling,
               solver_tolerance,
               pre_smoothing_steps,
               post_smoothing_steps,
               descend_order,
               restriction_matrix,
               prolongation_matrix,
               coarsest_level=0,
               smoother_relaxation = 0.6,
               plot_predicate="true"
               ):
    super( CollocatedMG, self ).__init__( name,
                                          min_h=min_h,
                                          max_h=max_h
                                          )
    
    assert pre_smoothing_steps  >= 1
    assert post_smoothing_steps >= 1
    assert coarsest_level >= 0

    self._pre_smoothing_steps  = pre_smoothing_steps
    self._post_smoothing_steps = post_smoothing_steps

    self._restriction_matrix  = restriction_matrix
    self._prolongation_matrix = prolongation_matrix

    self._coarsest_level      = coarsest_level

    self._vertex_data.data.add_attribute( dastgen2.attributes.Integer( name="Level", initval=-1 ) )
    self._cell_data.data.add_attribute  ( dastgen2.attributes.Integer( name="Level", initval=-1 ) )

    self._unknowns_per_vertex_node = unknowns_per_vertex

    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "u",          str(unknowns_per_vertex) ) )
    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "oldU",       str(unknowns_per_vertex) ) )
    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "delta",      str(unknowns_per_vertex) ) )
    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "rhs",        str(unknowns_per_vertex) ) )
    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "diag",       str(unknowns_per_vertex) ) )
    self._vertex_data.data.add_attribute( peano4.dastgen2.Peano4DoubleArray( "residual",   str(unknowns_per_vertex) ) )

    self._vertex_data.peano4_mpi_and_storage_aspect.merge_implementation = self.get_vertex_merge_method()
    self._vertex_data.peano4_mpi_and_storage_aspect.implementation_file_includes += """\n#include "repositories/SolverRepository.h"\n"""

    self._local_assembly_matrix         = local_assembly_matrix
    self._local_assembly_matrix_scaling = local_assembly_matrix_scaling

    self._mass_matrix                   = mass_matrix
    self._mass_matrix_scaling           = mass_matrix_scaling
    self._correction_matrix             = correction_matrix
    self._correction_matrix_scaling     = correction_matrix_scaling
    self._solver_tolerance              = solver_tolerance
    
    self._basic_descend_order           = descend_order
    
    self._smoother_relaxation           = smoother_relaxation
    self._plot_predicate                = plot_predicate

  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    super( CollocatedMG, self ).add_to_Peano4_datamodel(datamodel,verbose)

  def add_use_statements(self, observer):
    super( CollocatedMG, self ).add_use_statements(observer)

  def add_to_create_grid(self, observer):
    observer.add_action_set(peano4.toolbox.CreateRegularGrid(self.max_h))

  def add_to_init_solution(self, observer):
    """!
    
    Solution initialisation
    
    """
    observer.add_action_set( InitDofsCollocatedMG(self) ) 
    pass

  def add_to_solve(self, observer):
    invocation_order = observer.highest_descend_invocation_order() + self._basic_descend_order + 1
    observer.add_action_set( UpdateSolution(self,descend_invocation_order=invocation_order+0) )
    observer.add_action_set( Prolongation(self,descend_invocation_order=invocation_order+1) )
    observer.add_action_set( ResetAndUpdateResidual(self,descend_invocation_order=invocation_order+2) )
    observer.add_action_set( Restriction(self,descend_invocation_order=invocation_order+3) )
    pass
  
  def add_to_plot(self, observer):
    cell_predicate = f"""fineGridCell{self.typename()}.getLevel() == repositories::{self.instance_name()}.getFinestLevel() """
    observer.add_action_set(PlotVertexDataInPeanoBlockFormat(self, "u",   "getU",   plot_predicate=self._plot_predicate, cell_predicate=cell_predicate))
    observer.add_action_set(PlotVertexDataInPeanoBlockFormat(self, "rhs", "getRhs", plot_predicate=self._plot_predicate, cell_predicate=cell_predicate))
    pass

  def add_implementation_files_to_project(self, namespace, output, subdirectory=""):
    """
    
    The solver creates two classes: An abstract base class and its 
    implementations. The abstract base class will be overwritten if
    there is one in the directory. I pipe all the Python constants 
    into this base class, so they are available in the C++ code. 
    
    """
    templatefile_prefix  = os.path.dirname( os.path.realpath(__file__) ) + "/templates/"
    templatefile_prefix += self.__class__.__name__
    
    if(subdirectory):
        subdirectory += "/"

    dictionary = {
        "SOLVER_INCLUDES": "",
        "MIN_H": self.min_h,
        "MAX_H": self.max_h,
        "LOCAL_ASSEMBLY_MATRIX": self._local_assembly_matrix,
        "LOCAL_ASSEMBLY_MATRIX_SCALING": self._local_assembly_matrix_scaling,
        "MASS_MATRIX"         : self._mass_matrix,
        "MASS_MATRIX_SCALING" : self._mass_matrix_scaling,
        "VERTEX_CARDINALITY":            self._unknowns_per_vertex_node,
        "SOLVER_NAME":                   self.typename(),
        "SOLVER_TOLERANCE"            :  self._solver_tolerance,
        "SMOOTHER_RELAXATION":           self._smoother_relaxation,
        "PRE_SMOOTHING_STEPS":           self._pre_smoothing_steps,
        "POST_SMOOTHING_STEPS":          self._post_smoothing_steps,
        "RESTRICTION_MATRIX":            self._restriction_matrix,
        "PROLONGATION_MATRIX":           self._prolongation_matrix,
        "CORRECTION_MATRIX":             self._correction_matrix,
        "CORRECTION_MATRIX_SCALING":     self._correction_matrix_scaling,
        "COARSEST_LEVEL"           :     self._coarsest_level
    }

    generated_abstract_header_file = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".Abstract.template.h",
      templatefile_prefix + ".Abstract.template.cpp",
      "Abstract" + self.typename(), 
      namespace,
      subdirectory + ".", 
      dictionary,
      True,
      True)
    generated_solver_files = peano4.output.Jinja2TemplatedHeaderImplementationFilePair(
      templatefile_prefix + ".template.h",
      templatefile_prefix + ".template.cpp",
      self.typename(), 
      namespace,
      subdirectory + ".", 
      dictionary,
      False,
      True)

    output.add( generated_abstract_header_file )
    output.add( generated_solver_files )
    output.makefile.add_h_file( subdirectory + "Abstract" + self._name + ".h", generated=True )
    output.makefile.add_h_file( subdirectory + self._name + ".h", generated=True )
    output.makefile.add_cpp_file( subdirectory + "Abstract" + self._name + ".cpp", generated=True )
    output.makefile.add_cpp_file( subdirectory + self._name + ".cpp", generated=True )

  def get_vertex_merge_method(self):
    d = {}
    d["SOLVER_INSTANCE"] = self.instance_name()

    output = """

  // we can get away with only doing these on the active level
  if (
    repositories::{{SOLVER_INSTANCE}}.consolidateResidualAndDiag( getLevel() )
  ) {
    // naive += with the diag
    setDiag( getDiag() + neighbour.getDiag() );

    // ditto for residual
    setResidual( getResidual() + neighbour.getResidual() );
  }

    """

    return jinja2.Template(output).render(**d)
