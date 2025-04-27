import os, sys
import argparse
import peano4
import mghype
import numpy as np

import commonsteps


args = commonsteps.get_args()


executable = "peano4matrixfree"
    
project = mghype.matrixfree.api.Project( project_name = "DGPointwise", 
                              namespace = [ "tests", "multigrid", "matrixfree", "poisson" ],
                              executable = executable
                            )


matrices = mghype.api.matrixgenerators.GaussLobattoMatrixFree(
  args.dimensions,
  args.poly_degree,
  1,  # Unknowns per cell dof. Scalar PDE here
  2,  # We use the penalty formulation (see docu in tutorials)
  2
)


assembly_matrix, assembly_matrix_scaling = matrices.get_cell_system_matrix_for_laplacian()
mass_matrix, mass_matrix_scaling         = matrices.get_cell_mass_matrix()
face_from_cell_projection, \
face_from_cell_projection_scaling        = matrices.get_face_from_cell_matrix()
cell_from_face_projection, \
cell_from_face_projection_scaling        = matrices.get_cell_from_face_matrix()
approximate_system_matrix, \
approximate_system_matrix_scaling        = matrices.get_A_tilde()


solver = mghype.matrixfree.solvers.api.DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver(
  name = "DGPoisson",
  dimensions = args.dimensions,
  poly_degree = args.poly_degree,
  unknowns_per_cell_node = 1,
  solutions_per_face_node = 2,
  projections_per_face_node = 2,
  min_h = args.meshsize,
  max_h = args.meshsize,
  assembly_matrix= assembly_matrix, 
  assembly_matrix_scaling = assembly_matrix_scaling, 
  mass_matrix = mass_matrix, 
  mass_matrix_scaling = mass_matrix_scaling, 
  face_from_cell_projection = face_from_cell_projection, 
  face_from_cell_projection_scaling = face_from_cell_projection_scaling,
  cell_from_face_projection = cell_from_face_projection, 
  cell_from_face_projection_scaling = cell_from_face_projection_scaling,
  riemann_matrix = matrices.get_face_face_riemann_problem_matrix(),
  boundary_matrix = matrices.get_boundary_matrix(),
  cell_relaxation = args.omega_c,
  face_relaxation = args.omega_f,  # default here is 1, i.e. we solve exactly. See class docu
  approximate_system_matrix = approximate_system_matrix,
  approximate_system_matrix_scaling = approximate_system_matrix_scaling,
  solver_tolerance = 0.01,
  integration_points = 0.5*(matrices.get_nodal_points_1d() + 1.0) # Should be specified in [0,1] interval
)


project.add_solver(solver)

commonsteps.configure_and_build_project(args,project)
commonsteps.run_and_validate(args,executable)
