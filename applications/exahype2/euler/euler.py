import os
import sys
import argparse

sys.path.insert(0, os.path.abspath('../../../python'))

import peano4
import exahype2

dimensions                = 2
cell_size                 = 0.1
amr_levels                = 4 # Set to 0 to disable adaptive mesh refinement (AMR)
patch_size                = 4
rk_order                  = 4
dg_order                  = 2
end_time                  = 3.0
plot_interval             = end_time/100 # Set to 0 to disable I/O
#plot_interval             = 0
use_gpu                   = False # Requires a GPU enabled build
make                      = True
output_path               = "solution"

scenarios = [
  "PointExplosion",
  "PointExplosionWithDynamicAMR",
  "GaussianExplosion",
  "GaussianExplosionWithDynamicAMR",
  "BreakingDam",
  "BreakingDamWithDynamicAMR",
]
scenario = "GaussianExplosion"

fv_rusanov_solvers = {
  #"FVRusanovGlobalFixed":exahype2.solvers.fv.rusanov.GlobalFixedTimeStep, # admissible_time_step_size = min_h/patch_size*0.01
  #"FVRusanovGlobalFixedEnclave":exahype2.solvers.fv.rusanov.GlobalFixedTimeStepWithEnclaveTasking,
  "FVRusanovGlobalAdaptive":exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep,
  "FVRusanovGlobalAdaptiveEnclave":exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
  #"FVRusanovLocalEnclave":exahype2.solvers.fv.rusanov.LocalTimeStepWithEnclaveTasking,
}

rkdg_rusanov_solvers = {
  "RKDGRusanovGlobalAdaptive": exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep
}

solvers = {}
solvers.update(rkdg_rusanov_solvers)
solvers.update(fv_rusanov_solvers)
solver = "FVRusanovGlobalAdaptive"

build_modes = {
  "Release":peano4.output.CompileMode.Release,
  "Trace":peano4.output.CompileMode.Trace,
  "Assert":peano4.output.CompileMode.Asserts,
  "Stats":peano4.output.CompileMode.Stats,
  "Debug":peano4.output.CompileMode.Debug,
}
build_mode = "Release"

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler Application Script')

parser.add_argument("-s",       "--solver",            dest="solver",         choices=solvers.keys(),     default=solver,             help="|".join(solvers.keys()))
parser.add_argument("-o",       "--output",            dest="output_path",    type=str,                   default=output_path,        help="Output path for project solution output. The project will create a new folder at the given path. Default is 'solution'.")
parser.add_argument("-m",       "--build-mode",        dest="build_mode",     choices=build_modes.keys(), default=build_mode,         help="|".join(build_modes.keys()))
parser.add_argument("-d",       "--dim",               dest="dim",            type=int,                   default=dimensions,         help="Number of space dimensions")

parser.add_argument("-sc",      "--scenario",          dest="scenario",       choices=scenarios,          default=scenario,           help="|".join(scenarios))
parser.add_argument("-et",      "--end-time",          dest="end_time",       type=float,                 default=end_time,           help="End time of simulation")
parser.add_argument("-pi",      "--plot-interval",     dest="plot_interval",  type=float,                 default=plot_interval,      help="Time between plots and also between database entries when probes are used.")

parser.add_argument("-amr",     "--amr-levels",        dest="amr_levels",     type=int,                   default=amr_levels,         help="Number of AMR grid levels on top of max. size of a finite volume cell.")
parser.add_argument("-gpu",     "--gpu",               dest="gpu",            action="store_true",        default=use_gpu,            help="Enable GPU support (stateless PDE terms). Requires a GPU build.")

parser.add_argument("-pbc",     "--periodic-boundary-conditions",  dest="periodic_boundary_conditions",   action="store_true",        help="Use periodic boundary conditions" )

parser.add_argument("-maxh",    "--max-cell-size",     dest="max_cell_size",  type=float,                 default=cell_size,          help="Maximum size of a single finite volume cell on each axis.")
parser.add_argument("-ps",      "--patch-size",        dest="patch_size",     type=int,                   default=patch_size,         help="Number of finite volumes per axis (dimension) per patch.")

parser.add_argument("-rk-order","--rk-order",          dest="rk_order",       type=int,                   default=rk_order,           help="Order of time discretisation for Runge-Kutta scheme.")
parser.add_argument("-dg-order","--dg-order",          dest="dg_order",       type=int,                   default=dg_order,           help="Order of space discretisation for Discontinuous Galerkin.")

parser.add_argument(            "--make",              dest="make",           action="store_true",        default=make,               help="Compile the code after generation")
args = parser.parse_args()

if args.dim not in [2, 3]:
    print("Error, dimension must be 2 or 3, you supplied {}".format(args.dim))
    sys.exit(1)

if args.build_mode not in build_modes:
    print("Error, mode must be {} or {}, you supplied {}".format(", ".join(list(build_modes.keys())[:-1]), list(build_modes.keys())[-1],args.build_mode))
    sys.exit(1)

if args.solver not in solvers:
    print("Error, solver must be {} or {}, you supplied {}".format(", ".join(list(solvers.keys())[:-1]), list(solvers.keys())[-1],args.solver))
    sys.exit(1)

if args.scenario not in scenarios:
    print("Error, scenario must be {} or {}, you supplied {}".format(", ".join(scenarios[:-1]), scenarios[-1], args.scenario))
    sys.exit(1)

my_project = exahype2.Project(
  namespace=["applications", "exahype2", "euler"],
  project_name="Euler" + args.scenario,
  directory=".",
  executable="euler-" + str(dimensions) + "d-" + str(args.scenario)
)

solver_type = args.solver
if args.gpu:
  if not "Enclave" in solver_type:
    solver_type += "Enclave"
    if solver_type not in solvers:
      # This is necessary as "-s RKDGRusanovGlobalAdaptive -gpu" for example currently leads to this scenario
      raise ValueError("The requested solver {} is not available for GPUs!".format(solver_type))
solver_name = "EulerSolver" + str(args.scenario) + str(solver_type)

if args.gpu:
  solver_name += "GPU"

my_solver = None
if solver_type in fv_rusanov_solvers:
  my_solver = solvers[solver_type](
    name=solver_name,
    patch_size=args.patch_size,
    unknowns=args.dim + 2, # [rho, u, v, (w), e]
    auxiliary_variables=0,
    max_volume_h=args.max_cell_size,
    min_volume_h=args.max_cell_size * 3.0**(-args.amr_levels),
    time_step_relaxation=0.5,
)
elif solver_type in rkdg_rusanov_solvers:
  from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import FaceProjections
  my_solver = solvers[solver_type](
    name                      = solver_name,
    rk_order                  = args.rk_order,
    polynomials               = exahype2.solvers.GaussLegendreBasis(args.dg_order),
    face_projections          = FaceProjections.Solution, # TODO according to python/exahype2/solvers/rkdg/RungeKuttaDG.py, this should be == 2?
    unknowns                  = args.dim + 2, # [rho, u, v, (w), e]
    auxiliary_variables       = 0,
    max_cell_h                = args.max_cell_size,
    min_cell_h                = args.max_cell_size * 3.0**(-args.amr_levels),
    time_step_relaxation      = 0.5
)

if args.gpu:
  my_solver.set_pde_terms_without_state(True)

if args.scenario == "PointExplosion":
  from point_explosion import PointExplosion
  my_scenario = PointExplosion(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim)
elif args.scenario == "PointExplosionWithDynamicAMR":
  from point_explosion_with_dynamic_amr import PointExplosionWithDynamicAMR
  my_scenario = PointExplosionWithDynamicAMR(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim)
elif args.scenario == "GaussianExplosion":
  from gaussian_explosion import GaussianExplosion
  my_scenario = GaussianExplosion(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim)
elif args.scenario == "GaussianExplosionWithDynamicAMR":
  from gaussian_explosion_with_dynamic_amr import GaussianExplosionWithDynamicAMR
  my_scenario = GaussianExplosionWithDynamicAMR(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim)
elif args.scenario == "BreakingDam":
  from breaking_dam import BreakingDam
  my_scenario = BreakingDam(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim)
elif args.scenario == "BreakingDamWithDynamicAMR":
  from breaking_dam_with_dynamic_amr import BreakingDamWithDynamicAMR
  my_scenario = BreakingDamWithDynamicAMR(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim)

my_solver.set_implementation(
    initial_conditions    = my_scenario.implementation_of_initial_conditions(),
    boundary_conditions   = my_scenario.implementation_of_boundary_conditions(),
    flux                  = my_scenario.implementation_of_flux(),
    eigenvalues           = my_scenario.implementation_of_max_eigenvalue(),
    refinement_criterion  = my_scenario.implementation_of_refinement_criterion(),
    source_term           = my_scenario.implementation_of_sources(),
    use_split_loop = True,
    #memory_location = peano4.toolbox.blockstructured.ReconstructedArrayMemoryLocation.CallStack
)

my_solver.plot_description = my_scenario.unknown_identifier_for_plotter()

my_project.add_solver(my_solver)

if args.periodic_boundary_conditions:
  periodic_BC = [True, True, True]
else:
  periodic_BC = [False, False, False]

if args.dim == 2:
  size                    = [1.0, 1.0]
  offset                  = [0.0, 0.0]
else:
  size                    = [1.0, 1.0, 1.0]
  offset                  = [0.0, 0.0, 0.0]

my_project.set_global_simulation_parameters(
    dimensions=args.dim,
    size=size,
    offset=offset,
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=args.plot_interval,
    periodic_BC=periodic_BC,
)

output_path = str(args.output_path) + "-" + str(args.scenario)
my_project.set_output_path(output_path)
if not os.path.exists(output_path):
  os.makedirs(output_path)

my_project.set_load_balancing( "toolbox::loadbalancing::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()" )
my_project.set_Peano4_installation("../../../", mode=build_modes[args.build_mode])
my_project = my_project.generate_Peano4_project(verbose=True)
my_project.build(make=args.make, make_clean_first=True, throw_away_data_after_build=True)

print("\nPDE is: ")
print(my_scenario.__str__())
print("Executable is euler-" + str(dimensions) + "d-" + str(args.scenario))
print("Clean object files via 'make clean'")
print("Recompile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 euler.py' again")
print("Generate (and convert) any postprocessing data by calling 'python3 postprocess.py'")

with open("postprocess.py", "w") as file:
    file.write('''
import os
import sys
import subprocess

postprocessing_dir = "postprocessing-''' + str(args.scenario) + '''"
solution_dir = "solution-''' + str(args.scenario) + '''"
if not os.path.exists(postprocessing_dir):
    os.makedirs(postprocessing_dir)
if not os.path.exists(postprocessing_dir + "/" + solution_dir):
    os.makedirs(postprocessing_dir + "/" + solution_dir)

output_patch_file = solution_dir + "/solution-''' + str(solver_name) + '''.peano-patch-file"
if os.path.isfile(output_patch_file):
    subprocess.call([
        "python3", "../../../python/peano4/visualisation/render.py",
        str(output_patch_file),
        "--dir=" + postprocessing_dir
    ])
''')
