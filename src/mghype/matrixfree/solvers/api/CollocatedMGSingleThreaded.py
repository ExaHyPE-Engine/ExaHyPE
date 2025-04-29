import dastgen2.attributes
import peano4
import jinja2
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2
import os

from .Solver                                               import Solver
from .CollocatedMG                                         import CollocatedMG
from .actionsets.CollocatedMGSolver.InitDofs               import InitDofsCollocatedMG
from .actionsets.CollocatedMGSolver.ResetAndUpdateResidual import ResetAndUpdateResidual
from .actionsets.CollocatedMGSolver.Restriction            import Restriction
from .actionsets.CollocatedMGSolver.Prolongation           import Prolongation
from .actionsets.PlotVertexDataInPeanoBlockFormat          import PlotVertexDataInPeanoBlockFormat

from .actionsets.CollocatedMGSingleThreaded.UpdateSolution         import UpdateSolution

from abc import abstractmethod
from peano4.solversteps.ActionSet import ActionSet

class CollocatedMGSingleThreaded(CollocatedMG):
  """!
  We can just inherit everything, except we must rewrite the
  add_to_solve method.

  The important difference here is that we update the residual and the solution all in the same
  sweep, meaning that we don't need any complicated logic in the abstract solver class.

  See the documentation of solvers.api.CollocatedMG.CollocatedMG

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
    super( CollocatedMGSingleThreaded, self ).__init__( 
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
      coarsest_level,
      smoother_relaxation,
      plot_predicate
    )
    
  def add_to_solve(self, observer):
    invocation_order = observer.highest_descend_invocation_order() + self._basic_descend_order + 1
    observer.add_action_set( Prolongation(self,descend_invocation_order=invocation_order+0) )
    observer.add_action_set( ResetAndUpdateResidual(self,descend_invocation_order=invocation_order+1) )
    observer.add_action_set( UpdateSolution(self,descend_invocation_order=invocation_order+2) )
    observer.add_action_set( Restriction(self,descend_invocation_order=invocation_order+3) )
  
  def get_vertex_merge_method(self):
    return ""
