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
parser.add_argument("-deg", "--degree",             dest="degree",         type=int, required=True, help="Polynomial degree")
parser.add_argument("-meshsize",  "--meshsize",     dest="meshsize",       default=0.1, help="Mesh size")
parser.add_argument("-m",   "--mode",               dest="mode",           choices=["release","stats","asserts"], required=True, help="Pick build type" )
args = parser.parse_args()

assert args.dimensions == 2, "not ready for 3d yet"

project = multigrid.petsc.Project(project_name = "DG", 
                                  namespace = [ "benchmarks", "multigrid", "petsc", "poisson" ]
                                  ) 

                        
matrices = multigrid.matrixgenerators.GaussLobatto(poly_degree            = args.degree,
                                                   Dimensions             = args.dimensions,
                                                   unknowns_per_cell_node = 1,
                                                   unknowns_per_face_node = 1,  # for DG, this has to equal unknowns_per_cell_dof
                                                   )

solver = multigrid.petsc.solvers.DiscontinuousGalerkinDiscretisation(name                 = "DGPoisson",
                                                                     polynomial_degree    = args.degree,
                                                                     cell_unknowns        = 1,                      # unknowns_per_cell_dof,
                                                                     face_unknowns        = 1,                      # unknowns_per_face_dof,
                                                                     dimensions           = args.dimensions,
                                                                     min_h                = args.meshsize,
                                                                     max_h                = args.meshsize,
                                                                     cell_cell_rhs_matrix   = matrices.get_cell_mass_matrix(),     
                                                                     cell_cell_lhs_matrix   = matrices.get_cell_system_matrix_for_laplacian(),
                                                                     cell_cell_lhs_matrix_scaling = args.dimensions-2,
                                                                     cell_cell_rhs_matrix_scaling = args.dimensions,
                                                                     cell_to_face_matrix     = matrices.get_cell_to_face_matrix(),       
                                                                     face_to_cell_matrix     = matrices.get_cell_identity_matrix(), 
                                                                     cell_to_face_matrix_scaling  = 0,       
                                                                     face_to_cell_matrix_scaling  = args.dimensions, 
                                                                     face_face_Riemann_problem_matrix=matrices.get_face_face_riemann_problem_matrix(),
                                                                     quadrature_points_in_unit_interval = matrices.get_points_unit_interval()
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


project.set_load_balancing( "toolbox::loadbalancing::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()" )
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
