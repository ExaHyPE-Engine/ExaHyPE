import os, sys
import argparse
import peano4
import mghype
import numpy as np

import commonsteps


args = commonsteps.get_args()


executable = "peano4matrixfree"


project = mghype.matrixfree.api.Project( project_name = "CollocatedFEM", 
                              namespace = [ "tests", "multigrid", "matrixfree", "poisson" ],
                              executable = executable
                                )

#! [Construct matrices]
poisson_equation_matrices = mghype.api.matrixgenerators.DLinear(
  dimensions = args.dimensions,
  unknowns_per_node = 1
)

system_matrix, system_matrix_scaling = poisson_equation_matrices.get_cell_system_matrix_for_laplacian()
mass_matrix,   mass_matrix_scaling   = poisson_equation_matrices.get_cell_mass_matrix()
#! [Construct matrices]

solver = mghype.matrixfree.solvers.api.CollocatedDLinearDiscretisationWithPointJacobi("CollocatedPoisson",
                                                              1, # unknowns
                                                              args.dimensions,
                                                              args.meshsize,
                                                              args.meshsize,
                                                              local_assembly_matrix = system_matrix,
                                                              local_assembly_matrix_scaling=system_matrix_scaling,
                                                              mass_matrix = mass_matrix,
                                                              mass_matrix_scaling = mass_matrix_scaling,
                                                              solver_tolerance=0.01
                                                            )

project.add_solver(solver)

commonsteps.configure_and_build_project(args,project)
commonsteps.run_and_validate(args,executable)
