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
parser.add_argument("-pt",   "--plot-timestep",     dest="plot_each_timestep", action="store_true", default=False, help="Plot after each timestep. By default (False) we plot after initialisation and at the end" )
parser.add_argument("-pd",  "--peano-dir",            dest="peanodir",       default="../../../../", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",              dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument("-d",   "--dimensions",           dest="dimensions",     type=int, default=2, help="Dimensions")
parser.add_argument("-p",   "--poly_degree",          dest="poly_degree",    type=int, default=1, help="Polynomial Degree")
parser.add_argument("-meshsize",  "--meshsize",       dest="meshsize",       default=0.3, help="Mesh size")
parser.add_argument("-omega_c",  "--omega_c",       dest="omega_c",       default=0.5, help="Mesh size")
parser.add_argument("-omega_f",  "--omega_f",       dest="omega_f",       default=1.0, help="Mesh size")
parser.add_argument("-m",   "--mode",                 dest="mode",           choices=["release","stats","asserts", "trace"], required=True, help="Pick build type" )
args = parser.parse_args()

#! [Create project]
project = mghype.matrixfree.api.Project( project_name = "DGPointwise", 
                              namespace = [ "tests", "multigrid", "matrixfree", "poisson" ]
                            )
#! [Create project]


#! [Construct matrices]

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
#! [Construct matrices]



#! [Instantiate solver]
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
)

#! [Instantiate solver]

project.add_solver(solver)

#
# Configure build
#
if args.mode=="release":
  build_mode = peano4.output.CompileMode.Release
if args.mode=="stats":
  build_mode = peano4.output.CompileMode.Stats
if args.mode=="asserts":
  build_mode = peano4.output.CompileMode.Asserts
if args.mode=="trace":
  build_mode = peano4.output.CompileMode.Trace


cube_size = 1.0
project.set_global_simulation_parameters(
  dimensions            = args.dimensions,
  offset                = [ 0.0       for _ in range(args.dimensions) ],
  domain_size           = [ cube_size for _ in range(args.dimensions) ],
  plot_each_timestep    = args.plot_each_timestep,
)

project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( args.peanodir, build_mode )

peano4_project = project.generate_Peano4_project(args.verbose)
peano4_project.output.makefile.add_cpp_file( "Scenario.cpp" )
peano4_project.generate()

os.system( "mkdir visualisation" )

os.system( "make clean" )
if args.j>0:
  os.system( "make -j{}".format(args.j) )
else:
  os.system( "make -j" )
  
