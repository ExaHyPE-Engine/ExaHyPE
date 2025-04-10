""" 
 This file is part of the MGHyPE project within Peano 4. For conditions of distribution and 
 use, please see the copyright notice at www.peano-framework.org
"""
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import dastgen2
import os
import jinja2

from .Solver import Solver
from .DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver import DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver

from .actionsets.DGSolver.UpdateResidualWithTasks                        import UpdateResidualWithTasks
from .actionsets.DGSolver.ProjectIntoCellAndUpdateCellSolutionWithTasks  import ProjectIntoCellAndUpdateCellSolutionWithTasks


import peano4


class DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver_Tasks(DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver):
  """!
     
  DG solver adding tasks to baseline code
  

  See the documentation for solvers.api.DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver.DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver
  first. The class adds a few simple features:
  
  1. We create a cell marker which holds the task numbers. 
  2. This one is added to the mesh within add_to_Peano4_datamodel().
  3. We tell both the mesh initialisation and each solve step to use the data.
  4. We swap out the residual computation and the cell update with a bespoke
     task-based implementation
    
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
              integration_points,
              solver_tolerance,
              custom_init_actionset = None,
              custom_plot_actionset = None, # just pass in a list of tuples, one for the variable name and the other for the getter
              custom_init_cell = "",
              enable_plotting = True, # sometimes we want to disable standard plotting...,
              plot_predicate = "true", # enable custom plotting behaviour
              solver_includes = "",
              ):
    super( DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver_Tasks, self ).__init__(
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
      integration_points,
      solver_tolerance,
      custom_init_actionset,
      custom_plot_actionset,
      custom_init_cell,
      enable_plotting,
      plot_predicate,
      solver_includes,
      use_smart_pointer = True,
    )

    self._task_name = self._name + "DGTaskMarker"
    self._cell_marker_for_tasks = peano4.toolbox.api.EnumerateCellsAndVertices.create_cell_marker(self._task_name)

    # Stays the same
    #    self._action_set_update_face_solution                        =  UpdateFaceSolution(self, descend_order+1, False) 
    #    self._action_set_project_onto_faces                          =  ProjectOntoFaces(self, descend_order+3, False) 
    self._action_set_project_into_cell_and_update_cell_solution  =  ProjectIntoCellAndUpdateCellSolutionWithTasks(self, self._action_set_project_into_cell_and_update_cell_solution.descend_invocation_order, False) 
    self._action_set_update_residual                             =  UpdateResidualWithTasks(self, self._action_set_update_residual.descend_invocation_order, False) 

    
  def add_to_init_solution(self, observer):
    """!
        
    See superclass' constructor
    
    We ask the superclass to add all of its action sets to the project's
    observer. After that, we add our own action set to the algorithm 
    step.
    
    """
    super( DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver_Tasks, self ).add_to_init_solution( observer )
    observer.add_action_set( peano4.toolbox.api.AssignNumbersToCells(self._task_name) )

    
  def add_to_solve(self, observer):
    super( DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver_Tasks, self ).add_to_solve( observer )
    observer.add_action_set( peano4.toolbox.api.AssignNumbersToCells(self._task_name) )

  
  def add_to_Peano4_datamodel( self, datamodel, verbose ):
    super( DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver_Tasks, self ).add_to_Peano4_datamodel( datamodel, verbose )
    datamodel.add_cell(self._cell_marker_for_tasks)

    
  def add_use_statements(self, observer):
    super( DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver_Tasks, self ).add_use_statements( observer )
    observer.use_cell(  self._cell_marker_for_tasks)
    
