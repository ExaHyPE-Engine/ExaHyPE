import os
import sys
import argparse

sys.path.insert(0, os.path.abspath('../../../../python'))
import peano4
import exahype2

import numpy as np
import mghype

modes = {
  "release": peano4.output.CompileMode.Release,
  "trace":   peano4.output.CompileMode.Trace,
  "assert":  peano4.output.CompileMode.Asserts,
  "stats":   peano4.output.CompileMode.Stats,
  "debug":   peano4.output.CompileMode.Debug,
}

# parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler benchmarking script')
parser = argparse.ArgumentParser(description='Multigrid - Poisson solver')
parser.add_argument("-j",   "--parallel-builds",          dest="j",                       type=int,   default=-1, help="Parallel builds. Set to 0 to disable compile or to -1 (default) to use all cores")
parser.add_argument("-pt",   "--plot-timestep",     dest="plot_each_timestep", action="store_true", default=False, help="Plot after each timestep. By default (False) we plot after initialisation and at the end" )
parser.add_argument("-pd",  "--peano-dir",                dest="peanodir",                            default="../../../../../", help="Peano4 directory")
parser.add_argument("-cd",  "--configure-dir",            dest="configuredir",                        default="../../../../../", help="Location of configure" )
parser.add_argument("-v",   "--verbose",                  dest="verbose",                 action="store_true", default=True, help="Verbose")
parser.add_argument("-d",   "--dimensions",               dest="dimensions",              type=int,   default=2, help="Dimensions")
parser.add_argument("-upv","--unknowns_per_vertex_dof",  dest="upv", type=int,   default=3, help="unknowns per vertex dof")
parser.add_argument("-et",  "--end-time",                 dest="end_time",                type=float, default=1.0, help="Number of timesteps")
parser.add_argument("-amr", "--adaptive-levels",          dest="adaptivity_levels",       type=int,   default=0, help="Number of AMR grid levels on top of hmax (0 by default)")
parser.add_argument("-t",   "--type",                     dest="type",                    choices=["global-fixed", "global-adaptive"], required=True)
parser.add_argument("-pdt", "--plot-dt",                  dest="plot_snapshot_interval",              default=0, help="Time interval in-between two snapshots (switched off by default")
parser.add_argument("-cs",  "--cell-size",                dest="h",                       type=float, required=True, help="Mesh size")
parser.add_argument("-ps",  "--patch-size",               dest="patch_size",              type=int,   default=17, help="Dimensions")
parser.add_argument("-m",   "--mode",                     dest="mode",                    default="release", help="|".join(modes.keys()))
parser.add_argument("-p",   "--poly_degree",          dest="poly_degree",    type=int, default=1, help="Polynomial Degree")
parser.add_argument("-meshsize",  "--meshsize",       dest="meshsize",       default=0.3, help="Mesh size")
parser.add_argument("-omega_c",  "--omega_c",       dest="omega_c",       default=0.5, help="Mesh size")
parser.add_argument("-omega_f",  "--omega_f",       dest="omega_f",       default=1.0, help="Mesh size")
args = parser.parse_args()

if args.dimensions not in [2,3]:
    print("Error, dimension must be 2 or 3, you supplied {}".format(args.dimensions))
    import sys
    sys.exit(1)

if args.mode not in modes:
    print("Error, mode must be {} or {}, you supplied {}".format(", ",join(modes.keys()[:-1]),modes.keys()[-1],args.mode))
    import sys
    sys.exit(1)

print("\nConfiguring {}D Euler problem. Buildmode is {}, nbuilds={}.\n".format(args.dimensions, args.mode, args.j))

#
# Add the Finite Volumes solver
#
max_h          = args.h / args.patch_size
min_h          = 0.9 * args.h * 3.0**(-args.adaptivity_levels) / args.patch_size

time_step_size = 0.1 * min_h

auxiliary_variables = 0

euler_solver   = None
poisson_solver = None

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

#! [Instantiate solvers]

## Euler solver

# # @note: Trying to think in a modularised way:
# # there is MG code somewhere which I have to use and
# # assuming that we need different local stiffness matrices for LHS for different types of solvers
if args.type=="global-fixed":
  euler_solver = exahype2.solvers.fv.rusanov.GlobalFixedTimeStep(
    "Euler",
    args.patch_size,
    5, #unknowns,
    3, #auxiliary_variables,
    min_h, max_h,
    0.1*min_h,
    flux = exahype2.solvers.PDETerms.User_Defined_Implementation,
    eigenvalues = exahype2.solvers.PDETerms.User_Defined_Implementation,
    source_term = exahype2.solvers.PDETerms.User_Defined_Implementation
  )
if args.type=="global-adaptive":
  euler_solver = exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep(
    "Euler",
    args.patch_size,
    5, #unknowns,
    3, #auxiliary_variables,
    min_h, max_h,
    time_step_relaxation = 1e-2,
    flux = exahype2.solvers.PDETerms.User_Defined_Implementation,
    eigenvalues = exahype2.solvers.PDETerms.User_Defined_Implementation,
    source_term = exahype2.solvers.PDETerms.User_Defined_Implementation
  )

## Poisson solver

poisson_solver = mghype.matrixfree.solvers.api.DiscontinuousGalerkinDiscretisationPointWiseRiemannSolver(
  name = "Poisson",
  dimensions = args.dimensions,
  poly_degree = args.poly_degree,
  unknowns_per_cell_node = 1,
  solutions_per_face_node = 2,
  projections_per_face_node = 2,
  min_h = min_h,
  max_h = max_h,
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

# local_assembly_matrix = np.array([
#         0.6666666666666667, -0.16666666666666669, -0.16666666666666663, -0.33333333333333337
      
#         ,-0.16666666666666669, 0.6666666666666667, -0.33333333333333337, -0.16666666666666669
      
#         ,-0.16666666666666663, -0.33333333333333337, 0.6666666666666667, -0.16666666666666669
      
#         ,-0.33333333333333337, -0.16666666666666669, -0.16666666666666669, 0.6666666666666667
# ]).reshape((4,4))

# mass_matrix = np.eye( 2**args.dimensions )

# poisson_solver = mghype.matrixfree.solvers.api.CollocatedLowOrderDiscretisation("Poisson",
#                                                               args.upv,
#                                                               args.dimensions,
#                                                               args.meshsize,
#                                                               args.meshsize,
#                                                               local_assembly_matrix,
#                                                               local_assembly_matrix_scaling=0,
#                                                               mass_matrix = mass_matrix,
#                                                               mass_matrix_scaling = 0
#                                                             )
#! [Instantiate solvers]

# assert (
#     poisson_solver.name != euler_solver.name
# ), "names the solvers should not be the same"

# euler_solver.set_implementation(refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation)

#
# Create a project and configure it to end up in a subnamespace (and thus subdirectory).
#

euler_project = exahype2.Project(
  namespace     = ["applications", "exahype2", "euler", "selfgravitation" ],
  project_name  = "Euler",
  directory     = ".",
  subdirectory = "euler",
  executable    = "selfgravitation-euler"
)

# poisson_project = petsc.Project(
#   namespace     = ["applications", "exahype2", "euler", "selfgravitation"],
#   # subnamespace  = ["poisson"],
#   project_name  = "Poisson",
#   # subdirectory  = "poisson",
#   executable    = "selfgravitation-poisson"
# )

#! [Create project]
poisson_project = mghype.matrixfree.api.Project( 
  namespace = [ "applications", "exahype2", "euler", "selfgravitation" ],
  project_name = "Poisson", 
  directory     = ".",
  subdirectory  = "poisson",
  executable    = "selfgravitation-poisson",
  abstract_overwrite = False,
)
#! [Create project]

euler_project.add_solver(euler_solver)
poisson_project.add_solver(poisson_solver)

# #
# # Time step synchronisation. Ensure the Euler solver doesn't run ahead but waits
# # for the Poisson updates.
# #
# euler_solver._action_set_update_cell.guard += " and tarch::la::greaterEquals( fineGridCell" + \
#   poisson_solver.name + "CellLabel.getTimeStamp(), fineGridCell" + euler_solver.name + \
#   "CellLabel.getTimeStamp() )"

# #
# # Add the coupling terms
# #
# euler_solver.add_user_action_set_includes( """
# #include "toolbox/blockstructured/Copy.h"
# """)

# euler_solver.postprocess_updated_patch = """
#  ::toolbox::blockstructured::copyUnknown(
#    {},      // no of unknowns per dimension
#    fineGridCell{}Q.value, // source
#    0,       // the first index (0) is rho
#    5+3,     // unknowns in source (incl material parameters)
#    0,       // no overlap/halo here
#    fineGridCell{}PETScData.value, // dest
#    4,       // index four in destination, i.e. fifth entry
#    4+1,     // four unknowns in source (incl material parameters)
#    0        // no overlap/halo here
#  );
# """.format(args.patch_size, euler_solver.name, poisson_solver.name)

# poisson_solver.add_user_action_set_includes( """
# #include "toolbox/blockstructured/Derivative.h"
# """)

# poisson_solver.postprocess_updated_patch = """
#  double delta =
#  ::toolbox::blockstructured::computeGradientAndReturnMaxDifference(
#    {},      // no of unknowns per dimension
#    fineGridCellPoissonQ.value, // source
#    0,       // take first entry of solution vector, ignore helper entries
#    4+1,     // unknowns in source (incl material parameters)
#    0,       // no overlap/halo here
#    fineGridCellEulerQ.value, // source
#    #if Dimensions==2
#    {{5,6}},   // indices to which we write the gradient. Depends on dimension
#    #else
#    {{5,6,7}}, // and uses two brackets as escape symbol
#    #endif
#    5+3,     // four unknowns in source (incl material parameters)
#    0,       // no overlap/halo here
#    marker.h()
#  );
#  repositories::instanceOf{}.reportGradientDelta( delta );
# """.format(args.patch_size, poisson_solver.name)

# #
# # Ensure the Poisson solver does never overtake the Euler
# #
# poisson_solver._compute_new_time_step_size += """
# if (fineGridCell""" + poisson_solver.name + "CellLabel.getTimeStamp() > fineGridCell" + euler_solver.name + """CellLabel.getTimeStamp() ) {
#   timeStamp = fineGridCell""" + euler_solver.name + """CellLabel.getTimeStamp(); // reset time step size
#   timeStepSize = 0.0;           // invalidate time step size; the one chosen didn't make sense
#   fineGridCell""" + poisson_solver.name + """CellLabel.setTimeStamp( timeStamp );
# }
# else {
# }
# """

if args.dimensions == 2:
  size                    = [1.0, 1.0]
  offset                  = [0.0, 0.0]
else:
  size                    = [1.0, 1.0, 1.0]
  offset                  = [0.0, 0.0, 0.0]

#
# Lets configure some global parameters
#
euler_project.set_global_simulation_parameters(
  dimensions = args.dimensions,
  offset = offset,
  size = size,
  min_end_time = args.end_time,
  max_end_time = args.end_time,
  first_plot_time_stamp = 0.0,
  time_in_between_plots = args.plot_snapshot_interval,      # snapshots
  periodic_BC = [False, False, False]
)
poisson_project.set_global_simulation_parameters(
  dimensions            = args.dimensions,
  offset                = offset,
  domain_size           = size,
  plot_each_timestep    = args.plot_each_timestep,
)

#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
#
# Configure build
#
euler_project.set_load_balancing("toolbox::loadbalancing::strategies::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()")
euler_project.set_Peano4_installation(args.peanodir, modes[args.mode])
poisson_project.set_load_balancing("toolbox::loadbalancing::strategies::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()")
poisson_project.set_Peano4_installation(args.peanodir, modes[args.mode])

#
# This is the plain variant
#
# #
# # Create an 'empty' Peano4 project
# #
# peano4_project = euler_project.generate_Peano4_project(args.verbose)
# peano4_project = poisson_project.generate_Peano4_project(args.verbose)

peano4_project = peano4.Project(
  namespace     = [], # "applications", "exahype2", "euler", "selfgravitation"
  project_name  = "EulerPoisson", #"EulerPoisson",
  directory     = ".",
  executable    = "selfgravitation-new",
)
peano4_project.add_subproject(
  poisson_project.generate_Peano4_project(args.verbose),
)
peano4_project.add_subproject(
  euler_project.generate_Peano4_project(args.verbose),
  True
)

# peano4_project.add_subproject(
#   poisson_project.generate_Peano4_project(args.verbose),
#   # subnamespace = ["poisson"],
# )


peano4_project.output.makefile.parse_configure_script_outcome(args.configuredir)
peano4_project.output.makefile.add_header_search_path("euler")
peano4_project.output.makefile.add_header_search_path("poisson")
peano4_project.output.makefile.add_h_file("""{}/{}.h""".format(euler_project.subdirectory, euler_solver.name))
peano4_project.output.makefile.add_cpp_file("""{}/{}.cpp""".format(euler_project.subdirectory, euler_solver.name))
peano4_project.output.makefile.add_h_file("""{}/{}.h""".format(poisson_project.subdirectory, poisson_solver.name))
peano4_project.output.makefile.add_cpp_file("""{}/{}.cpp""".format(poisson_project.subdirectory, poisson_solver.name))
if args.j==0:
  peano4_project.generate()
else:
  peano4_project.build(make_clean_first=True, number_of_parallel_builds=args.j)
print("EulerPoisson Peano4 Project built successfully")

# print("Poisson Peano4 Project built successfully")
# print("EulerPoisson Peano4 project built successfully") #

# print("Convert any output via pvpython ~/git/Peano/python/peano4/visualisation/render.py solution-Euler.peano-patch-file")
