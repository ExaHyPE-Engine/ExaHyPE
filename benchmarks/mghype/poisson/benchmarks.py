import os, sys
import argparse
import peano4
import mghype
import numpy as np

print( """

Peano 4 multigrid solver using Discontinuous Galerkin

""")


parser = argparse.ArgumentParser(description='Multigrid - Poisson solver')
parser.add_argument("-j",   "--parallel-builds",      dest="j",              type=int, default=-1, help="Parallel builds" )
parser.add_argument("-pd",  "--peano-dir",            dest="peanodir",       default="../../..", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",              dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument("-d",   "--dimensions",           dest="dimensions",     type=int, default=2, help="Dimensions")
parser.add_argument("-p",   "--poly_degree",          dest="poly_degree",    type=int, required=True, help="Polynomial Degree")
parser.add_argument("-m",   "--meshsize",             dest="meshsize",       required=True, help="Mesh size")
parser.add_argument("-var", "--variant",              dest="variant",        choices=["dg-sweeps", "dg-shifted-and-fused", "tasks"], required=True, help="Pick variant" )
parser.add_argument("-o",   "--output",               dest="output",         required=True, help="Name of executable")
args = parser.parse_args()

project = mghype.matrixfree.api.Project( project_name = "DGPointwise", 
                                         namespace = [ "benchmarks", "mghype", "poisson" ],
                                         executable = args.output
                                        )

matrices = mghype.api.matrixgenerators.GLMatrixFree(
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


Solver_Type = None
if args.variant == "dg-sweeps":
    Solver_Type = mghype.matrixfree.solvers.api.DiscontinuousGalerkinWithFusionUnshifted
elif args.variant == "dg-shifted-and-fused":
    Solver_Type = mghype.matrixfree.solvers.api.DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver
elif args.variant == "tasks":
    Solver_Type = mghype.matrixfree.solvers.api.DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver

solver = Solver_Type(
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
  cell_relaxation = 0.5,
  face_relaxation = 1.0,
  approximate_system_matrix = approximate_system_matrix,
  approximate_system_matrix_scaling = approximate_system_matrix_scaling,
  solver_tolerance = 0.01,
  integration_points = [ float(i) / (args.poly_degree) for i in range(args.poly_degree+1) ],
  enable_plotting    = False,
  solver_includes    = """#include "{}/tests/multigrid/matrixfree/poisson/Scenario.h" """.format(args.peanodir)
)

project.add_solver(solver)

#
# Configure build
#
build_mode = peano4.output.CompileMode.Release
#build_mode = peano4.output.CompileMode.Asserts


cube_size = 1.0
project.set_global_simulation_parameters(
  dimensions            = args.dimensions,
  offset                = [ 0.0       for _ in range(args.dimensions) ],
  domain_size           = [ cube_size for _ in range(args.dimensions) ],
  plot_after_each_mesh_sweep       = False,
  max_iterations                   = 5
)

project.set_load_balancing( "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates", "new ::exahype2::LoadBalancingConfiguration(0.9, 0, false)" )
project.set_Peano4_installation( args.peanodir, build_mode )

peano4_project = project.generate_Peano4_project(args.verbose)
peano4_project.output.makefile.add_cpp_file( args.peanodir + "/tests/multigrid/matrixfree/poisson/Scenario.cpp" )
peano4_project.generate()

os.system( "make clean" )
if args.j>0:
  os.system( "make -j{}".format(args.j) )
else:
  os.system( "make -j$(nproc --all)" )
  
