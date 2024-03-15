# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org


import os, sys
import argparse
import peano4
import multigrid

print( """

Peano 4 multigrid solver using Discontinuous Galerkin and PETSc

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


project = multigrid.petsc.api.Project(project_name = "CollocatedFEM", 
                                  namespace = [ "benchmarks", "multigrid", "petsc", "poisson" ]
                                  ) 

                        
matrices = multigrid.api.matrixgenerators.DLinear(dimensions              = args.dimensions,
                                                  unknowns_per_vertex_dof = args.upv,
                                                  poly_degree             = 1
                                              )

solver = multigrid.petsc.api.solvers.CollocatedLowOrderDiscretisation(name                 = "CollocatedPoisson",
                                                                  unknowns             = args.upv,
                                                                  dimensions           = args.dimensions,
                                                                  min_h                = args.meshsize,
                                                                  max_h                = args.meshsize,
                                                                  cell_lhs_matrix      = matrices.get_cell_system_matrix_for_laplacian(),
                                                                  cell_rhs_matrix      = matrices.get_cell_identity_matrix(),
                                                                  cell_lhs_matrix_scaling = -2,
                                                                  cell_rhs_matrix_scaling = args.dimensions,
                                                                  )

project.add_solver( solver )
                        
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


project.set_load_balancing( "toolbox::loadbalancing::strategies::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
project.set_Peano4_installation( args.peanodir, build_mode )
    
    
#
# Generate code and then copy cpp file over main
#
peano4_project = project.generate_Peano4_project(args.verbose)
peano4_project.generate()

os.system( "make clean" )
if args.j>0:
  os.system( "make -j{}".format(args.j) )
else:
  os.system( "make -j" )
  
print("Done")
