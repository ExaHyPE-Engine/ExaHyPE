# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import argparse

from dataclasses import dataclass

from single_black_hole_settings import *


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
        "--scenario",
        dest="scenario",
        type=int,
        help="Scenario number",
    )
    parser.add_argument(
        "-p",
        "--plot",
        dest="plot_interval",
        type=float,
        required=True,
        help="Plot solution every value time intervals",
    )

    args = parser.parse_args()
    return args


#! [Single black hole heuristics]
@dataclass
class Heuristics:
    min_tasks_to_fuse = 16
    max_tasks_to_fuse = 1024
    scheduler = SchedulerChoices_ProducerConsumerTaskSequence
    max_trees_per_rank = 32 
#! [Single black hole heuristics]


    

if __name__ == "__main__":
    args = get_command_line_args()
    
    ########################################################################################
    # Determine simulation settings
    ########################################################################################
    heuristics = Heuristics()
    if args.scenario==1: #here is the "standard" production run setup
        #! [Single black hole scenario 1]
        max_cell_size = 0.4*6
        min_cell_size = 0.04*6
        offset = [-9, -9, -9]
        domain_size   = [18, 18, 18]
        fd4_patch_size = 6
        add_limiter = False
        refinement_criterion_call = """return refinementCriterionScenario1(meshCellCentre, meshCellH);"""
        #! [Single black hole scenario 1]
    elif args.scenario==2: #here is a bigger production run setup
        #! [Single black hole scenario 2]
        max_cell_size = 0.7*6
        min_cell_size = 0.03*6
        offset = [-18, -18, -18]
        domain_size   = [36, 36, 36]
        fd4_patch_size = 6
        add_limiter = False
        refinement_criterion_call = """return refinementCriterionScenario2(meshCellCentre, meshCellH);"""
        #! [Single black hole scenario 2]
    elif args.scenario==3: 
        #! [Single black hole scenario 3]
        #here is a much bigger setup which we shall be able to simulate for gravitational wave
        # extraction in the future
        max_cell_size = 6.1*6
        min_cell_size = 0.03*6
        offset = [-162, -162, -162]
        domain_size   = [324, 324, 324]
        fd4_patch_size = 6
        add_limiter = False
        refinement_criterion_call = """return refinementCriterionScenario3(meshCellCentre, meshCellH);"""
        #! [Single black hole scenario 3]

    # =================================
    # Create project
    # =================================
    project = create_exahype2_project( 
        executable    = args.output,
        offset        = offset,
        domain_size   = domain_size,
        end_time      = args.end_time,
        plot_interval = args.plot_interval,
        peano_dir     = args.peanodir, 
        mode          = "Release"
        )

    ########################################################################################
    # Configure run
    ########################################################################################
    configure_multicore_configuration(project,
                                      heuristics.scheduler,
                                      heuristics.min_tasks_to_fuse,
                                      heuristics.max_tasks_to_fuse,
                                      )

    ########################################################################################
    # Configure solvers
    ########################################################################################
    my_primary_solver = FD4SolverWithoutLimiter(
            name="CCZ4SBH",
            patch_size = fd4_patch_size,
            min_cell_size=min_cell_size,
            max_cell_size=max_cell_size
        )
    exahype2.solvers.rkfd.fd4.switch_to_FD4_second_order_interpolation( my_primary_solver ) 
    exahype2.solvers.rkfd.fd4.switch_to_FD4_second_order_restriction( my_primary_solver ) 
    
    my_primary_solver.set_implementation(
      refinement_criterion=refinement_criterion_call,
    )
    
    
    project.add_solver(my_primary_solver)

    if add_limiter:
        my_secondary_solver = Limiter(
            name="CCZ4SBH",
            patch_size=FD4PatchSize,
            amend_priorities=enable_higher_priority_for_FV_tasks,
            parallelisation_of_kernels=fv_kernel_parallelisation,
        )
        project.add_solver(my_secondary_solver)

    add_generic_load_balancing(project, heuristics.max_trees_per_rank)

    peano4_project = lower_to_peano4_project(project,
                                             args.verbose,
                                             add_fv = add_limiter,
                                             add_fd4 = True,
                                             )
    
    peano4_project.generate(throw_away_data_after_generation=False)
    peano4_project.build(
        make=True, make_clean_first=True, number_of_parallel_builds=args.j
    )
