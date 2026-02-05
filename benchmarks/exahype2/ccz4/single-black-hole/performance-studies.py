# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
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


def create_exahype2_project(
        executable,
        end_time,
        plot_interval,
        peano_dir,
        mode,
        offset,
        domain_size
        ):
    """!
    
    Create the vanilla ExaHyPE 2 project which we then use to supplement with 
    solvers. That is, this routine does nto add any solvers yet
    
    """
    project = exahype2.Project(
        ["benchmarks", "exahype2", "ccz4"],
        "ccz4",
        executable=executable,
    )

    dimensions = 3

    project.set_global_simulation_parameters(
        dimensions,  # dimensions
        offset,
        domain_size,
        args.end_time,
        0.0,
        args.plot_interval,
        [False, False, False],
        8,  # plotter precision
    )

    project.set_output_path("./")
 
    project.set_Peano4_installation(peano_dir, peano4.output.string_to_mode(mode))
      
    return project


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
         
def add_hardcoded_load_balancing_over_regular_mesh(project, 
                                                   trees):
    """!
    
     @todo This is not nice. The signature should accept the mesh 
     size, call Dmitry's offline load balancer and hand the outcome
     over to the load balancing directly.
    
    """
    project.set_load_balancing(
      "toolbox::loadbalancing::strategies::Hardcoded",
      "{ {6,6,6}, {0,0,0}, {4921,4921,4920}, {0,0,0}, peano4::SplitInstruction::Mode::AggressiveTopDown}"
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


def lower_to_peano4_project(project,
                            verbose,
                            add_fv,
                            add_fd4,
                            ):
    """!
    
    Take the ExaHyPE 2 project with all the CCZ4 solvers and lower it
    into a plain Peano project. Then, add all the CCZ4-specific implementation
    fiels to the build setup.
    
    This routine also adds the multicore orchestration implementation. This
    can be done totally agonistic to the fact if the tailored orchestration
    is used or not. After all, the linker will throw it out if it is not 
    used at all.
    
    """
    peano4_project = project.generate_Peano4_project(verbose)

    my_primary_solver.add_makefile_parameters(
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
<<<<<<< HEAD
    parser.add_argument(
        "-product",
        "--production-run",
        dest="production_run",
        type=int,
        default=0,
        help="switch to the pre-defined production run setup",
=======
    # If --max-tasks is specified, --min-tasks must also be specifed due to the order of arguments in the C++ code
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
>>>>>>> refs/remotes/origin/p4
    )

    args = parser.parse_args()
    return args


def configure_multicore_configuration(scheduler):
    """!
    
    Add a proper scheduler to the project
    
    """
    if args.min_tasks == -1:
        task_fusion_size_string = ""
    elif args.max_tasks == -1:
        task_fusion_size_string = f", {args.min_tasks}"
    else:
        task_fusion_size_string = f", {args.min_tasks}, {args.max_tasks}"
         
    if args.scheduler == SchedulerChoices_Native:
        project.add_mainfile_include( "tarch/multicore/orchestration/Hardcoded.h")
        project.set_multicore_orchestration(
            "tarch::multicore::orchestration::Hardcoded::createNative()"
        )
    elif args.scheduler == SchedulerChoices_NativePriorities:
        project.add_mainfile_include( "MulticoreOrchestration.h")
        project.set_multicore_orchestration(
            "new MulticoreOrchestration(tarch::multicore::orchestration::FusionImplementation::DisableFusionAndMapOntoNativeTask)"
        )
    elif args.scheduler == SchedulerChoices_LazyEvaluation:
        project.add_mainfile_include( "MulticoreOrchestration.h")
        project.set_multicore_orchestration(
            "new MulticoreOrchestration(tarch::multicore::orchestration::FusionImplementation::LazyEvaluation" + task_fusion_size_string + ")"
        )
    elif args.scheduler == SchedulerChoices_TimedExecution:
        project.add_mainfile_include( "MulticoreOrchestration.h")
        project.set_multicore_orchestration(
            "new MulticoreOrchestration(tarch::multicore::orchestration::FusionImplementation::TimedExecution" + task_fusion_size_string + ")"
        )
    elif args.scheduler == SchedulerChoices_ProducerConsumerTaskSequence:
        project.add_mainfile_include( "MulticoreOrchestration.h")
        project.set_multicore_orchestration(
            "new MulticoreOrchestration(tarch::multicore::orchestration::FusionImplementation::ProducerConsumerTaskSequence" + task_fusion_size_string + ")"
        )
    elif args.scheduler == SchedulerChoices_EagerConsumerTasks:
        project.add_mainfile_include( "MulticoreOrchestration.h")
        project.set_multicore_orchestration(
            "new MulticoreOrchestration(tarch::multicore::orchestration::FusionImplementation::EagerConsumerTasks" + task_fusion_size_string + ")"
        )
    elif args.scheduler == SchedulerChoices_NoTasks:
        project.add_mainfile_include( "MulticoreOrchestration.h")
        project.set_multicore_orchestration(
            "new MulticoreOrchestration(tarch::multicore::orchestration::FusionImplementation::DisableFusionAndProcessImmediately)"
        )
    else:
            assert False, "not supported yet"
           
               
if __name__ == "__main__":
    args = get_command_line_args()
    
    if args.amr:
        max_cell_size = domain_size[0] / 3.0
        min_cell_size = args.cell_size
    else:
        max_cell_size = args.cell_size
        min_cell_size = args.cell_size

    offset = [-9, -9, -9]
    domain_size   = [18, 18, 18]

    # here we prepare the production run setup, the domain size, the minimum and maximal cell size 
    # need to be changed accordingly. The refinementCriterion, which would be implemented in _FD4.cpp
    # file, is controled by the flag variable "production_run" itself.
    # The patch size is assumed to be 6 in the setup.
    if args.production_run==1: #here is the "standard" production run setup
        max_cell_size = 0.4*6
        min_cell_size = 0.04*6
        # offset and domain size remain unchanged
    elif args.production_run==2: #here is a bigger production run setup
        max_cell_size = 0.7*6
        min_cell_size = 0.03*6
        offset = [-18, -18, -18]
        domain_size   = [36, 36, 36]
    elif args.production_run==3: 
        #here is a much bigger setup which we shall be able to simulate for gravitational wave
        # extraction in the future
        max_cell_size = 6.1*6
        min_cell_size = 0.03*6
        offset = [-162, -162, -162]
        domain_size   = [324, 324, 324]

    # =================================
    # Create project
    # =================================
    project = create_exahype2_project( 
        executable    = args.output,
        end_time      = args.end_time,
        plot_interval = args.plot_interval,
        peano_dir     = args.peanodir, 
        mode          = args.mode,
        offset        = offset,
        domain_size   = domain_size
        )

    ########################################################################################
    # Configure run
    ########################################################################################
    configure_multicore_configuration(args.scheduler)

    added_limiter = False
    if "fd4-rk1-ps-" in args.solver:
        FD4PatchSize = int(args.solver[-1])
        my_primary_solver = FD4SolverWithoutLimiter(
            name="CCZ4SBH",
            patch_size=FD4PatchSize,
            min_cell_size=min_cell_size,
            max_cell_size=max_cell_size,
            production_run=args.production_run #send the corresponding flag to cpp level for refinement control
        )
        if args.production_run>0: #then we shall use second-order interpolation and restriction
            exahype2.solvers.rkfd.fd4.switch_to_FD4_second_order_interpolation( my_primary_solver ) 
            exahype2.solvers.rkfd.fd4.switch_to_FD4_second_order_restriction( my_primary_solver ) 
        project.add_solver(my_primary_solver)
        added_limiter = False

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
        project.add_solver(my_primary_solver)
        added_limiter = False
    else:
        raise "unknown solver type {}".format(args.solver)

    # probe_point = [-12,-12,0.0]
    # project.add_plot_filter( probe_point,[24.0,24.0,0.01],1 )
    # my_solver.select_dofs_to_print = [0,12,16,17,18,54,59,60]

    if args.load_balancing_metric == LoadBalancingMetrics_Generic:
        add_generic_load_balancing(project, args.trees)
    elif args.load_balancing_metric == LoadBalancingMetrics_Tailored:
        add_load_balancing_with_cost_metric(project, args.trees)
    elif args.load_balancing_metric == LoadBalancingMetrics_Offline_Regular:
        add_hardcoded_load_balancing_over_regular_mesh(project, args.trees)
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
