# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import argparse
import peano4
import exahype2
import dastgen2
import math

from SBH import FD4SolverWithoutLimiter

#![tutorials_exahype2_hpc_tutorial_offline_load_balancing_inputs]
from toolbox.loadbalancing.api.input.RegularGridGenerator import RegularGridGenerator
import toolbox.loadbalancing.api.Strategies.SplitTrees as SplitTrees
#![tutorials_exahype2_hpc_tutorial_offline_load_balancing_inputs]


Offset = [-9, -9, -9]
Domain_size = [18, 18, 18]


LoadBalancingMetrics_None                      = "none"
LoadBalancingMetrics_Offline_Regular_Fine_Grid = "offline-regular-fine-grid"
LoadBalancingMetrics = [LoadBalancingMetrics_None, 
                        LoadBalancingMetrics_Offline_Regular_Fine_Grid
                        ]


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
        default="../../..",
        help="Peano4 directory",
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
    parser.add_argument(
        "-amr",
        "--amr",
        dest="amr",
        action="store_true",
        default=False,
        help="Enable AMR",
    )
    parser.add_argument(
        "-lbm",
        "--load-balancing-metric",
        dest="load_balancing_metric",
        choices=LoadBalancingMetrics,
        default=LoadBalancingMetrics[0],
        help="Load balancing metric",
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
    
    end_time = 1e-2
    
    # =================================
    # Create project
    # =================================
    project = exahype2.Project(
        ["benchmarks", "exahype2", "ccz4"], # we hijack the benchmark
        "offline_load_balancing",
        executable="offline-lb-tutorial",
    )

    project.set_global_simulation_parameters(
        3,  # dimensions
        Offset,
        Domain_size,
        end_time,
        0.0,
        end_time, # plot interval
        [False, False, False],
        8,  # plotter precision
    )

    project.set_output_path("./")
 
    project.set_Peano4_installation(args.peanodir, peano4.output.string_to_mode(peano4.output.CompileModes[0]))

    if args.amr:
        max_cell_size = Domain_size[0] / 3.0
        min_cell_size = args.cell_size
    else:
        max_cell_size = args.cell_size
        min_cell_size = args.cell_size

    ########################################################################################
    # Configure run
    ########################################################################################
    project.add_mainfile_include( "tarch/multicore/orchestration/Hardcoded.h")
    project.set_multicore_orchestration(
            "tarch::multicore::orchestration::Hardcoded::createNative()"
    )

    ########################################################################################
    # Configure solvers
    ########################################################################################
    FD4PatchSize = 3
    my_primary_solver = FD4SolverWithoutLimiter(
        name="OfflineLoadBalancingTutorial_CCZ4SBH",
        patch_size=FD4PatchSize,
        min_cell_size=min_cell_size,
        max_cell_size=max_cell_size#,
        #production_run=args.production_run #send the corresponding flag to cpp level for refinement control
    )
    project.add_solver(my_primary_solver)
    my_primary_solver.set_implementation(
        refinement_criterion="""return artificialRefinement(meshCellCentre, meshCellH);""",
    )
    my_primary_solver.add_user_solver_includes( """#include "SingleBlackHoleScenario.h" """)
    
    # probe_point = [-12,-12,0.0]
    # project.add_plot_filter( probe_point,[24.0,24.0,0.01],1 )
    # my_solver.select_dofs_to_print = [0,12,16,17,18,54,59,60]
    
    #![tutorials_exahype2_hpc_tutorial_offline_load_balancing_none]
    if args.load_balancing_metric==LoadBalancingMetrics_None:
        pass
    #![tutorials_exahype2_hpc_tutorial_offline_load_balancing_none]
    elif args.load_balancing_metric==LoadBalancingMetrics_Offline_Regular_Fine_Grid:
        #![tutorials_exahype2_hpc_tutorial_offline_load_balancing_regular_fine_grid]
        tree_height_required_to_match_cell_size = math.ceil( math.log( Domain_size[0] / args.cell_size, 3.0 ) )
        
        tree = RegularGridGenerator(max_level=tree_height_required_to_match_cell_size, dimensions=3)
        print(tree)
        #![tutorials_exahype2_hpc_tutorial_offline_load_balancing_regular_fine_grid]
        
        #![tutorials_exahype2_hpc_tutorial_offline_load_balancing_regular_fine_grid_split]
        number_of_subtrees = args.ranks * args.trees

        SplitTrees.distribute_leaves_bottom_up(tree, number_of_subtrees=number_of_subtrees)
        # @todo This is a bug: This line should really be inside distribute_leaves_dfs and not external. It should be removed from here
        tree._subtree_count = number_of_subtrees
        grid_sweeps, splitting_trees, number_of_cells, subtrees = SplitTrees.extract_split_instructions(tree)
        
        print(tree)

        print("Extracted hardcoded parameters:")
        print("  grid_sweeps     =", grid_sweeps)
        print("  splitting_trees =", splitting_trees)
        print("  number_of_cells =", number_of_cells)
        print("  subtrees       =", subtrees)        
        #![tutorials_exahype2_hpc_tutorial_offline_load_balancing_regular_fine_grid_split]
        
        
        #![tutorials_exahype2_hpc_tutorial_offline_load_balancing_regular_fine_grid_map]
        destination_ranks = [
            tree_number % args.ranks for tree_number in subtrees
        ]
        print("  destination_ranks  =", destination_ranks)
        
        hardcoded_mode = "peano4::SplitInstruction::Mode::AggressiveTopDown"
        hardcoded_config = (
            "{"
            f"{{{', '.join(map(str, grid_sweeps))}}}, "
            f"{{{', '.join(map(str, splitting_trees))}}}, "
            f"{{{', '.join(map(str, number_of_cells))}}}, "
            f"{{{', '.join(map(str, destination_ranks))}}}, "
            f"{hardcoded_mode}"
            "}"
        )
        
        print( "Arguments to hardcoded strategy: ")
        print( hardcoded_config )
        
        project.set_load_balancing(
            "toolbox::loadbalancing::strategies::Hardcoded",
            hardcoded_config
        )
        #![tutorials_exahype2_hpc_tutorial_offline_load_balancing_regular_fine_grid_map]
        pass
    else:
        assert False, "should not be entered"
        

    peano4_project = project.generate_Peano4_project(True)

    project._solvers[0].add_makefile_parameters(
        peano4_project, args.peanodir + "/applications/exahype2/ccz4"
    )

    peano4_project.output.makefile.add_linker_flag("-lm -lgsl -lgslcblas")
    peano4_project.output.makefile.add_cpp_file(
        args.peanodir + "/applications/exahype2/ccz4/libtwopunctures/TP_Utilities.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        args.peanodir + "/applications/exahype2/ccz4/libtwopunctures/TP_Parameters.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        args.peanodir + "/applications/exahype2/ccz4/libtwopunctures/TP_Logging.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        args.peanodir + "/applications/exahype2/ccz4/libtwopunctures/TwoPunctures.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        args.peanodir + "/applications/exahype2/ccz4/libtwopunctures/CoordTransf.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        args.peanodir + "/applications/exahype2/ccz4/libtwopunctures/Equations.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        args.peanodir + "/applications/exahype2/ccz4/libtwopunctures/FuncAndJacobian.cpp"
    )
    peano4_project.output.makefile.add_cpp_file(
        args.peanodir + "/applications/exahype2/ccz4/libtwopunctures/Newton.cpp"
    )

    peano4_project.output.makefile.add_cpp_file(args.peanodir + "/benchmarks/exahype2/ccz4/single-black-hole/SingleBlackHoleScenario.cpp")
    peano4_project.output.makefile.add_header_search_path(args.peanodir + "/benchmarks/exahype2/ccz4/single-black-hole")

    peano4_project.output.makefile.add_CXX_flag("-DIncludeTwoPunctures")

    peano4_project.output.makefile.add_CXX_flag("-DPureFD4")

    
    peano4_project.generate(throw_away_data_after_generation=False)
#    peano4_project.build(
#        make=True, make_clean_first=True, number_of_parallel_builds=args.j
#    )
 