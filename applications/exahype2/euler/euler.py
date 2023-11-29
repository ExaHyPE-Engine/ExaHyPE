import os
import sys
import argparse

sys.path.insert(0, os.path.abspath('../../../python'))
import peano4
import exahype2

dimensions                = 2
cell_size                 = 0.1
amr_levels                = 3
patch_size                = 4
rk_order                  = 2
dg_order                  = 1
end_time                  = 3.0
plot_interval             = end_time/100
#plot_interval             = 0
use_gpu                   = False
use_make                  = True
use_symbolic_description  = False
output_path               = "solution"

scenarios = [
  "PointExplosion",
  "PointExplosionWithDynamicAMR",
  "GaussianExplosion",
  "GaussianExplosionWithDynamicAMR",
  "BreakingDam",
  "BreakingDamWithDynamicAMR",
]
scenario = "PointExplosion"

fv_rusanov_solvers = {
  "FVRusanovGlobalFixed":exahype2.solvers.fv.rusanov.GlobalFixedTimeStep,
  "FVRusanovGlobalFixedEnclave":exahype2.solvers.fv.rusanov.GlobalFixedTimeStepWithEnclaveTasking,
  "FVRusanovGlobalAdaptive":exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep,
  "FVRusanovGlobalAdaptiveEnclave":exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
}

rkdg_rusanov_solvers = {
  "RKDGRusanovGlobalFixed": exahype2.solvers.rkdg.rusanov.GlobalFixedTimeStep,
  "RKDGRusanovGlobalAdaptive": exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep,
  "RKDGRusanovGlobalAdaptiveEnclave": exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
}

solvers = {}
solvers.update(fv_rusanov_solvers)
solvers.update(rkdg_rusanov_solvers)
solver = "RKDGRusanovGlobalFixed"

build_modes = {
  "Release":peano4.output.CompileMode.Release,
  "Trace":peano4.output.CompileMode.Trace,
  "Assert":peano4.output.CompileMode.Asserts,
  "Stats":peano4.output.CompileMode.Stats,
  "Debug":peano4.output.CompileMode.Debug,
}
build_mode = "Release"

storage_types = {
  "CallStack":exahype2.solvers.fv.Storage.CallStack,
  "Heap":exahype2.solvers.fv.Storage.Heap,
  "SmartPointers":exahype2.solvers.fv.Storage.SmartPointers,
}
storage_type = "CallStack"

parser = argparse.ArgumentParser(description='ExaHyPE 2 - Euler Application Script')

parser.add_argument("-s",       "--solver",            dest="solver",         choices=solvers.keys(),     default=solver,             help="|".join(solvers.keys()))
parser.add_argument("-o",       "--output",            dest="output_path",    type=str,                   default=output_path,        help="Output path for project solution output. The project will create a new folder at the given path. Default is 'solution'.")
parser.add_argument("-m",       "--build-mode",        dest="build_mode",     choices=build_modes.keys(), default=build_mode,         help="|".join(build_modes.keys()))
parser.add_argument("-d",       "--dim",               dest="dim",            type=int,                   default=dimensions,         help="Number of space dimensions")

parser.add_argument("-st",      "--storage",           dest="storage",        choices=storage_types.keys(), default=storage_type,     help="|".join(storage_types.keys()))
parser.add_argument("-sc",      "--scenario",          dest="scenario",       choices=scenarios,          default=scenario,           help="|".join(scenarios))
parser.add_argument("-et",      "--end-time",          dest="end_time",       type=float,                 default=end_time,           help="End time of simulation")
parser.add_argument("-pi",      "--plot-interval",     dest="plot_interval",  type=float,                 default=plot_interval,      help="Time between plots and also between database entries when probes are used.")

parser.add_argument("-amr",     "--amr-levels",        dest="amr_levels",     type=int,                   default=amr_levels,         help="Number of AMR grid levels on top of max. size of a finite volume cell.")
parser.add_argument("-sym",     "--symbolic",          dest="symbolic",       action="store_true",        default=use_symbolic_description, help="Use the symbolic PDE description of the Euler equations.")
parser.add_argument("-gpu",     "--gpu",               dest="use_gpu",        action="store_true",        default=use_gpu,            help="Use GPU offloading (requires a GPU enabled Peano build).")

parser.add_argument("-pbc",     "--periodic-boundary-conditions",  dest="periodic_boundary_conditions",   action="store_true",        help="Use periodic boundary conditions" )

parser.add_argument("-maxh",    "--max-cell-size",     dest="max_cell_size",  type=float,                 default=cell_size,          help="Maximum size of a single finite volume cell on each axis.")
parser.add_argument("-ps",      "--patch-size",        dest="patch_size",     type=int,                   default=patch_size,         help="Number of finite volumes per axis (dimension) per patch.")

parser.add_argument("-rk-order","--rk-order",          dest="rk_order",       type=int,                   default=rk_order,           help="Order of time discretisation for Runge-Kutta scheme.")
parser.add_argument("-dg-order","--dg-order",          dest="dg_order",       type=int,                   default=dg_order,           help="Order of space discretisation for Discontinuous Galerkin.")

parser.add_argument("--trees",                         dest="trees",          type=int,                   default=0,                  help="Number of trees (subpartitions) per rank.")

parser.add_argument(            "--make",                                     action="store_true",                                    help="Compile the code after generation")
parser.add_argument(            "--no-make",           dest="use_make",       action="store_false",                                   help="Do not compile the code after generation")
parser.set_defaults(use_make=use_make)
parser.add_argument(            "--executable",        dest="executable",     type=str,                                               help="Specify the executable name. If none is chosen, the name is automatically deduced by some parameters.")
args = parser.parse_args()

if args.dim not in [2, 3]:
    print("Error, dimension must be 2 or 3, you supplied {}".format(args.dim))
    sys.exit(1)

if args.build_mode not in build_modes:
    print("Error, mode must be {} or {}, you supplied {}".format(", ".join(list(build_modes.keys())[:-1]), list(build_modes.keys())[-1], args.build_mode))
    sys.exit(1)

if args.solver not in solvers:
    print("Error, solver must be {} or {}, you supplied {}".format(", ".join(list(solvers.keys())[:-1]), list(solvers.keys())[-1], args.solver))
    sys.exit(1)

if args.storage not in storage_type:
    print("Error, storage type must be {} or {}, you supplied {}".format(", ".join(list(storage_type.keys())[:-1]), list(storage_type.keys())[-1], args.storage))
    sys.exit(1)

if args.scenario not in scenarios:
    print("Error, scenario must be {} or {}, you supplied {}".format(", ".join(scenarios[:-1]), scenarios[-1], args.scenario))
    sys.exit(1)

if args.executable:
  executable_name = args.executable
else:
  executable_name = "euler-" + str(args.dim) + "d-" + str(args.scenario)
my_project = exahype2.Project(
  namespace=["applications", "exahype2", "euler"],
  project_name="Euler" + args.scenario,
  directory=".",
  executable=executable_name
)

solver_type = args.solver
if args.use_gpu:
  if not "Enclave" in solver_type:
    solver_type += "Enclave"
    if solver_type not in solvers:
      raise ValueError("The requested solver {} is not available for GPU offloading!".format(solver_type))
solver_name = "EulerSolver" + str(args.scenario) + str(solver_type)
if args.use_gpu:
  solver_name += "GPU"

my_solver = None
if solver_type in fv_rusanov_solvers and "Adaptive" in solver_type:
  my_solver = solvers[solver_type](
    name=solver_name,
    patch_size=args.patch_size,
    unknowns=args.dim + 2, # [rho, u, v, (w), e]
    auxiliary_variables=0,
    max_volume_h=args.max_cell_size,
    min_volume_h=args.max_cell_size * 3.0**(-args.amr_levels),
    time_step_relaxation=0.5,
  )
  #self._patch.generator = peano4.datamodel.PatchToDoubleArrayOnHeap(self._patch,"double")
  my_solver.switch_to_heap_storage(
    cell_data_storage=storage_types[args.storage],
    face_data_storage=storage_types[args.storage],
  )

  #from exahype2.solvers.fv.rusanov.kernels import create_compute_Riemann_kernel_for_Rusanov
  #my_solver._fused_compute_kernel_call_cpu = create_compute_Riemann_kernel_for_Rusanov(
  #  my_solver._flux_implementation,
  #  my_solver._ncp_implementation,
  #  my_solver._source_term_implementation,
  #  compute_max_eigenvalue_of_next_time_step=True,
  #  solver_variant = exahype2.solvers.fv.rusanov.SolverVariant.Multicore,
  #  kernel_variant = exahype2.solvers.fv.rusanov.KernelVariant.BatchedAoSHeap
  #)
elif solver_type in fv_rusanov_solvers and "Fixed" in solver_type:
  min_h = args.max_cell_size * 3.0**(-args.amr_levels)
  admissible_time_step_size = min_h/patch_size*0.5
  my_solver = solvers[solver_type](
    name=solver_name,
    patch_size=args.patch_size,
    unknowns=args.dim + 2, # [rho, u, v, (w), e]
    auxiliary_variables=0,
    max_volume_h=args.max_cell_size,
    min_volume_h=min_h,
    normalised_time_step_size=admissible_time_step_size,
  )
elif solver_type in rkdg_rusanov_solvers and "Adaptive" in solver_type:
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
elif solver_type in rkdg_rusanov_solvers and "Fixed" in solver_type:
  from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import FaceProjections
  min_h = args.max_cell_size * 3.0**(-args.amr_levels)
  admissible_time_step_size = min_h/patch_size*0.5
  my_solver = solvers[solver_type](
    name                      = solver_name,
    rk_order                  = args.rk_order,
    polynomials               = exahype2.solvers.GaussLegendreBasis(args.dg_order),
    face_projections          = FaceProjections.Solution, # TODO according to python/exahype2/solvers/rkdg/RungeKuttaDG.py, this should be == 2?
    unknowns                  = args.dim + 2, # [rho, u, v, (w), e]
    auxiliary_variables       = 0,
    max_cell_h                = args.max_cell_size,
    min_cell_h                = min_h,
    time_step_size            = admissible_time_step_size,
  )

if args.use_gpu:
  my_solver.set_pde_terms_without_state(True)

if args.scenario == "PointExplosion":
  from point_explosion import PointExplosion
  my_scenario = PointExplosion(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim, use_symbolic_description=args.symbolic)
elif args.scenario == "PointExplosionWithDynamicAMR":
  from point_explosion_with_dynamic_amr import PointExplosionWithDynamicAMR
  my_scenario = PointExplosionWithDynamicAMR(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim, use_symbolic_description=args.symbolic)
elif args.scenario == "GaussianExplosion":
  from gaussian_explosion import GaussianExplosion
  my_scenario = GaussianExplosion(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim, use_symbolic_description=args.symbolic)
elif args.scenario == "GaussianExplosionWithDynamicAMR":
  from gaussian_explosion_with_dynamic_amr import GaussianExplosionWithDynamicAMR
  my_scenario = GaussianExplosionWithDynamicAMR(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim, use_symbolic_description=args.symbolic)
elif args.scenario == "BreakingDam":
  from breaking_dam import BreakingDam
  my_scenario = BreakingDam(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim, use_symbolic_description=args.symbolic)
elif args.scenario == "BreakingDamWithDynamicAMR":
  from breaking_dam_with_dynamic_amr import BreakingDamWithDynamicAMR
  my_scenario = BreakingDamWithDynamicAMR(unknowns=args.dim + 2, auxiliary_variables=0, dimensions=args.dim, use_symbolic_description=args.symbolic)

my_solver.set_implementation(
    initial_conditions    = my_scenario.implementation_of_initial_conditions(),
    boundary_conditions   = my_scenario.implementation_of_boundary_conditions(),
    flux                  = my_scenario.implementation_of_flux(),
    eigenvalues           = my_scenario.implementation_of_max_eigenvalue(),
    refinement_criterion  = my_scenario.implementation_of_refinement_criterion(),
    source_term           = my_scenario.implementation_of_sources(),
)

if args.symbolic:
  my_solver.plot_description = my_scenario.pde.unknown_identifier_for_plotter()
else:
  if args.dim == 2:
    my_solver.plot_description = "rho, u, v, e"
  else:
    my_solver.plot_description = "rho, u, v, w, e"
  my_solver.add_user_solver_includes("""
#include "EulerKernels.h"
""")

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

solution_dir = str(args.output_path) + "-" + str(solver_name) + "-" + str(args.dim) + "d"
my_project.set_output_path(solution_dir)
if not os.path.exists(solution_dir):
  os.makedirs(solution_dir)

#my_project.set_load_balancing("toolbox::loadbalancing::strategies::SpreadOut", "new ::exahype2::LoadBalancingConfiguration(0.98)")
if args.trees > 0:
  my_project.set_load_balancing("toolbox::loadbalancing::strategies::SpreadOutHierarchically", "new ::exahype2::LoadBalancingConfiguration(0.98, 100, " + str(args.trees) + ")")
else:
  my_project.set_load_balancing("toolbox::loadbalancing::strategies::SpreadOutHierarchically", "new ::exahype2::LoadBalancingConfiguration(0.98)")
#my_project.set_load_balancing("toolbox::loadbalancing::strategies::cascade::SpreadOut_RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration(0.98, 0, 16)")
#my_project.set_load_balancing("toolbox::loadbalancing::strategies::cascade::SpreadOut_SplitOversizedTree", "new ::exahype2::LoadBalancingConfiguration(0.98, 1000, 16)")
#my_project.set_load_balancing("toolbox::loadbalancing::strategies::RecursiveBipartition", "new ::exahype2::LoadBalancingConfiguration()")
my_project.set_Peano4_installation("../../../", mode=build_modes[args.build_mode])
my_project = my_project.generate_Peano4_project(verbose=True)
my_project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")
my_project.build(make=args.use_make, make_clean_first=True, throw_away_data_after_build=True)

with open("postprocess.py", "w") as file:
    file.write('''
import os
import sys
import subprocess

output_patch_file = "''' + str(solution_dir) + '''/solution-''' + str(solver_name) + '''.peano-patch-file"
if os.path.isfile(output_patch_file):
    subprocess.call([
        "python3", "../../../python/peano4/visualisation/render.py",
        str(output_patch_file),
    ])
''')

if args.symbolic:
  print("\nPDE is: ")
  print(my_scenario.pde.__str__())

print("Executable is ", executable_name)
print("Clean object files via 'make clean'")
print("Recompile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 euler.py' again")
print("Generate (and convert) any postprocessing data by calling 'python3 postprocess.py'")
