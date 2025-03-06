# This file is part of the ExaHyPE2 project. For conditions of distribution and 
# use, please see the copyright notice at www.peano-framework.org
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2
import os

from .Solver                                                              import Solver
from .actionsets.PlotDGDataInPeanoBlockFormat                             import PlotDGDataInPeanoBlockFormat

from .DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver import DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver
from .actionsets.DGTaskingSolver.AssignTaskNumbersToCells                 import AssignTaskNumbersToCells
from .actionsets.DGTaskingSolver.AssignTaskNumbersToCells                 import AssignTaskNumbersToCells
from .actionsets.DGTaskingSolver.ProjectIntoCellAndUpdateCellSolution     import ProjectIntoCellAndUpdateCellSolution
from .actionsets.DGTaskingSolver.UpdateFaceSolution                       import UpdateFaceSolution

class DGTaskingSolver(DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver):
  """!
  We reintroduce the tasking solver here.

  All we need to do is inherit from the normal DG version, and
  add a few things of our own

  See the documentation for solvers.api.DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver.DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver

  """
  def __init__(self,
              name,
              dimensions,
              poly_degree,
              unknowns_per_cell_node,
              solutions_per_face_node,
              projections_per_face_node,
              min_h,
              max_h,
              assembly_matrix,
              assembly_matrix_scaling,
              mass_matrix,
              mass_matrix_scaling,
              face_from_cell_projection,
              face_from_cell_projection_scaling,
              cell_from_face_projection,
              cell_from_face_projection_scaling,
              riemann_matrix,
              boundary_matrix,
              cell_relaxation,
              face_relaxation,
              approximate_system_matrix,
              approximate_system_matrix_scaling,
              solver_tolerance,
              custom_init_actionset = None,
              custom_plot_actionset = None, # just pass in a list of tuples, one for the variable name and the other for the getter
              custom_init_cell = "",
              custom_init_face = "",
              plot_predicate = "true" # enable custom plotting behaviour
              ):
    super( DGTaskingSolver, self ).__init__(
      name,
      dimensions,
      poly_degree,
      unknowns_per_cell_node,
      solutions_per_face_node,
      projections_per_face_node,
      min_h,
      max_h,
      assembly_matrix,
      assembly_matrix_scaling,
      mass_matrix,
      mass_matrix_scaling,
      face_from_cell_projection,
      face_from_cell_projection_scaling,
      cell_from_face_projection,
      cell_from_face_projection_scaling,
      riemann_matrix,
      boundary_matrix,
      cell_relaxation,
      face_relaxation,
      approximate_system_matrix,
      approximate_system_matrix_scaling,
      solver_tolerance,
      custom_init_actionset = None,
      custom_plot_actionset = None, # just pass in a list of tuples, one for the variable name and the other for the getter
      custom_init_cell = "",
      custom_init_face = "",
      plot_predicate = "true" # enable custom plotting behaviour
    )

  def add_to_init_solution(self, observer):
    observer.add_action_set( AssignTaskNumbersToCells(self, tasks_per_cell=2) )
    super( DGTaskingSolver, self ).add_to_init_solution(observer)

  # OVERWRITE THIS ONE ENTIRELY
  def add_to_solve(self, observer):
    descend_order = observer.highest_descend_invocation_order() + self._basic_descend_order
    if self.preprocessing_action_set != None:
      self.preprocessing_action_set.descend_invocation_order = descend_order + 0
      observer.add_action_set( self.preprocessing_action_set )

    observer.add_action_set( UpdateFaceSolution(self, descend_order+1, False) )
    observer.add_action_set( ProjectIntoCellAndUpdateCellSolution(self, descend_order+2, False) )
