""" 
 This file is part of the MGHyPE project within Peano 4. For conditions of distribution and 
 use, please see the copyright notice at www.peano-framework.org
"""
import peano4
import peano4.output.Jinja2TemplatedHeaderImplementationFilePair
import jinja2
import dastgen2
import os

from .DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver import DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver

class DiscontinuousGalerkinWithoutFusion(DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver):
  """!

  Inherit everything.

  The only difference here is the templates - we redefine the guard functions in the abstract solver
  to make sure we only do one actionset per sweep. So for that, we need a new set of template files
  which is no issue.

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
              integration_points,
              solver_tolerance,
              custom_init_actionset = None,
              custom_plot_actionset = None, # just pass in a list of tuples, one for the variable name and the other for the getter
              custom_init_cell = "",
              enable_plotting = True, # sometimes we want to disable standard plotting...,
              plot_predicate = "true", # enable custom plotting behaviour
              solver_includes = ""
              ):
    """!
    
      
    """  
    super( DiscontinuousGalerkinWithoutFusion, self ).__init__(
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
      custom_plot_actionset, # just pass in a list of tuples, one for the variable name and the other for the getter
      custom_init_cell,
      enable_plotting, 
      plot_predicate,  
      solver_includes, 
    )

    # we also need to make sure that the merge method has the solver repository header available  
    self._face_data.peano4_mpi_and_storage_aspect.implementation_file_includes += """\n#include "repositories/SolverRepository.h"\n"""

  def get_face_merge_method(self):
    """!
    We only want to merge face projections when we are about to
    update the solution on faces.
    """
    d = {}
    d["SOLVER_INSTANCE"] = self.instance_name()

    output = """
  if (!marker.willBeRefined()
    and
    context == ::peano4::grid::TraversalObserver::SendReceiveContext::BoundaryExchange
    and
    repositories::{{SOLVER_INSTANCE}}.updateFace()
  ) { //todo change this to use the face's own enum

    setLProjection(
      getLProjection() + neighbour.getLProjection()
    );
    setRProjection(
      getRProjection() + neighbour.getRProjection()
    );
  }
    """
  
    return jinja2.Template(output).render(**d)
