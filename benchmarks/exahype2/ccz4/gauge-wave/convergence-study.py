import os
import argparse

import peano4
import exahype2
import dastgen2

import peano4.toolbox.particles
import numpy as np

# See comments in README.dox
from CCZ4Solver import CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import CCZ4Solver_RKDG_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import (
    CCZ4Solver_FD4_SecondOrderFormulation_GlobalAdaptiveTimeStepWithEnclaveTasking,
)


parser = argparse.ArgumentParser(
    description="ExaHyPE 2 - CCZ4-GaugeWave benchmarking script"
)
parser.add_argument(
    "-j", "--parallel-builds", dest="j", type=int, default=-1, help="Parallel builds"
)
parser.add_argument(
    "-pd",
    "--peano-dir",
    dest="peanodir",
    default="../../../../",
    help="Peano4 directory",
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
    "-s",
    "--solver",
    dest="solver",
    choices=[
        "fv",
        "fd4-rk1",
        "fd4-rk2",
        "fd4-rk3",
        "fd4-rk4",
        "dg0-rk1",
        "dg0-rk2",
        "dg0-rk3",
        "dg0-rk4",
        "dg1-rk1",
        "dg2-rk1",
        "dg2-rk2",
        "dg3-rk1",
        "dg3-rk3",
        "dg4-rk4",
        "dg4-rk1",
        "dg4-rk2",
        "dg4-rk3",
        "second-order",
    ],
    required=True,
    help="Pick solver type",
)
parser.add_argument(
    "-et",
    "--end-time",
    dest="end_time",
    type=float,
    default=1.0,
    help="End of simulation",
)
parser.add_argument(
    "-cs",
    "--cell-size",
    dest="cell_size",
    type=float,
    default=0.3,
    help="Cell size (default 0.3)",
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
    "-tracer",
    "--tracer",
    dest="tracer",
    action="store_true",
    default=False,
    help="Activate tracer",
)
parser.add_argument(
    "-hs",
    "--heap-storage",
    dest="use_heap_storage",
    action="store_true",
    default=False,
    help="Store user data on heap via smart pointers",
)

args = parser.parse_args()


project = exahype2.Project(
    ["benchmarks", "exahype2", "ccz4"],
    "ccz4",
    executable=args.solver + "-" + str(args.cell_size),
)


#
# Create solver class
#
# For FV and FD schemes, I pick patches of 9x9x9 here, as I then can match stuff exactly
# with the DG solver of DG order 0. This helps me to debug stuff.
#
my_solver = None

if args.solver == "fv":
    my_solver = CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking(
        name="CCZ4FV",
        patch_size=9,
        min_volume_h=args.cell_size / 5,
        max_volume_h=args.cell_size / 5,
    )
    my_solver.set_implementation(
        initial_conditions="""
    for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0; 
      ::applications::exahype2::ccz4::gaugeWave(Q, volumeCentre, 0);
    """
    )
if "fd4-rk" in args.solver:
    order = int(args.solver[-1])
    my_solver = CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking(
        name="CCZ4FD4RK" + str(order),
        patch_size=5,
        rk_order=order,
        min_meshcell_h=args.cell_size / 3,
        max_meshcell_h=args.cell_size / 3,
    )
    my_solver.set_implementation(
        initial_conditions="""
    for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0; 
      ::applications::exahype2::ccz4::gaugeWave(Q, meshCellCentre, 0);
    """
    )

if "second-order" in args.solver:
    order = 1
    my_solver = (
        CCZ4Solver_FD4_SecondOrderFormulation_GlobalAdaptiveTimeStepWithEnclaveTasking(
            name="CCZ4FD4RK_SO_" + str(order),
            patch_size=5,
            rk_order=order,
            min_meshcell_h=args.cell_size / 3,
            max_meshcell_h=args.cell_size / 3,
            reconstruction_type="leftDifferences",
        )
    )
    my_solver.set_implementation(
        initial_conditions="""
    for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0; 
      ::applications::exahype2::ccz4::gaugeWave(Q, meshCellCentre, 0);
    """
    )


if "rk" in args.solver and "dg" in args.solver:
    space_order = int(args.solver[2])
    time_order = int(args.solver[6])
    my_solver = CCZ4Solver_RKDG_GlobalAdaptiveTimeStepWithEnclaveTasking(
        name="CCZ4DG" + str(space_order) + "RK" + str(time_order),
        polynomials=exahype2.solvers.GaussLegendreBasis(space_order),
        rk_order=time_order,
        min_cell_h=args.cell_size * space_order,
        max_cell_h=args.cell_size * space_order,
    )
    my_solver.set_implementation(
        initial_conditions="""
                                                       for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0; 
                                                       ::applications::exahype2::ccz4::gaugeWave(Q, x, 0);
                                                     """
    )


assert my_solver != None

my_solver.add_all_solver_constants()
project.add_solver(my_solver)

########################################################################################
# Tracer setting
########################################################################################
if args.tracer:
    my_solver.add_tracer(
        name="Tracer",
        coordinates=[[-0.4251, 0, 0], [0, 0, 0], [0.4251, 0, 0]],
        project=project,
    )


########################################################################################
# Build the project
########################################################################################

if args.asserts:
    build_mode = peano4.output.CompileMode.Asserts
else:
    build_mode = peano4.output.CompileMode.Release

if args.asserts:
    delta_plot = 1.0 / 100
else:
    delta_plot = 1.0 / 20

dimensions = 3
offset = [-0.5, -0.5, -0.5]
domain_size = [1, 1, 1]

periodic_boundary_conditions = [True, True, True]  # Periodic BC

project.set_global_simulation_parameters(
    dimensions,  # dimensions
    offset,
    domain_size,
    args.end_time,
    0.0,
    delta_plot,
    periodic_boundary_conditions,
    8,  # plotter precision
)

project.set_Peano4_installation(args.peanodir, build_mode)

# project.set_load_balancing("toolbox::loadbalancing::strategies::RecursiveBipartition","(new ::exahype2::LoadBalancingConfiguration(0.98,0))" )
project.set_load_balancing(
    "toolbox::loadbalancing::strategies::SpreadOutHierarchically",
    "(new ::exahype2::LoadBalancingConfiguration(0.98))",
)

peano4_project = project.generate_Peano4_project(verbose=args.verbose)

my_solver.add_makefile_parameters(
    peano4_project, "../../../../applications/exahype2/ccz4"
)

peano4_project.generate(throw_away_data_after_generation=False)
peano4_project.build(make_clean_first=True, number_of_parallel_builds=args.j)
