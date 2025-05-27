import os, sys
import argparse
import peano4
import mghype
import numpy as np

import commonsteps


args = commonsteps.get_args()


executable = "peano4matrixfree"

project = mghype.matrixfree.api.Project( project_name = "DGTwoGrid", 
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
  solver_tolerance = 1.0e-14,
  integration_points = 0.5*(matrices.get_nodal_points_1d() + 1.0) # Should be specified in [0,1] interval
)


#! [Construct correction matrices]
correction_equation_matrices = mghype.api.matrixgenerators.DLinearMassIdentity(
  dimensions = args.dimensions,
  poly_degree = 1,
  unknowns_per_node = 1
)
system_matrix, system_matrix_scaling = correction_equation_matrices.get_cell_system_matrix_for_laplacian()
mass_matrix,   mass_matrix_scaling   = correction_equation_matrices.get_cell_mass_matrix()
#! [Construct correction matrices]


#! [Correction equation solver]
correction_equation_solver = mghype.matrixfree.solvers.api.CollocatedDLinearDiscretisationWithPointJacobi(
  name = "CollocatedPoisson",
  unknowns_per_vertex = 1, 
  dimensions = args.dimensions,
  min_h = args.meshsize,
  max_h = args.meshsize,
  local_assembly_matrix = system_matrix,
  local_assembly_matrix_scaling=system_matrix_scaling,
  mass_matrix = mass_matrix,
  mass_matrix_scaling = mass_matrix_scaling,
  solver_tolerance              = 1e-12,
  smoother_relaxation           = 0.5
)
#! [Correction equation solver]


#! [Coupling]
intergrid_operators = mghype.api.matrixgenerators.blockmatrix.IntergridOperators(
  dim = args.dimensions,
  coarse_order = 1,
  fine_order = args.poly_degree
  )
prolongation_matrix = [intergrid_operators.prolongation()]
prolongation_matrix_scaling = [0]
restriction_matrix = [intergrid_operators.restriction()]
restriction_matrix_scaling = [0]
injection_matrix = [intergrid_operators.injection()]
injection_matrix_scaling = [0]

solver.preprocessing_action_set = mghype.matrixfree.solvers.api.actionsets.MultiplicativeDGCGCoupling(
    solver,                      # dg_solver
    correction_equation_solver,  # cg_solver
    prolongation_matrix,
    prolongation_matrix_scaling,
    restriction_matrix,
    restriction_matrix_scaling,
    injection_matrix,
    injection_matrix_scaling,
    args.fas
)
#! [Coupling]


#! [Add solvers]
project.add_solver(correction_equation_solver, auxiliary=True)
project.add_solver(solver)
#! [Add solvers]

commonsteps.configure_and_build_project(args,project)
commonsteps.run_and_validate(args,executable)
