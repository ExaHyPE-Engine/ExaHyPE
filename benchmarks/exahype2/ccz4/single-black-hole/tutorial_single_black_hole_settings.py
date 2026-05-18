# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2
import dastgen2

#import peano4.toolbox.particles
#import numpy as np

#import jinja2



LoadBalancingMetrics_Generic  = "generic"
LoadBalancingMetrics_Tailored = "tailored"
LoadBalancingMetrics_Offline_Regular = "offline-regular"
LoadBalancingMetrics = [LoadBalancingMetrics_Generic, LoadBalancingMetrics_Tailored, LoadBalancingMetrics_Offline_Regular]

SchedulerChoices_Native                        = "native"
SchedulerChoices_NativePriorities              = "native-priorities"
SchedulerChoices_LazyEvaluation                = "gpu-lazy-evaluation"
SchedulerChoices_TimedExecution                = "gpu-timed-execution"
SchedulerChoices_ProducerConsumerTaskSequence  = "gpu-producer-consumer"
SchedulerChoices_EagerConsumerTasks            = "gpu-eager-consumer-tasks"
SchedulerChoices_NoTasks                       = "no-tasks"
SchedulerChoices = [
            SchedulerChoices_Native,
            SchedulerChoices_NativePriorities,
            SchedulerChoices_LazyEvaluation,
            SchedulerChoices_TimedExecution,
            SchedulerChoices_ProducerConsumerTaskSequence,
            SchedulerChoices_EagerConsumerTasks,
            SchedulerChoices_NoTasks
        ]


SolverChoices_FiniteVolumes_PatchSize6  = "fv-6"
SolverChoices_FiniteVolumes_PatchSize48 = "fv-48"
SolverChoices_FiniteVolumes_PatchSize96 = "fv-96"
SolverChoices = [
            SolverChoices_FiniteVolumes_PatchSize6,
            SolverChoices_FiniteVolumes_PatchSize48,
            SolverChoices_FiniteVolumes_PatchSize96,
            "fd4-rk1-ps-3",
            "fd4-rk1-ps-6",
            "fd4-rk1-ps-6-enclave",
            "fd4-rk1-limited-ps-3",
            "fd4-rk1-limited-ps-6",
        ] 


Dimensions = 3


from SBH import KernelParallelisation
from SBH import Limiter
from SBH import FD4SolverWithLimiter
from SBH import FD4SolverWithoutLimiter
from SBH import FVSolver

# See comments in README.dox
# export PYTHONPATH=../../../../python
# export PYTHONPATH=$PYTHONPATH:../../../../applications/exahype2/ccz4
from CCZ4Solver import CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import (
    CCZ4Solver_FD4_SecondOrderFormulation_GlobalAdaptiveTimeStepWithEnclaveTasking,
)

from CCZ4Solver import CCZ4Solver_FV_GlobalAdaptiveTimeStep
from CCZ4Solver import CCZ4Solver_FD4_GlobalAdaptiveTimeStep

def configure_multicore_configuration(project,
                                      scheduler,
                                      min_tasks,
                                      max_tasks,
                                      ):
    """!
    
    Add a proper scheduler to the project
    
    """
    if min_tasks == -1:
        task_fusion_size_string = ""
    elif max_tasks == -1:
        task_fusion_size_string = f", {min_tasks}"
    else:
        task_fusion_size_string = f", {min_tasks}, {max_tasks}"
         
    if scheduler == SchedulerChoices_Native:
        project.add_mainfile_include( "tarch/multicore/orchestration/Hardcoded.h")
        project.set_multicore_orchestration(
            "tarch::multicore::orchestration::Hardcoded::createNative()"
        )
    elif scheduler == SchedulerChoices_NativePriorities:
        project.add_mainfile_include( "MulticoreOrchestration.h")
        project.set_multicore_orchestration(
            "new MulticoreOrchestration(tarch::multicore::orchestration::FusionImplementation::DisableFusionAndMapOntoNativeTask)"
        )
    elif scheduler == SchedulerChoices_LazyEvaluation:
        project.add_mainfile_include( "MulticoreOrchestration.h")
        project.set_multicore_orchestration(
            "new MulticoreOrchestration(tarch::multicore::orchestration::FusionImplementation::LazyEvaluation" + task_fusion_size_string + ")"
        )
    elif scheduler == SchedulerChoices_TimedExecution:
        project.add_mainfile_include( "MulticoreOrchestration.h")
        project.set_multicore_orchestration(
            "new MulticoreOrchestration(tarch::multicore::orchestration::FusionImplementation::TimedExecution" + task_fusion_size_string + ")"
        )
    elif scheduler == SchedulerChoices_ProducerConsumerTaskSequence:
        project.add_mainfile_include( "MulticoreOrchestration.h")
        project.set_multicore_orchestration(
            "new MulticoreOrchestration(tarch::multicore::orchestration::FusionImplementation::ProducerConsumerTaskSequence" + task_fusion_size_string + ")"
        )
    elif scheduler == SchedulerChoices_EagerConsumerTasks:
        project.add_mainfile_include( "MulticoreOrchestration.h")
        project.set_multicore_orchestration(
            "new MulticoreOrchestration(tarch::multicore::orchestration::FusionImplementation::EagerConsumerTasks" + task_fusion_size_string + ")"
        )
    elif scheduler == SchedulerChoices_NoTasks:
        project.add_mainfile_include( "MulticoreOrchestration.h")
        project.set_multicore_orchestration(
            "new MulticoreOrchestration(tarch::multicore::orchestration::FusionImplementation::DisableFusionAndProcessImmediately)"
        )
    else:
        assert False, "not supported yet"


def create_exahype2_project(
        executable,
        offset,
        domain_size,
        end_time,
        plot_interval,
        peano_dir,
        mode,
        
        ):
    """!
    
    Create the vanilla ExaHyPE 2 project which we then use to supplement with 
    solvers. That is, this routine does nto add any solvers yet
    
    @param mode String identifying the build mode
    
    """
    project = exahype2.Project(
        ["benchmarks", "exahype2", "ccz4"],
        "ccz4",
        executable=executable,
    )

    project.set_global_simulation_parameters(
        Dimensions,  # dimensions
        offset,
        domain_size,
        end_time,
        0.0,
        plot_interval,
        [False, False, False],
        8,  # plotter precision
    )

    project.set_output_path("./")
 
    project.set_Peano4_installation(peano_dir, peano4.output.string_to_mode(mode))
      
    return project


def add_no_load_balancing(project,
                         ):
    project.set_load_balancing(
        "toolbox::loadbalancing::strategies::NoLoadBalancing",
        "nullptr,nullptr"
        )


def add_generic_load_balancing(project, 
                               trees,
                               ):
    assert trees>=0 
    if trees>0:
        project.set_load_balancing(
            "toolbox::loadbalancing::strategies::SpreadOutHierarchically",
            """
          new ::exahype2::LoadBalancingConfiguration(
            0.98, 
            1, 
            {},
            {}
          ),
          new toolbox::loadbalancing::metrics::CellCount()
        """.format(
                trees, trees
            ),
        )
    else:
        project.set_load_balancing(
            "toolbox::loadbalancing::strategies::SpreadOutHierarchically",
            """
          new ::exahype2::LoadBalancingConfiguration(
            0.98,  
            1
          ),
          new toolbox::loadbalancing::metrics::CellCount()
        """
        )

# =========================================
# Part 1 of tutorial: add the right imports
# =========================================
from toolbox.loadbalancing.api.Input.RegularGridGenerator import RegularGridGenerator
import toolbox.loadbalancing.api.Strategies.SplitTrees as split_strategy

def add_hardcoded_load_balancing_over_regular_mesh(project, 
                                                   trees_per_rank,
                                                   number_of_ranks):
    """!
    
     @todo This is not nice. The signature should accept the mesh 
     size, call Dmitry's offline load balancer and hand the outcome
     over to the load balancing directly.
    
    """

# =========================================
# Part 2 of tutorial: Create a regular mesh representation that we feed into our loadbalacing
# =========================================
#Example output:
#    Generating 2D regular grid 'pytest-hardcoded-lb-2d' with 3 levels
#  Level 1: 3^2 = 9 cells
#  Level 2: 9^2 = 81 cells
#  Level 3: 27^2 = 729 cells
#Total cells generated: 819
#Regular grid generation completed successfully
#  Tree: pytest-hardcoded-lb-2d
#  Leaf count: 729
#  Subtree count: 1##

#Peano Tree Structure From pytest-hardcoded-lb-2d
#Domain offset:  [0, 0]
#Domain size:    [1, 1]  
#Dimension:              2
#Max Level:              3
#Leaf count:     729  
#Subtree count:  1
#Split pattern:  [729.]

    tree = RegularGridGenerator(name="pytest-hardcoded-lb-2d", max_level=3, dimension=2)
    print(tree)


# @todo This is a step that is important in the second tutorial where we read from a decomposition file rather than generate one. In the first tutorial, where we use a generated mesh, it is not required and should not be discussed
#    tree = split_strategy.reset_tree_to_single_subtree(tree)

# =========================================
# Part 3 of tutorial: Split the tree into ranks*trees_per_rank subtrees and obtain split instructions
# =========================================
    total_subtrees = trees_per_rank * number_of_ranks
    number_of_splits = total_subtrees - 1

    gridSweeps, splittingTrees, numberOfCells, subTrees = split_strategy.split_tree(
        tree,
        number_of_splits=number_of_splits,
        enable_propagate_bottom_up=False,
        enable_extract_split_instructions=True,
    )  # creates subtrees {0,1,2}
    print("After split_tree():")
    print(tree)

    print("Extracted hardcoded parameters:")
    print("  gridSweeps     =", gridSweeps)
    print("  splittingTrees =", splittingTrees)
    print("  numberOfCells  =", numberOfCells)
    print("  subTrees       =", subTrees, "(subtree ids, not ranks)")

# @todo Implement the mapping to number of ranks.
#    split_strategy.map_to_number_of_ranks(2)

# @atodo Again plot it.

# =========================================
# Part 4 of tutorial: Hand the split instr. over to the load balancer
# =========================================
    # Map subtree ids to destination ranks according to trees_per_rank.
    destinationRanks = [
        min(number_of_ranks - 1, sid // trees_per_rank) for sid in subTrees
    ]

    # Build Hardcoded constructor argument string (single mode variant)
    hardcoded_mode = "peano4::SplitInstruction::Mode::AggressiveTopDown"
    hardcoded_config = (
        "{"
        f"{{{', '.join(map(str, gridSweeps))}}}, "
        f"{{{', '.join(map(str, splittingTrees))}}}, "
        f"{{{', '.join(map(str, numberOfCells))}}}, "
        f"{{{', '.join(map(str, destinationRanks))}}}, "
        f"{hardcoded_mode}"
        "}"
    )

    project.set_load_balancing(
      "toolbox::loadbalancing::strategies::Hardcoded",
      hardcoded_config
    )


def add_load_balancing_with_cost_metric(project,
                                        trees):
        load_balancing_cell_label = (
            exahype2.grid.FineGridCellLoadBalancingCostMetric.create_cell_label(project)
        )
        load_balancing_action_set = exahype2.grid.FineGridCellLoadBalancingCostMetric(
            load_balancing_cell_label, "1.0 / (1.0+tarch::la::norm2(marker.x()))"
        )
        project.add_action_set_to_create_grid(load_balancing_action_set)
        if trees==0:
            configuration = """
          new ::exahype2::LoadBalancingConfiguration(
            0.98, 
            1, 
            {},
            {}
          ),
          new toolbox::loadbalancing::metrics::CustomCellWeight()
        """.format(
                trees, trees
            )
        else:
            configuration = """
          new ::exahype2::LoadBalancingConfiguration(
            0.98, 
            1, 
          ),
          new toolbox::loadbalancing::metrics::CustomCellWeight()
        """
        project.set_load_balancing(
            "toolbox::loadbalancing::strategies::cascade::SpreadOut_SplitOversizedTree",
            configuration,
        )


def lower_to_peano4_project(project: exahype2.Project,
                            verbose,
                            add_fv,
                            add_fd4,
                            ):
    """!
    
    Lower ExaHyPE project to Peano 4 project
    
    Take the ExaHyPE 2 project with all the CCZ4 solvers and lower it
    into a plain Peano project. Then, add all the CCZ4-specific implementation
    fiels to the build setup.
    
    This routine also adds the multicore orchestration implementation. This
    can be done totally agonistic to the fact if the tailored orchestration
    is used or not. After all, the linker will throw it out if it is not 
    used at all.
    
    We assume that project hosts at least one solver and that this solver is
    the primary solver, i.e. the one covering the whole domain.
    
    """
    peano4_project = project.generate_Peano4_project(verbose)

    assert project._solvers != []

    project._solvers[0].add_makefile_parameters(
        peano4_project, "../../../../applications/exahype2/ccz4"
    )

    peano4_project.output.makefile.add_linker_flag("-lm -lgsl -lgslcblas")
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

    peano4_project.output.makefile.add_cpp_file("MulticoreOrchestration.cpp")
    peano4_project.output.makefile.add_cpp_file("SingleBlackHoleScenario.cpp")

    peano4_project.output.makefile.add_CXX_flag("-DIncludeTwoPunctures")
    if add_fv and add_fd4:
        peano4_project.output.makefile.add_CXX_flag("-DCoupleWithFV")
        peano4_project.output.makefile.add_cpp_file("CCZ4SBH_FV.cpp")
        peano4_project.output.makefile.add_h_file("CCZ4SBH_FV.h")
        peano4_project.output.makefile.add_cpp_file("CCZ4SBH_FD4.cpp")
        peano4_project.output.makefile.add_h_file("CCZ4SBH_FD4.h")
    elif add_fv:
        peano4_project.output.makefile.add_CXX_flag("-DPureFV")
        peano4_project.output.makefile.add_cpp_file("CCZ4SBH_FV.cpp")
        peano4_project.output.makefile.add_h_file("CCZ4SBH_FV.h")
    else:
        peano4_project.output.makefile.add_CXX_flag("-DPureFD4")
        peano4_project.output.makefile.add_cpp_file("CCZ4SBH_FD4.cpp")
        peano4_project.output.makefile.add_h_file("CCZ4SBH_FD4.h")

    return peano4_project
