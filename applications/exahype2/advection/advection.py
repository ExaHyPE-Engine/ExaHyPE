# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys
import argparse
import subprocess
import peano4
import exahype2

# Some reasonable default values
dimensions = 2
cell_size = 0.1
amr_levels = 2
patch_size = 4
rk_order = 2
dg_order = 3
end_time = 3.0
plot_interval = end_time / 100
# plot_interval             = 0
advection_speed = 1.0

fv_rusanov_solvers = {
    "FVRusanovGlobalFixed": exahype2.solvers.fv.rusanov.GlobalFixedTimeStep,
    "FVRusanovGlobalFixedEnclave": exahype2.solvers.fv.rusanov.GlobalFixedTimeStepWithEnclaveTasking,
    "FVRusanovGlobalAdaptive": exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStep,
    "FVRusanovGlobalAdaptiveEnclave": exahype2.solvers.fv.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
}

rkdg_rusanov_solvers = {
    "RKDGRusanovGlobalFixed": exahype2.solvers.rkdg.rusanov.GlobalFixedTimeStep,
    "RKDGRusanovGlobalAdaptive": exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep,
    "RKDGRusanovGlobalAdaptiveEnclave": exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStepWithEnclaveTasking,
}

aderdg_rusanov_solvers = {
    "ADERDGRusanovGlobalFixed": exahype2.solvers.aderdg.rusanov.GlobalFixedTimeStep,
    "ADERDGRusanovGlobalAdaptive": exahype2.solvers.aderdg.rusanov.GlobalAdaptiveTimeStep,
}

solvers = {}
solvers.update(fv_rusanov_solvers)
solvers.update(rkdg_rusanov_solvers)
solvers.update(aderdg_rusanov_solvers)
solver = "ADERDGRusanovGlobalAdaptive"

build_modes = {
    "Release": peano4.output.CompileMode.Release,
    "Trace": peano4.output.CompileMode.Trace,
    "Assert": peano4.output.CompileMode.Asserts,
    "Stats": peano4.output.CompileMode.Stats,
    "Debug": peano4.output.CompileMode.Debug,
}
build_mode = "Release"

storage_types = {
    "CallStack":     exahype2.solvers.Storage.CallStack,
    "Heap":          exahype2.solvers.Storage.Heap,
    "SmartPointers": exahype2.solvers.Storage.SmartPointers,
}
storage_type = "CallStack"

parser = argparse.ArgumentParser(description="ExaHyPE 2 - Advection Application Script")

parser.add_argument(
    "-s",
    "--solver",
    dest="solver",
    choices=solvers.keys(),
    default=solver,
    help="|".join(solvers.keys()),
)
parser.add_argument(
    "-o",
    "--output",
    dest="output_path",
    type=str,
    default="solution",
    help="Output path for project solution output. The project will create a new folder at the given path. Default is 'solution'.",
)
parser.add_argument(
    "-m",
    "--build-mode",
    dest="build_mode",
    choices=build_modes.keys(),
    default=build_mode,
    help="|".join(build_modes.keys()),
)
parser.add_argument(
    "-d",
    "--dim",
    dest="dim",
    type=int,
    default=dimensions,
    help="Number of space dimensions",
)

parser.add_argument(
    "-st",
    "--storage",
    dest="storage",
    choices=storage_types.keys(),
    default=storage_type,
    help="|".join(storage_types.keys()),
)
parser.add_argument(
    "-et",
    "--end-time",
    dest="end_time",
    type=float,
    default=end_time,
    help="End time of simulation",
)
parser.add_argument(
    "-pi",
    "--plot-interval",
    dest="plot_interval",
    type=float,
    default=plot_interval,
    help="Time between plots and also between database entries when probes are used.",
)

parser.add_argument(
    "-amr",
    "--amr-levels",
    dest="amr_levels",
    type=int,
    default=amr_levels,
    help="Number of AMR grid levels on top of max. size of a finite volume cell.",
)
parser.add_argument(
    "-dynamic",
    "--dynamic-amr",
    dest="dynamic_amr",
    action="store_true",
    default=False,
    help="Use dynamic AMR.",
)
parser.add_argument(
    "-stateless",
    "--stateless",
    dest="use_stateless_pde_terms",
    action="store_true",
    default=True,
    help="Use stateless PDE terms (GPU offloading requires a GPU enabled Peano build and an enclave solver).",
)
parser.add_argument(
    "-f",
    "--fuse",
    dest="fused_tasks",
    type=int,
    default=16384,
    help="Number of enclave tasks to fuse into one meta task.",
)
parser.add_argument(
    "-vis",
    "--visualise",
    dest="visualise",
    action="store_true",
    default=False,
    help="Visualise the output data using the postprocessor.",
)

parser.add_argument(
    "-pbc",
    "--periodic-boundary-conditions",
    dest="periodic_boundary_conditions",
    action="store_true",
    help="Use periodic boundary conditions",
)

parser.add_argument(
    "-maxh",
    "--max-cell-size",
    dest="max_cell_size",
    type=float,
    default=cell_size,
    help="Maximum size of a single finite volume cell on each axis.",
)
parser.add_argument(
    "-ps",
    "--patch-size",
    dest="patch_size",
    type=int,
    default=patch_size,
    help="Number of finite volumes per axis (dimension) per patch.",
)

parser.add_argument(
    "-rk-order",
    "--rk-order",
    dest="rk_order",
    type=int,
    default=rk_order,
    help="Order of time discretisation for Runge-Kutta scheme.",
)
parser.add_argument(
    "-dg-order",
    "--dg-order",
    dest="dg_order",
    type=int,
    default=dg_order,
    help="Order of space discretisation for Discontinuous Galerkin.",
)

parser.add_argument(
    "--trees",
    dest="trees",
    type=int,
    default=0,
    help="Number of trees (subpartitions) per rank.",
)
parser.add_argument(
    "--threads", dest="threads", type=int, default=0, help="Number of threads per rank."
)
parser.add_argument(
    "--timeout", dest="timeout", type=int, default=3600, help="MPI timeout in seconds."
)
parser.add_argument(
    "-fpe",
    "--fp-exception",
    dest="enable_fpe",
    action="store_true",
    help="Enable a floating-point exception handler",
)

parser.add_argument(
    "--no-make",
    dest="no_make",
    action="store_true",
    help="Do not compile the code after generation",
)
parser.add_argument(
    "--executable",
    dest="executable",
    type=str,
    help="Specify the executable name. If none is chosen, the name is automatically deduced by given parameters.",
)
args = parser.parse_args()

if args.dim not in [2, 3]:
    print("Error, dimension must be 2 or 3, you supplied {}".format(args.dim))
    sys.exit(1)

if args.build_mode not in build_modes:
    print(
        "Error, mode must be {} or {}, you supplied {}".format(
            ", ".join(list(build_modes.keys())[:-1]),
            list(build_modes.keys())[-1],
            args.build_mode,
        )
    )
    sys.exit(1)

if args.solver not in solvers:
    print(
        "Error, solver must be {} or {}, you supplied {}".format(
            ", ".join(list(solvers.keys())[:-1]), list(solvers.keys())[-1], args.solver
        )
    )
    sys.exit(1)

if args.storage not in storage_type:
    print(
        "Error, storage type must be {} or {}, you supplied {}".format(
            ", ".join(list(storage_type.keys())[:-1]),
            list(storage_type.keys())[-1],
            args.storage,
        )
    )
    sys.exit(1)

if args.executable:
    executable_name = args.executable
else:
    executable_name = "advection-" + str(args.dim) + "d-" + str(args.solver)
    if args.use_stateless_pde_terms:
        executable_name += "-Stateless"
    executable_name += "-" + str(args.build_mode)

my_project = exahype2.Project(
    namespace=["applications", "exahype2", "advection"],
    project_name="Advection",
    directory=".",
    executable=executable_name,
)

if args.solver in aderdg_rusanov_solvers:
    solver_name = "Advection_ADERDG"
elif args.solver in rkdg_rusanov_solvers:
    solver_name = "Advection_RKDG"
elif args.solver in fv_rusanov_solvers:
    solver_name = "Advection_FV"

unknowns = {"v": args.dim}

my_solver = None
if args.solver in fv_rusanov_solvers and "Adaptive" in args.solver:
    my_solver = solvers[args.solver](
        name=solver_name,
        patch_size=args.patch_size,
        unknowns=unknowns,
        auxiliary_variables=0,
        max_volume_h=args.max_cell_size,
        min_volume_h=args.max_cell_size * 3.0 ** (-args.amr_levels),
        time_step_relaxation=0.5,
        pde_terms_without_state=args.use_stateless_pde_terms
    )
    # self._patch.generator = peano4.datamodel.PatchToDoubleArrayOnHeap(self._patch,"double")
    my_solver.switch_storage_scheme(
        cell_data_storage=storage_types[args.storage],
        face_data_storage=storage_types[args.storage],
    )

    # from exahype2.solvers.fv.rusanov.kernels import create_compute_Riemann_kernel_for_Rusanov
    # my_solver._fused_compute_kernel_call_cpu = create_compute_Riemann_kernel_for_Rusanov(
    #  my_solver._flux_implementation,
    #  my_solver._ncp_implementation,
    #  my_solver._source_term_implementation,
    #  compute_max_eigenvalue_of_next_time_step=True,
    #  solver_variant = exahype2.solvers.fv.rusanov.SolverVariant.Multicore,
    #  kernel_variant = exahype2.solvers.fv.rusanov.KernelVariant.BatchedAoSHeap
    # )
elif args.solver in fv_rusanov_solvers and "Fixed" in args.solver:
    min_h = args.max_cell_size * 3.0 ** (-args.amr_levels)
    admissible_time_step_size = min_h / patch_size * 0.5
    my_solver = solvers[args.solver](
        name=solver_name,
        patch_size=args.patch_size,
        unknowns=unknowns,
        auxiliary_variables=0,
        max_volume_h=args.max_cell_size,
        min_volume_h=min_h,
        normalised_time_step_size=admissible_time_step_size,
        pde_terms_without_state=args.use_stateless_pde_terms
    )
elif args.solver in rkdg_rusanov_solvers and "Adaptive" in args.solver:
    from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import (
        FaceProjections,
    )

    my_solver = solvers[args.solver](
        name=solver_name,
        rk_order=args.rk_order,
        polynomials=exahype2.solvers.GaussLegendreBasis(args.dg_order),
        face_projections=FaceProjections.Solution,
        unknowns=unknowns,
        auxiliary_variables=0,
        max_cell_h=args.max_cell_size,
        min_cell_h=args.max_cell_size * 3.0 ** (-args.amr_levels),
        time_step_relaxation=0.5,
        pde_terms_without_state=args.use_stateless_pde_terms
    )
elif args.solver in rkdg_rusanov_solvers and "Fixed" in args.solver:
    from exahype2.solvers.rkdg.actionsets.ProjectLinearCombinationOfEstimatesOntoFaces import (
        FaceProjections,
    )

    min_h = args.max_cell_size * 3.0 ** (-args.amr_levels)
    admissible_time_step_size = min_h / patch_size * 0.5
    my_solver = solvers[args.solver](
        name=solver_name,
        rk_order=args.rk_order,
        polynomials=exahype2.solvers.GaussLegendreBasis(args.dg_order),
        face_projections=FaceProjections.Solution,
        unknowns=unknowns,
        auxiliary_variables=0,
        max_cell_h=args.max_cell_size,
        min_cell_h=min_h,
        time_step_size=admissible_time_step_size,
        pde_terms_without_state=args.use_stateless_pde_terms
    )
elif args.solver in aderdg_rusanov_solvers and "Fixed" in args.solver:
    # stability condition: CFL for Ader-DG
    # dt <= c*dx / (lambda_max*(2N+1)
    # cflAder = [ 1.0, 0.33, 0.17, 0.1, 0.069, 0.045, 0.038, 0.03, 0.02, 0.015]
    # here lambda_max = 1
    # dt <= 0.1 * 0.03 / 7 \approx  0.0004
    # dt <= 0.1 * 0.03 / 11 \approx  0.00025
    min_h = args.max_cell_size * 3.0 ** (-args.amr_levels)
    admissible_time_step_size = 0.001
    my_solver = solvers[args.solver](
        name=solver_name,
        order=args.dg_order,
        unknowns=unknowns,
        auxiliary_variables=0,
        max_cell_h=args.max_cell_size,
        min_cell_h=min_h,
        time_step_size=admissible_time_step_size,
    )
    my_solver.add_kernel_optimizations(
        is_linear=True, use_kernel_generator=True, architecture="noarch"
    )
elif args.solver in aderdg_rusanov_solvers and "Adaptive" in args.solver:
    my_solver = solvers[args.solver](
        name=solver_name,
        order=args.dg_order,
        unknowns=unknowns,
        auxiliary_variables=0,
        max_cell_h=args.max_cell_size,
        min_cell_h=args.max_cell_size * 3.0 ** (-args.amr_levels),
        time_step_relaxation=0.5,
    )
    my_solver.add_kernel_optimizations(
        is_linear=True, use_kernel_generator=True, architecture="noarch"
    )

my_solver.set_implementation(
    initial_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    boundary_conditions=exahype2.solvers.PDETerms.User_Defined_Implementation,
    flux="::applications::exahype2::advection::flux(Q, normal, F);",
    eigenvalues="return ::applications::exahype2::advection::maxEigenvalue();",
    refinement_criterion=exahype2.solvers.PDETerms.User_Defined_Implementation,
    source_term=exahype2.solvers.PDETerms.None_Implementation,
    ncp=exahype2.solvers.PDETerms.None_Implementation,
)

if args.dim == 2:
    my_solver.plot_description = "u, v"
else:
    my_solver.plot_description = "u, v, w"

my_solver.add_user_solver_includes(
    """
#include "AdvectionKernels.h"
"""
)

my_project.add_solver(my_solver)

if args.periodic_boundary_conditions:
    periodic_BC = [True, True, True]
else:
    periodic_BC = [False, False, False]

my_project.set_global_simulation_parameters(
    dimensions=args.dim,
    size=[1.0, 1.0, 1.0][0 : args.dim],
    offset=[0.0, 0.0, 0.0][0 : args.dim],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=args.plot_interval,
    periodic_BC=periodic_BC,
)

solution_dir = str(args.output_path) + "-" + str(executable_name)
my_project.set_output_path(solution_dir)
if not os.path.exists(solution_dir):
    os.makedirs(solution_dir)

if args.visualise:
    output_patch_file = (
        solution_dir + "/solution-" + str(solver_name) + ".peano-patch-file"
    )
    if os.path.isfile(output_patch_file):
        subprocess.call(
            [
                "python3",
                "../../../python/peano4/visualisation/render.py",
                str(output_patch_file),
            ]
        )
    sys.exit(0)

if args.trees > 0:
    my_project.set_load_balancing(
        "toolbox::loadbalancing::strategies::SpreadOutHierarchically",
        "new ::exahype2::LoadBalancingConfiguration(0.98, 100, "
        + str(args.trees)
        + ")",
    )
else:
    my_project.set_load_balancing(
        "toolbox::loadbalancing::strategies::SpreadOutHierarchically",
        "new ::exahype2::LoadBalancingConfiguration(0.98)",
    )
if args.threads > 0:
    my_project.set_number_of_threads(args.threads)
else:
    my_project.set_number_of_threads(
        "tarch::multicore::Core::UseDefaultNumberOfThreads"
    )

my_project.set_timeout(args.timeout)

my_project.set_multicore_orchestration(
    "::tarch::multicore::orchestration::Hardcoded::createFuseAll(%s, true, true, ::tarch::accelerator::Device::getInstance().getLocalDeviceId())"
    % str(args.fused_tasks)
)

my_project.set_Peano4_installation("../../../", mode=build_modes[args.build_mode])
my_project = my_project.generate_Peano4_project(verbose=True)
if args.enable_fpe:
    my_project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")

my_project.constants.define_value("ADVECTION_SPEED", str(advection_speed))
my_project.constants.export_boolean("UseDynamicAMR", args.dynamic_amr)

my_project.output.makefile.add_cpp_file(solver_name + ".cpp")

my_project.build(
    make=not args.no_make, make_clean_first=True, throw_away_data_after_build=True
)

print("Executable is ", executable_name)
print("Clean object files via 'make clean'")
print("Recompile the generated code via 'make -j'")
print("Remove all generated code via 'make distclean'")
print("Regenerate all code by running 'python3 advection.py' again")
print(
    "Generate (and convert) any postprocessing data by calling 'python3 advection.py --visualise'"
)
