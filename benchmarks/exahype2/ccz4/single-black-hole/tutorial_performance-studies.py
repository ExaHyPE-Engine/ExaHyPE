# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import argparse


from tutorial_single_black_hole_settings import *


# Offset = [-9, -9, -9]
# Domain_size = [18, 18, 18]
Offset = [0, 0, 0]
Domain_size = [1, 1, 1]


def get_command_line_args():
    parser = argparse.ArgumentParser(
        description="ExaHyPE 2 - CCZ4 single black hole benchmarking script"
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
        required=True,
        help="End of simulation",
    )
    parser.add_argument(
        "--trees",
        dest="trees",
        type=int,
        default=0,
        help="Number of trees (subpartitions) per rank",
    )
    parser.add_argument(
        "--ranks",
        dest="ranks",
        type=int,
        default=1,
        help="Total number of MPI ranks used for offline hardcoded decomposition",
    )
    parser.add_argument(
        "-cs",
        "--cell-size",
        dest="cell_size",
        type=float,
        required=True,
        help="Minimal cell size (AMR) or regular mesh cell size (without AMR), default=1.8",
    )
    parser.add_argument("-m",   "--mode",               dest="mode",           choices=peano4.output.CompileModes, default=peano4.output.CompileModes[0], help="|".join(peano4.output.CompileModes) )
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
        dest="plot_interval",
        type=float,
        help="Plot solution every value time intervals",
    )
    parser.add_argument(
        "-sched",
        "--scheduler",
        dest="scheduler",
        choices=SchedulerChoices,
        default=SchedulerChoices[0],
        help="Task scheduler flavours",
    )
    parser.add_argument(
        "-lbm",
        "--load-balancing-metric",
        dest="load_balancing_metric",
        choices=LoadBalancingMetrics,
        default=LoadBalancingMetrics[0],
        help="Load balancing metric",
    )
    parser.add_argument(
        "-s",
        "--solver",
        dest="solver",
        choices=SolverChoices,
        required=True,
        help="Pick solver type",
    )
    parser.add_argument(
        "--no-make",
        dest="no_make",
        action="store_true",
        help="Do not compile the code after generation",
    )
    parser.add_argument(
        "--interpolation-method",
        dest="interpolation_method",
        choices=["linear", "matrix", "secondOrder"],
        default="linear",
        help="Set method to interpolate between solvers",
    )
    parser.add_argument(
        "-min-tasks",
        "--min-tasks",
        type=int,
        default=-1,
        help="minimum number of tasks that are fused together using the hardcoded fusion implementation"
    )
    parser.add_argument(
        "-max-tasks",
        "--max-tasks",
        type=int,
        default=-1,
        help="maximum number of tasks that are fused together using the hardcoded fusion implementation"
    )

    args = parser.parse_args()

    if args.ranks < 1:
        parser.error("--ranks must be >= 1")
    if args.trees < 0:
        parser.error("--trees must be >= 0")
    if args.ranks > 1 and args.trees < 1:
        parser.error(
            "Unsupported combination: --ranks > 1 requires --trees >= 1 "
            "for offline decomposition."
        )

    return args


           
               
if __name__ == "__main__":
    args = get_command_line_args()
    
    # =================================
    # Create project
    # =================================
    project = create_exahype2_project( 
        executable    = args.output,
        offset        = Offset,
        domain_size   = Domain_size,
        end_time      = args.end_time,
        plot_interval = args.plot_interval,
        peano_dir     = args.peanodir, 
        mode          = args.mode,
        )

    if args.amr:
        max_cell_size = Domain_size[0] / 3.0
        min_cell_size = args.cell_size
    else:
        max_cell_size = args.cell_size
        min_cell_size = args.cell_size

    ########################################################################################
    # Configure run
    ########################################################################################
    configure_multicore_configuration(project,
                                      args.scheduler,
                                      args.min_tasks,
                                      args.max_tasks,
                                      )

    ########################################################################################
    # Configure solvers
    ########################################################################################
    added_limiter = False
    if "fd4-rk1-ps-" in args.solver:
        FD4PatchSize = int(args.solver[-1])
        my_primary_solver = FD4SolverWithoutLimiter(
            name="CCZ4SBH",
            patch_size=FD4PatchSize,
            min_cell_size=min_cell_size,
            max_cell_size=max_cell_size#,
            #production_run=args.production_run #send the corresponding flag to cpp level for refinement control
        )
        project.add_solver(my_primary_solver)
        added_limiter = False
        my_primary_solver.set_implementation(
          refinement_criterion="""return artificialRefinement(meshCellCentre, meshCellH);""",
        )
        my_primary_solver.add_user_solver_includes( """#include "SingleBlackHoleScenario.h" """)

    elif "fd4-rk1-limited-ps-" in args.solver:
        FD4PatchSize = 16*int(args.solver[-1])
        my_primary_solver = FD4SolverWithLimiter(
            name="CCZ4SBH",
            patch_size=FD4PatchSize,
            min_cell_size=min_cell_size,
            max_cell_size=max_cell_size,
            parallelisation_of_interpolation=parallelise_interpolation,
            parallelisation_of_kernels=fv_kernel_parallelisation,
            interpolation_method=args.interpolation_method,
        )
        my_primary_solver.set_implementation(
          refinement_criterion="""return artificialRefinement(x, h);""",
        )
        my_primary_solver.add_user_solver_includes( """#include "SingleBlackHoleScenario.h" """)

        my_secondary_solver = Limiter(
            name="CCZ4SBH",
            patch_size=FD4PatchSize,
            amend_priorities=enable_higher_priority_for_FV_tasks,
            parallelisation_of_kernels=fv_kernel_parallelisation,
        )
        project.add_solver(my_primary_solver)
        project.add_solver(my_secondary_solver)
        added_limiter = True
    elif (
      args.solver == SolverChoices_FiniteVolumes_PatchSize6
      or 
      args.solver == SolverChoices_FiniteVolumes_PatchSize48
      or 
      args.solver == SolverChoices_FiniteVolumes_PatchSize96
      ):
        if args.solver == SolverChoices_FiniteVolumes_PatchSize6:
            patch_size = 6
        if args.solver == SolverChoices_FiniteVolumes_PatchSize48:
            patch_size = 48
        if args.solver == SolverChoices_FiniteVolumes_PatchSize96:
            patch_size = 96
        my_primary_solver = FVSolver(
            name="CCZ4SBH",
            patch_size=patch_size,
            min_cell_size=args.cell_size,
            max_cell_size=max_cell_size,
        )
        my_primary_solver.set_implementation(
          refinement_criterion="""return artificialRefinement(volumeCentre, volumeH);""",
        )
        my_primary_solver.add_user_solver_includes( """#include "SingleBlackHoleScenario.h" """)

        project.add_solver(my_primary_solver)
        added_limiter = False
    else:
        raise "unknown solver type {}".format(args.solver)

    # probe_point = [-12,-12,0.0]
    # project.add_plot_filter( probe_point,[24.0,24.0,0.01],1 )
    # my_solver.select_dofs_to_print = [0,12,16,17,18,54,59,60]

    if args.trees <= 1 and args.ranks == 1:
        add_no_load_balancing(project)
    elif args.load_balancing_metric == LoadBalancingMetrics_Generic:
        add_generic_load_balancing(project, args.trees)
    elif args.load_balancing_metric == LoadBalancingMetrics_Tailored:
        add_load_balancing_with_cost_metric(project, args.trees)
    elif args.load_balancing_metric == LoadBalancingMetrics_Offline_Regular:
        add_hardcoded_load_balancing_over_regular_mesh(project, args.trees, args.ranks)
    else:
        assert False, "variant not supported"

    peano4_project = lower_to_peano4_project(project,
                                             args.verbose,
                                             add_fv = "fv"   in args.solver or added_limiter,
                                             add_fd4 = "fd4" in args.solver or added_limiter,
                                             )
    
    peano4_project.generate(throw_away_data_after_generation=False)
    peano4_project.build(
        make=not args.no_make, make_clean_first=True, number_of_parallel_builds=args.j
    )
