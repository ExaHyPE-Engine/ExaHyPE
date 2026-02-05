# This file is part of the Peano project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import argparse

import peano4
import exahype2
import dastgen2

if __name__ == "__main__":
    args = get_command_line_args()
    
    if args.amr:
        max_cell_size = domain_size[0] / 3.0
        min_cell_size = args.cell_size
    else:
        max_cell_size = args.cell_size
        min_cell_size = args.cell_size

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
