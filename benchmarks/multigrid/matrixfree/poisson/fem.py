import os, sys
import argparse
import peano4
import numpy as np

import multigrid.matrixfree

print( """

Peano 4 multigrid solver using Discontinuous Galerkin

@author 2022 Sean Baccas, Dmitry Nikolaenko, Tobias Weinzierl

""")

parser = argparse.ArgumentParser(description='Multigrid - Poisson solver')
parser.add_argument("-j",   "--parallel-builds",    dest="j",              type=int, default=-1, help="Parallel builds" )
parser.add_argument("-pd",  "--peano-dir",          dest="peanodir",       default="../../../../", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",            dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument("-d",   "--dimensions",         dest="dimensions",     type=int, default=2, help="Dimensions")
parser.add_argument("-upv", "--unknowns_per_vertex",dest="upv",            type=int, default=1, help="Unknowns per vertex")
parser.add_argument("-meshsize",  "--meshsize",     dest="meshsize",       default=0.3, help="Mesh size")
parser.add_argument("-m",   "--mode",               dest="mode",           choices=["release","stats","asserts", "trace"], required=True, help="Pick build type" )
args = parser.parse_args()

project = multigrid.matrixfree.Project( project_name = "CollocatedFEM", 
                                        namespace = [ "benchmarks", "multigrid", "matrixfree", "poisson" ]
                                         )

local_assembly_matrix = np.eye( 2**args.dimensions )

solver = multigrid.matrixfree.solvers.CollocatedLowOrderDiscretisation("CollocatedPoisson",
                                                                       args.upv,
                                                                       args.dimensions,
                                                                       args.meshsize,
                                                                       args.meshsize,
                                                                       local_assembly_matrix,
                                                                       local_assembly_matrix_scaling=0
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
)

project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( args.peanodir, build_mode )

peano4_project = project.generate_Peano4_project(args.verbose)
peano4_project.generate()
