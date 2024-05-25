import os, sys
import argparse
import peano4
import numpy as np
import mghype

print( """

Peano 4 multigrid solver using Continuous Galerkin

@author 2022 Sean Baccas, Dmitry Nikolaenko, Tobias Weinzierl

""")

parser = argparse.ArgumentParser(description='Multigrid - Poisson solver')
parser.add_argument("-j",   "--parallel-builds",    dest="j",              type=int, default=-1, help="Parallel builds" )
parser.add_argument("-pt",   "--plot-timestep",     dest="plot_each_timestep", action="store_true", default=False, help="Plot after each timestep. By default (False) we plot after initialisation and at the end" )
parser.add_argument("-pd",  "--peano-dir",          dest="peanodir",       default="../../../../", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",            dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument("-d",   "--dimensions",         dest="dimensions",     type=int, default=2, help="Dimensions")
parser.add_argument("-meshsize",  "--meshsize",     dest="meshsize",       default=0.3, help="Mesh size")
parser.add_argument("-m",   "--mode",               dest="mode",           choices=["release","stats","asserts", "trace"], required=True, help="Pick build type" )
args = parser.parse_args()

project = mghype.matrixfree.api.Project( project_name = "CollocatedFEM", 
                              namespace = [ "tests", "multigrid", "matrixfree", "poisson" ]
                                )

#! [Construct matrices]
poisson_equation_matrices = mghype.api.matrixgenerators.DLinear(
  dimensions = args.dimensions,
  poly_degree = 1,
  unknowns_per_vertex_dof = 1
)

system_matrix, system_matrix_scaling = poisson_equation_matrices.get_cell_system_matrix_for_laplacian()
mass_matrix,   maxx_matrix_scaling   = poisson_equation_matrices.get_cell_mass_matrix()
#! [Construct matrices]

solver = mghype.matrixfree.solvers.api.CollocatedLowOrderDiscretisation("CollocatedPoisson",
                                                              1, # unknowns
                                                              args.dimensions,
                                                              args.meshsize,
                                                              args.meshsize,
                                                              local_assembly_matrix = system_matrix,
                                                              local_assembly_matrix_scaling=system_matrix_scaling,
                                                              mass_matrix = mass_matrix,
                                                              mass_matrix_scaling = maxx_matrix_scaling,
                                                              solver_tolerance=0.01
                                                            )

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

os.system( "make clean" )
if args.j>0:
  os.system( "make -j{}".format(args.j) )
else:
  os.system( "make -j" )
  
print("Done")

