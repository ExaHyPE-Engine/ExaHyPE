import os, sys
import argparse
import peano4
import numpy as np

import src.multigrid.matrixfree as matrixfree

print( """

Peano 4 multigrid solver using Discontinuous Galerkin

@author 2022 Sean Baccas, Dmitry Nikolaenko, Tobias Weinzierl

""")


parser = argparse.ArgumentParser(description='Multigrid - Poisson solver')
parser.add_argument("-j",   "--parallel-builds",    dest="j",              type=int, default=-1, help="Parallel builds" )
parser.add_argument("-pd",  "--peano-dir",          dest="peanodir",       default="../../../../", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",            dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument("-d",   "--dimensions",         dest="dimensions",     type=int, default=2, help="Dimensions")
parser.add_argument("-p",   "--poly_degree",        dest="poly_degree",    type=int, default=1, help="Polynomial Degree")
parser.add_argument("-upcn", "--unknowns_per_cell", dest="upcn",            type=int, default=1, help="Unknowns per cell node")
parser.add_argument("-upfn", "--unknowns_per_face", dest="upfn",            type=int, default=1, help="Unknowns per face node")
parser.add_argument("-meshsize",  "--meshsize",     dest="meshsize",       default=0.3, help="Mesh size")
parser.add_argument("-m",   "--mode",               dest="mode",           choices=["release","stats","asserts", "trace"], required=True, help="Pick build type" )
args = parser.parse_args()

project = matrixfree.Project( project_name = "DGPointwise", 
                              namespace = [ "benchmarks", "multigrid", "matrixfree", "poisson" ]
                            )

# matrices
assembly_matrix = np.eye( args.upcn * (args.poly_degree + 1) ** (args.dimensions) )
mass_matrix     = np.eye( args.upcn * (args.poly_degree + 1) ** (args.dimensions) )
face_from_cell  = np.zeros(( 2* args.upcn * (args.poly_degree + 1) ** (args.dimensions-1) * 2*args.dimensions,   args.upcn * (args.poly_degree + 1) ** (args.dimensions) ))

# we are projecting the face solution onto the cell here.
cell_from_face  = np.zeros(( args.upcn * (args.poly_degree + 1) ** (args.dimensions),     args.upcn * (args.poly_degree + 1) ** (args.dimensions-1) * 2*args.dimensions ) )

riemann_matrix  = np.zeros( (args.upcn * (args.poly_degree + 1) ** (args.dimensions-1), 2* args.upcn * (args.poly_degree + 1) ** (args.dimensions-1)) )


solver = matrixfree.solvers.api.DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver(
  "DGPoisson",
  args.dimensions,
  args.poly_degree,
  args.upcn,
  args.upfn,
  args.meshsize,
  args.meshsize,
  assembly_matrix, # assembly matrix
  0, # assembly matrix scaling
  mass_matrix, # mass matrix
  0, # mass matrix scaling
  face_from_cell, # cell to face
  cell_from_face, # face to cell
  riemann_matrix
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

os.system( "make clean" )
if args.j>0:
  os.system( "make -j{}".format(args.j) )
else:
  os.system( "make -j" )
  
