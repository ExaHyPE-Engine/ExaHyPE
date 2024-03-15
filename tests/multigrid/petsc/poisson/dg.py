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
parser.add_argument("-deg", "--degree",             dest="degree",         type=int, default=1, help="Polynomial degree")
parser.add_argument("-upcn", "--cell-unknowns",     dest="upcn",           type=int, default=1, help="Unknowns per cell node")
parser.add_argument("-upfn", "--face-unknowns",     dest="upfn",           type=int, default=2, help="Unknowns per face node")
parser.add_argument("-meshsize",  "--meshsize",     dest="meshsize",       default=0.4, help="Mesh size")
parser.add_argument("-m",   "--mode",               dest="mode",           choices=["release","stats","asserts", "trace"], required=True, help="Pick build type" )
args = parser.parse_args()

assert args.dimensions == 2, "not ready for 3d yet"

#! [Create project]
project = multigrid.petsc.api.Project(project_name = "DG", 
                                  namespace = [ "benchmarks", "multigrid", "petsc", "poisson" ],

                                  # next two arguments let us customise the solver we use in petsc
                                  # see enum class in Peano/src/petsc/LinearEquationSystem.h for implemented options
                                  preconditioner_type = "JACOBI",
                                  solver_type         = "GMRES"
                                  ) 
#! [Create project]

                        
matrices = multigrid.api.matrixgenerators.GaussLobatto(poly_degree            = args.degree,
                                                   Dimensions             = args.dimensions,
                                                   unknowns_per_cell_node = args.upcn,
                                                   # this attribute is not used by code written by Alex
                                                   unknowns_per_face_node = args.upfn,
                                                   )

#! [Create solver]
solver = multigrid.petsc.api.solvers.DiscontinuousGalerkinDiscretisation(name                 = "DGPoisson",
                                                                     polynomial_degree    = args.degree,
                                                                     cell_unknowns        = args.upcn,                      # unknowns_per_cell_dof,
                                                                     face_unknowns        = args.upfn,                      # unknowns_per_face_dof,
                                                                     dimensions           = args.dimensions,
                                                                     min_h                = args.meshsize,
                                                                     max_h                = args.meshsize,
                                                                     cell_cell_rhs_matrix   = matrices.get_cell_mass_matrix(),     
                                                                     cell_cell_lhs_matrix   = matrices.get_cell_system_matrix_for_laplacian(),
                                                                     cell_cell_lhs_matrix_scaling = args.dimensions-2,
                                                                     cell_cell_rhs_matrix_scaling = args.dimensions,
                                                                     cell_to_face_matrix    = matrices.get_cell_to_face_matrix(),       
                                                                     face_to_cell_matrix    = matrices.get_face_to_cell_matrix(), 
                                                                     
                                                                     # removing scaling for debugging
                                                                     cell_to_face_matrix_scaling  = None,
                                                                     face_to_cell_matrix_scaling  = None, 
                                                                     face_face_Riemann_problem_matrix=matrices.get_face_face_riemann_problem_matrix(),
                                                                     quadrature_points_in_unit_interval = matrices.get_points_unit_interval(),
                                                                     gaussian_integration_points= matrices.get_points1d_legendre(),
                                                                     gaussian_integration_weights=matrices.get_weights1d_legendre()
                                                                     )

project.add_solver( solver )
#! [Create solver]

#! [Configure build]
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
#! [Configure build]
    
    
#! [Create Peano project]
peano4_project = project.generate_Peano4_project(args.verbose)
peano4_project.generate()

os.system( "make clean" )
if args.j>0:
  os.system( "make -j{}".format(args.j) )
else:
  os.system( "make -j" )
#! [Create Peano project]
  
print("Done")
