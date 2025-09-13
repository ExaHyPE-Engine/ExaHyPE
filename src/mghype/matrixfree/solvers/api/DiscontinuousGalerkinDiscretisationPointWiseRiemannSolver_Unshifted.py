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

from .actionsets.InitHigherOrderDofs                            import InitHigherOrderDofs
from .actionsets.DGSolver.UpdateResidual                        import UpdateResidual
from .actionsets.DGSolver.ProjectOntoFaces                      import ProjectOntoFaces
from .actionsets.DGSolver.ProjectIntoCellAndUpdateCellSolution  import ProjectIntoCellAndUpdateCellSolution
from .actionsets.PlotDGDataInPeanoBlockFormat                   import PlotDGDataInPeanoBlockFormat


class DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver_Unshifted(DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver):
  """!
     
  Simple solver which maps the DG steps onto separate mesh traversals
  
  This Python class is a mere wrapper without any funtionality. The interesting
  stuff happens in the template file that is used for this variant, i.e.
  DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver_Unshifted.Abstract.template.h
  and
  DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver_Unshifted.Abstract.template.cpp.
  These files introduce a marker toggling mechnanism, where a marker always 
  runs through the three steps

  - ProjectOntoFaces
  - ComputeNumericalFlux
  - UpdateCell
  
  and hence ensures that we actually traverse the mesh three times before we 
  actually have a new solution.
    
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
              use_smart_pointer = True
              ):
    super( DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver_Unshifted, self ).__init__(
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
      use_smart_pointer,
    )

