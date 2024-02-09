import os, sys
import argparse
import peano4
import numpy as np

import src.multigrid.matrixfree as matrixfree
import multigrid.matrixgenerators

print( """

Peano 4 multigrid solver using Discontinuous Galerkin

@author 2022 Sean Baccas, Dmitry Nikolaenko, Tobias Weinzierl

""")


parser = argparse.ArgumentParser(description='Multigrid - Poisson solver')
parser.add_argument("-j",   "--parallel-builds",      dest="j",              type=int, default=-1, help="Parallel builds" )
parser.add_argument("-pd",  "--peano-dir",            dest="peanodir",       default="../../../../", help="Peano4 directory" )
parser.add_argument("-v",   "--verbose",              dest="verbose",        action="store_true", default=False, help="Verbose")
parser.add_argument("-d",   "--dimensions",           dest="dimensions",     type=int, default=2, help="Dimensions")
parser.add_argument("-p",   "--poly_degree",          dest="poly_degree",    type=int, default=1, help="Polynomial Degree")
parser.add_argument("-upcn", "--unknowns_per_cell",   dest="upcn",            type=int, default=1, help="Unknowns per cell node")
parser.add_argument("-spfn", "--solutions_per_face_node",  dest="spfn",            type=int, default=2, help="Number of solutions to store per face node")
parser.add_argument("-ppfn", "--projections_per_face_node",dest="ppfn",            type=int, default=2, help="Number of projections to store per face node")
parser.add_argument("-meshsize",  "--meshsize",       dest="meshsize",       default=0.3, help="Mesh size")
parser.add_argument("-m",   "--mode",                 dest="mode",           choices=["release","stats","asserts", "trace"], required=True, help="Pick build type" )
args = parser.parse_args()

#! [Create project]
project = matrixfree.Project( project_name = "DGPointwise", 
                              namespace = [ "benchmarks", "multigrid", "matrixfree", "poisson" ]
                            )
#! [Create project]


#! [Construct matrices]

'''
TODO: hand this over to Alex. The shapes here are an indicator for what needs to go where...
'''

# assembly_matrix = np.eye( args.upcn * (args.poly_degree + 1) ** (args.dimensions) )
# mass_matrix     = np.eye( args.upcn * (args.poly_degree + 1) ** (args.dimensions) )

# face_from_cell  = np.zeros(( 2* args.ppfn * (args.poly_degree + 1) ** (args.dimensions-1) * 2*args.dimensions,   args.upcn * (args.poly_degree + 1) ** (args.dimensions) ))

# # we are sending the solns on the face onto the cell
# cell_from_face  = np.zeros(( args.upcn * (args.poly_degree + 1) ** (args.dimensions),     args.spfn * (args.poly_degree + 1) ** (args.dimensions-1) * 2*args.dimensions ) )

# riemann_matrix  = np.zeros( (args.spfn * (args.poly_degree + 1) ** (args.dimensions-1), 2* args.ppfn * (args.poly_degree + 1) ** (args.dimensions-1)) )
# approximate_system_matrix = np.eye( args.upcn * (args.poly_degree + 1) ** (args.dimensions) )

matrices = multigrid.matrixgenerators.GLMatrixFree(
  args.dimensions,
  args.poly_degree,
  args.upcn,
  args.spfn,
  args.ppfn
)

#! [Construct matrices]


#! [Instantiate solver]

assembly_matrix, assembly_matrix_scaling = matrices.get_cell_system_matrix_for_laplacian()
mass_matrix, mass_matrix_scaling         = matrices.get_cell_mass_matrix()
face_from_cell_projection, \
face_from_cell_projection_scaling        = matrices.get_face_from_cell_matrix()
cell_from_face_projection, \
cell_from_face_projection_scaling        = matrices.get_cell_from_face_matrix()
approximate_system_matrix, \
approximate_system_matrix_scaling        = matrices.get_A_tilde()


solver = matrixfree.solvers.api.DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver(
  name = "DGPoisson",
  dimensions = args.dimensions,
  poly_degree = args.poly_degree,
  unknowns_per_cell_node = args.upcn,
  solutions_per_face_node = args.spfn,
  projections_per_face_node = args.ppfn,
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
  cell_relaxation = 0.1,
  face_relaxation = 0.1,
  approximate_system_matrix = approximate_system_matrix,
  approximate_system_matrix_scaling = approximate_system_matrix_scaling,
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
  
