import os
import argparse

import peano4
import exahype2
import dastgen2

import peano4.toolbox.particles
import numpy as np

import jinja2

from SBH import KernelParallelisation
from SBH import Limiter
from SBH import FD4SolverWithLimiter
from SBH import FD4SolverWithoutLimiter

# See comments in README.dox
# export PYTHONPATH=$PYTHONPATH:~/Peano/applications/exahype2/ccz4
from CCZ4Solver import CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import (
    CCZ4Solver_FD4_SecondOrderFormulation_GlobalAdaptiveTimeStepWithEnclaveTasking,
)

from CCZ4Solver import CCZ4Solver_FV_GlobalAdaptiveTimeStep
from CCZ4Solver import CCZ4Solver_FD4_GlobalAdaptiveTimeStep


if __name__ == "__main__":
    parser = argparse.ArgumentParser(
        description="ExaHyPE 2 - CCZ4-GaugeWave benchmarking script"
    )
    parser.add_argument(
        "-j",
        "--parallel-builds",
        dest="j",
        type=int,
        default=-1,
        help="Parallel builds",
    )
    parser.add_argument(
        "-pd",
        "--peano-dir",
        dest="peanodir",
        default="../../../../",
        help="Peano4 directory",
    )
    parser.add_argument(
        "-o",
        "--output",
        dest="output",
        default="peano_sbh",
        help="Executable name (output)",
    )
    parser.add_argument(
        "-v",
        "--verbose",
        dest="verbose",
        action="store_true",
        default=False,
        help="Verbose",
    )
    parser.add_argument(
        "-et",
        "--end-time",
        dest="end_time",
        type=float,
        default=0.01,
        help="End of simulation",
    )
    parser.add_argument(
        "--trees",
        dest="trees",
        type=int,
        required=True,
        help="Number of trees (subpartitions) per rank",
    )
    parser.add_argument(
        "-cs",
        "--cell-size",
        dest="cell_size",
        type=float,
        default=1.8,
        help="Minimal cell size (AMR) or regular mesh cell size (without AMR), default=1.8",
    )
    parser.add_argument(
        "-asserts",
        "--asserts",
        dest="asserts",
        action="store_true",
        default=False,
        help="Switch on assertions",
    )
    parser.add_argument(
        "-amr",
        "--amr",
        dest="amr",
        action="store_true",
        default=False,
        help="Enable AMR",
    )
    parser.add_argument(
        "-p",
        "--plot",
        dest="plot",
        action="store_true",
        default=False,
        help="Plot initial and final solution",
    )
    parser.add_argument(
        "-sched",
        "--scheduler",
        dest="scheduler",
        choices=[
            "native-no-priorities",
            "native",
            "tailored",
            "parallel-for",
            "subtasks",
            "subtasks-and-kernel-parallelisation"
        ],
        required=True,
        help="Task scheduler flavours",
    )
    parser.add_argument(
        "-lbm",
        "--load-balancing-metric",
        dest="load_balancing_metric",
        choices=[
            "generic",
            "tailored"
        ],
        required=True,
        help="Load balancing metric",
    )
    parser.add_argument(
        "-s",
        "--solver",
        dest="solver",
        choices=[
            "fd4-rk1-ps-3",
            "fd4-rk1-ps-6",
            "fd4-rk1-limited-ps-3",
            "fd4-rk1-limited-ps-6",
        ],
        required=True,
        help="Pick solver type",
    )

    args = parser.parse_args()

    
    #
    # Start of real ExaHyPE 2 script
    #
    project = exahype2.Project(
        ["benchmarks", "exahype2", "ccz4"],
        "ccz4",
        executable=args.output,
    )

    dimensions = 3
    offset = [-9, -9, -9]
    domain_size = [18, 18, 18]

    if args.amr:
      max_cell_size = domain_size[0]/3.0
    else:
      max_cell_size = args.cell_size
       
    ########################################################################################
    # Configure run
    ########################################################################################
    if args.scheduler == "native-no-priorities":
        project.set_load_balancing( "tarch::multicore::orchestration::Hardcoded::createNative()" )
        enable_higher_priority_for_FV_tasks = False
        parallelise_interpolation           = KernelParallelisation.NONE
        fv_kernel_parallelisation           = KernelParallelisation.NONE
    elif args.scheduler == "native":
        project.set_load_balancing( "tarch::multicore::orchestration::Hardcoded::createNative()" )
        enable_higher_priority_for_FV_tasks = True
        parallelise_interpolation           = KernelParallelisation.NONE
        fv_kernel_parallelisation           = KernelParallelisation.NONE
    elif args.scheduler == "tailored":
        project.set_load_balancing( "new benchmarks::exahype2::ccz4::MulticoreOrchestration()" )
        enable_higher_priority_for_FV_tasks = True
        parallelise_interpolation           = KernelParallelisation.NONE
        fv_kernel_parallelisation           = KernelParallelisation.NONE
    elif args.scheduler == "parallel-for":
        project.set_load_balancing( "new benchmarks::exahype2::ccz4::MulticoreOrchestration()" )
        enable_higher_priority_for_FV_tasks = True
        parallelise_interpolation           = KernelParallelisation.PARALLEL_FOR
        fv_kernel_parallelisation           = KernelParallelisation.NONE
    elif args.scheduler == "subtasks":
        project.set_load_balancing( "new benchmarks::exahype2::ccz4::MulticoreOrchestration()" )
        enable_higher_priority_for_FV_tasks = True
        parallelise_interpolation           = KernelParallelisation.SUBTASKS
        fv_kernel_parallelisation           = KernelParallelisation.NONE
    elif args.scheduler == "subtasks-and-kernel-parallelisation":
        project.set_load_balancing( "new benchmarks::exahype2::ccz4::MulticoreOrchestration()" )
        enable_higher_priority_for_FV_tasks = True
        parallelise_interpolation           = KernelParallelisation.SUBTASKS
        fv_kernel_parallelisation           = KernelParallelisation.SUBTASKS
    else:
        assert False, "not supported yet"


      
    added_limiter = False
    if "fd4-rk1-ps-" in args.solver:
        FD4PatchSize = int(args.solver[-1])
        my_primary_solver = FD4SolverWithoutLimiter(name          = "CCZ4SBH",
                                                    patch_size    = FD4PatchSize,
                                                    min_cell_size = args.cell_size, 
                                                    max_cell_size = max_cell_size,
                                                    parallelisation_of_interpolation = parallelise_interpolation,
                                                    parallelisation_of_kernels       = fv_kernel_parallelisation
                                                    )
        project.add_solver(my_primary_solver)
        added_limiter = False
    elif "fd4-rk1-limited-ps-" in args.solver:
        FD4PatchSize = int(args.solver[-1])
        my_primary_solver = FD4SolverWithLimiter(name          = "CCZ4SBH",
                                                 patch_size    = FD4PatchSize,
                                                 min_cell_size = args.cell_size, 
                                                 max_cell_size = max_cell_size,
                                                 parallelisation_of_interpolation = parallelise_interpolation,
                                                 parallelisation_of_kernels       = fv_kernel_parallelisation
                                                 )
        
        my_secondary_solver = Limiter(name                      = "CCZ4SBH",
                                      patch_size                = FD4PatchSize,
                                      amend_priorities          = enable_higher_priority_for_FV_tasks,
                                      parallelisation_of_kernels       = fv_kernel_parallelisation
                                      )
        project.add_solver(my_primary_solver)
        project.add_solver(my_secondary_solver)
        added_limiter = True
    else:
        raise "unknown solver type {}".format(args.solver)


      
    ########################################################################################
    # Build the project
    ########################################################################################

    if args.asserts:
        build_mode = peano4.output.CompileMode.Asserts
    else:
        build_mode = peano4.output.CompileMode.Release

    if args.plot:
      plot_interval = args.end_time
    else:
      plot_interval = 0.0

    project.set_global_simulation_parameters(
        dimensions,  # dimensions
        offset,
        domain_size,
        args.end_time,
        0.0,
        plot_interval,
        [False, False, False],
        8,  # plotter precision
    )

    project.set_output_path("./")
    # probe_point = [-12,-12,0.0]
    # project.add_plot_filter( probe_point,[24.0,24.0,0.01],1 )
    # my_solver.select_dofs_to_print = [0,12,16,17,18,54,59,60]

    project.set_Peano4_installation(args.peanodir, build_mode)

    
    if args.load_balancing_metric=="generic":
      project.set_load_balancing(
        "toolbox::loadbalancing::strategies::cascade::SpreadOut_SplitOversizedTree",
        "new ::exahype2::LoadBalancingConfiguration(0.98, 1, " + str(args.trees) + ",::exahype2::LoadBalancingConfiguration::UseNumberOfThreads), new toolbox::loadbalancing::metrics::CellCount()",
        )
    else:
      load_balancing_cell_label = exahype2.grid.FineGridCellLoadBalancingCostMetric.create_cell_label(project)
      load_balancing_action_set = exahype2.grid.FineGridCellLoadBalancingCostMetric(load_balancing_cell_label,
                                                                                    "1.0 / (1.0+tarch::la::norm2(marker.x()))")
      project.add_action_set_to_create_grid( load_balancing_action_set )
      project.set_load_balancing(
        "toolbox::loadbalancing::strategies::cascade::SpreadOut_SplitOversizedTree",
        "new ::exahype2::LoadBalancingConfiguration(0.98, 1, " + str(args.trees) + ",::exahype2::LoadBalancingConfiguration::UseNumberOfThreads), new toolbox::loadbalancing::metrics::CustomCellWeight()",
        )

    peano4_project = project.generate_Peano4_project(verbose=args.verbose)

    my_primary_solver.add_makefile_parameters(
        peano4_project, "../../../../applications/exahype2/ccz4"
    )

    peano4_project.output.makefile.add_linker_flag(
        "-lm -L/cosma/local/gsl/2.4/lib -lgsl -L/cosma/local/gsl/2.4/lib -lgslcblas"
    )
    peano4_project.output.makefile.add_cpp_file(
        "../../../../applications/exahype2/ccz4/libtwopunctures/TP_Utilities.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        "../../../../applications/exahype2/ccz4/libtwopunctures/TP_Parameters.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        "../../../../applications/exahype2/ccz4/libtwopunctures/TP_Logging.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        "../../../../applications/exahype2/ccz4/libtwopunctures/TwoPunctures.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        "../../../../applications/exahype2/ccz4/libtwopunctures/CoordTransf.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        "../../../../applications/exahype2/ccz4/libtwopunctures/Equations.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        "../../../../applications/exahype2/ccz4/libtwopunctures/FuncAndJacobian.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        "../../../../applications/exahype2/ccz4/libtwopunctures/Newton.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        "MulticoreOrchestration.cpp"
    )
    peano4_project.output.makefile.add_CXX_flag("-DIncludeTwoPunctures")
    if added_limiter:
        peano4_project.output.makefile.add_CXX_flag("-DCoupleWithFV")

    peano4_project.generate(throw_away_data_after_generation=False)
    peano4_project.build(make_clean_first=True, number_of_parallel_builds=args.j)
