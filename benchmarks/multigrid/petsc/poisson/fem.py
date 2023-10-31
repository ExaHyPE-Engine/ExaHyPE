# This file is part of the Peano multigrid project. For conditions of 
# distribution and use, please see the copyright notice at www.peano-framework.org


import os, sys
import argparse
import peano4
import multigrid.petsc
import multigrid.matrixgenerators

print( """

Peano 4 multigrid solver using Discontinuous Galerkin and PETSc

@author 2022 Sean Baccas, Dmitry Nikolaenko, Tobias Weinzierl

""")

parser = argparse.ArgumentParser(description='Multigrid - Poisson solver')
parser.add_argument("-j",   "--parallel-builds",    dest="j",              type=int, default=-1, help="Parallel builds" )
parser.add_argument("-pd",  "--peano-dir",          dest="peanodir",       default="../../../../", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",            dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument("-d",   "--dimensions",         dest="dimensions",     type=int, default=2, help="Dimensions")
parser.add_argument("-meshsize",  "--meshsize",     dest="meshsize",       default=0.1, help="Mesh size")
parser.add_argument("-m",   "--mode",               dest="mode",           choices=["release","stats","asserts"], required=True, help="Pick build type" )
args = parser.parse_args()

assert args.dimensions == 2, "not ready for 3d yet"

project = multigrid.petsc.Project(project_name = "CollocatedFEM", 
                                  namespace = [ "benchmarks", "multigrid", "petsc", "poisson" ]
                                  ) 

                        
matrices = multigrid.matrixgenerators.DLinear(dimensions              = args.dimensions,
                                              unknowns_per_vertex_dof = 1,
                                              )
                        
solver = multigrid.petsc.solvers.CollocatedLowOrderDiscretisation(name                 = "CollocatedPoisson",
                                                                  unknowns             = 1,                      # unknowns_per_cell_dof,
                                                                  dimensions           = args.dimensions,
                                                                  min_h                = args.meshsize,
                                                                  max_h                = args.meshsize,
                                                                  cell_lhs_matrix      = matrices.getCellSystemMatrixForLaplacian(),
                                                                  cell_rhs_matrix      = matrices.getCellIdentityMatrix(),
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
