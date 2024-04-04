import os
import sys
import argparse

sys.path.insert(0, os.path.abspath('../../../../python'))
import peano4
import exahype2

# from multigrid.matrixgenerators import DLinear
# import multigrid.petsc as petsc

modes = {
  "release": peano4.output.CompileMode.Release,
  "trace":   peano4.output.CompileMode.Trace,
  "assert":  peano4.output.CompileMode.Asserts,
  "stats":   peano4.output.CompileMode.Stats,
  "debug":   peano4.output.CompileMode.Debug,
}

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler benchmarking script')
parser.add_argument("-j",   "--parallel-builds",          dest="j",                       type=int,   default=1, help="Parallel builds. Set to 0 to disable compile or to -1 to use all cores")
parser.add_argument("-pd",  "--peano-dir",                dest="peanodir",                            default="../../../../../", help="Peano4 directory")
parser.add_argument("-cd",  "--configure-dir",            dest="configuredir",                        default="../../../../../", help="Location of configure" )
parser.add_argument("-v",   "--verbose",                  dest="verbose",                 action="store_true", default=True, help="Verbose")
parser.add_argument("-d",   "--dimensions",               dest="dimensions",              type=int,   default=2, help="Dimensions")
parser.add_argument("-upvd","--unknowns_per_vertex_dof",  dest="unknowns_per_vertex_dof", type=int,   default=3, help="unknowns per vertex dof")
parser.add_argument("-et",  "--end-time",                 dest="end_time",                type=float, default=1.0, help="Number of timesteps")
parser.add_argument("-amr", "--adaptive-levels",          dest="adaptivity_levels",       type=int,   default=0, help="Number of AMR grid levels on top of hmax (0 by default)")
parser.add_argument("-t",   "--type",                     dest="type",                    choices=["global-fixed", "global-adaptive"], required=True)
parser.add_argument("-pdt", "--plot-dt",                  dest="plot_snapshot_interval",              default=0, help="Time interval in-between two snapshots (switched off by default")
parser.add_argument("-cs",  "--cell-size",                dest="h",                       type=float, required=True, help="Mesh size")
parser.add_argument("-ps",  "--patch-size",               dest="patch_size",              type=int,   default=17, help="Dimensions")
parser.add_argument("-m",   "--mode",                     dest="mode",                    default="release", help="|".join(modes.keys()))
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

# @note: Trying to think in a modularised way:
# there is MG code somewhere which I have to use and
# assuming that we need different local stiffness matrices for LHS for different types of solvers
matrices = None

if args.type=="global-fixed":
  euler_solver = exahype2.solvers.fv.rusanov.GlobalFixedTimeStep(
    "Euler",
    args.patch_size,
    5, #unknowns,
    3, #auxiliary_variables,
    min_h, max_h,
    time_step_size,
    flux = exahype2.solvers.PDETerms.User_Defined_Implementation,
    eigenvalues = exahype2.solvers.PDETerms.User_Defined_Implementation,
    source_term = exahype2.solvers.PDETerms.User_Defined_Implementation
  )
  # class DLinearFixedTimeStep(DLinear) # just the LHS matrix modification?
  # matrices = DLinearFixedTimeStep(args.dimensions, args.unknowns_per_vertex_dof)
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

#generate matrices
  # class DLinearAdaptiveTimeStep(DLinear) # just the LHS matrix modification?
  # matrices = DLinearAdaptiveTimeStep(args.dimensions, args.unknowns_per_vertex_dof)
# matrices = DLinear(args.dimensions, args.unknowns_per_vertex_dof)

# # @note: Is it about an individual CollocatedLowOrderDiscretisation for every type of solver?
# poisson_solver = petsc.solvers.CollocatedLowOrderDiscretisation(
#     "Poisson",
#     1,
#     args.dimensions,
#     args.h,
#     args.h,
#     matrices.get_cell_system_matrix_for_laplacian(),
#     matrices.get_cell_mass_matrix(),
#     1,
#     1,
#   )

# assert (
#     poisson_solver.name != euler_solver.name
# ), "names the solvers should not be the same"

euler_solver.set_implementation(refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation)

# peano4_project = peano4.Project(
#   namespace     = ["applications", "exahype2", "euler", "selfgravitation"],
#   project_name  = "EulerPoisson",
#   directory     = ".",
#   executable    = "selfgravitation-with-petsc",
# )

#
# Create a project and configure it to end up in a subnamespace (and thus subdirectory).
#
#euler_project = exahype2.Project(
#  namespace     = ["applications", "exahype2", "euler", "selfgravitation"],
#  # subnamespace  = ["euler"],
#  project_name  = "Euler",
#  directory     = ".",
##  subdirectory  = "euler",
#  executable    = "selfgravitation-euler"
#)

euler_project = exahype2.Project(
  namespace     = ["applications", "exahype2", "euler", "selfgravitation", "euler"],
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

euler_project.add_solver(euler_solver)
# poisson_project.add_solver(poisson_solver)

# #
# # Time step synchronisation. Ensure the Euler solver doesn't run ahead but waits
# # for the Poisson updates.
# #
# euler_solver._action_set_update_cell.guard += " and tarch::la::greaterEquals( fineGridCell" + \
#   poisson_solver.name + "CellLabel.getTimeStamp(), fineGridCell" + euler_solver.name + \
#   "CellLabel.getTimeStamp() )"

#
# Add the coupling terms
#
euler_solver.add_user_action_set_includes( """
#include "toolbox/blockstructured/Copy.h"
""")

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
# poisson_project.set_global_simulation_parameters(
#   dimensions            = args.dimensions,
#   offset                = offset,
#   domain_size           = size,
# )

#
# So here's the parallel stuff. This is new compared to the serial
# prototype we did start off with.
#
euler_project.set_load_balancing("toolbox::loadbalancing::strategies::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()")
euler_project.set_Peano4_installation(args.peanodir, modes[args.mode])
# poisson_project.set_load_balancing("toolbox::loadbalancing::strategies::RecursiveSubdivision", "new ::exahype2::LoadBalancingConfiguration()")
# poisson_project.set_Peano4_installation(args.peanodir, modes[args.mode])

#
# This is the plain variant
#
# #
# # Create an 'empty' Peano4 project
# #
peano4_project = euler_project.generate_Peano4_project(args.verbose)

# peano4_project = peano4.Project()
# peano4_project.add_subproject( euler_project.generate_Peano4_project(args.verbose),
                              # subnamespace = ["euler"]
                              # )

# peano4_project.add_subproject(
#   poisson_project.generate_Peano4_project(args.verbose),
#   # subnamespace = ["poisson"],
# )


# peano4_project.output.makefile.add_h_file("""{}.h""".format(euler_solver.name))
# peano4_project.output.makefile.add_cpp_file("""{}.cpp""".format(euler_solver.name))
# peano4_project.output.makefile.add_h_file("""{}.h""".format(poisson_solver.name))
# peano4_project.output.makefile.add_cpp_file("""{}.cpp""".format(poisson_solver.name))

peano4_project.output.makefile.parse_configure_script_outcome(args.configuredir)
peano4_project.output.makefile.add_header_search_path("euler")
if args.j==0:
  peano4_project.generate()
else:
  peano4_project.build(make_clean_first=True, number_of_parallel_builds=args.j)
print("Euler Peano4 Project built successfully") #Poisson

# print("The combined Euler-Poisson Peano4 project built successfully")

# print("Convert any output via pvpython ~/git/Peano/python/peano4/visualisation/render.py solution-Euler.peano-patch-file")
