import os
import argparse

import peano4
import exahype2
import dastgen2

import peano4.toolbox.particles
import numpy as np

# See comments in README.dox
from CCZ4Solver import CCZ4Solver_FV_GlobalAdaptiveTimeStep
from CCZ4Solver import CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import CCZ4Solver_FD4_GlobalAdaptiveTimeStep
from CCZ4Solver import CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import CCZ4Solver_RKDG_GlobalAdaptiveTimeStep
from CCZ4Solver import CCZ4Solver_RKDG_GlobalAdaptiveTimeStepWithEnclaveTasking
from CCZ4Solver import (
    CCZ4Solver_FD4_SecondOrderFormulation_GlobalAdaptiveTimeStepWithEnclaveTasking,
)

from ComputeFirstDerivatives import ComputeFirstDerivativesFD4RK

storage_types = {
    "CallStack":     exahype2.solvers.Storage.CallStack,
    "Heap":          exahype2.solvers.Storage.Heap,
    "SmartPointers": exahype2.solvers.Storage.SmartPointers,
}


parser = argparse.ArgumentParser(
    description="ExaHyPE 2 - CCZ4-GaugeWave benchmarking script"
)
parser.add_argument(
    "-j", "--parallel-builds", dest="j", type=int, default=-1, help="Parallel builds"
)
parser.add_argument(
    "-st",
    "--storage",
    dest="storage",
    choices=storage_types.keys(),
    required=True,
    help="|".join(storage_types.keys()),
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
        "fd4-rk1", "sofd4-rk1",
        "fd4-rk2", "sofd4-rk2",
        "fd4-rk3", "sofd4-rk3",
        "fd4-rk4", "sofd4-rk4",
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
        "dg4-rk3"
    ],
    required=True,
    help="Pick solver type",
)
parser.add_argument(
    "-enclave",
    "--enclave",
    dest="enclave",
    action="store_true",
    default=False,
    help="switch on the enclave tasking solver",
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

if args.solver == "fv" and args.enclave:
    my_solver = CCZ4Solver_FV_GlobalAdaptiveTimeStepWithEnclaveTasking(
        name="CCZ4FV",
        patch_size=9,
        min_volume_h=args.cell_size / 5,
        max_volume_h=args.cell_size / 5,
        pde_terms_without_state=True,
    )
    my_solver.set_implementation(
        initial_conditions="""
    for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0; 
      ::applications::exahype2::ccz4::gaugeWave(Q, volumeCentre, 0);
    """
    )
if args.solver == "fv" and not args.enclave:
    my_solver = CCZ4Solver_FV_GlobalAdaptiveTimeStep(
        name="CCZ4FV",
        patch_size=9,
        min_volume_h=args.cell_size / 5,
        max_volume_h=args.cell_size / 5,
        pde_terms_without_state=True,
    )
    my_solver.set_implementation(
        initial_conditions="""
    for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0; 
      ::applications::exahype2::ccz4::gaugeWave(Q, volumeCentre, 0);
    """
    )
if "fd4-rk" in args.solver or "sofd4-rk" in args.solver:
    order = int(args.solver[-1])
    second_order_formulation = "sofd4" in args.solver
    if args.enclave==True:
        my_solver = CCZ4Solver_FD4_GlobalAdaptiveTimeStepWithEnclaveTasking(
            name="CCZ4FD4RK" + str(order) + "Enclave",
            patch_size=9,
            rk_order=order,
            min_meshcell_h=args.cell_size / 3,
            max_meshcell_h=args.cell_size / 3,
            second_order = second_order_formulation,
            pde_terms_without_state=False
        )
    else:
        my_solver = CCZ4Solver_FD4_GlobalAdaptiveTimeStep(
            name="CCZ4FD4RK" + str(order),
            patch_size=9,
            rk_order=order,
            min_meshcell_h=args.cell_size / 3,
            max_meshcell_h=args.cell_size / 3,
            second_order = second_order_formulation
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
    if args.enclave:
        my_solver = CCZ4Solver_RKDG_GlobalAdaptiveTimeStepWithEnclaveTasking(
            name="CCZ4DG" + str(space_order) + "RK" + str(time_order),
            polynomials=exahype2.solvers.GaussLegendreBasis(space_order),
            rk_order=time_order,
            min_cell_h=args.cell_size * space_order,
            max_cell_h=args.cell_size * space_order,
            pde_terms_without_state=True,
        )
    else:
        my_solver = CCZ4Solver_RKDG_GlobalAdaptiveTimeStep(
            name="CCZ4DG" + str(space_order) + "RK" + str(time_order),
            polynomials=exahype2.solvers.GaussLegendreBasis(space_order),
            rk_order=time_order,
            min_cell_h=args.cell_size * space_order,
            max_cell_h=args.cell_size * space_order,
            pde_terms_without_state=True,
        )
    my_solver.set_implementation(
        initial_conditions="""
                                                       for (int i=0; i<NumberOfUnknowns+NumberOfAuxiliaryVariables; i++) Q[i] = 0.0; 
                                                       ::applications::exahype2::ccz4::gaugeWave(Q, x, 0);
                                                     """
    )


my_solver.switch_storage_scheme(
    cell_data_storage=storage_types[args.storage],
    face_data_storage=storage_types[args.storage],
)


assert my_solver != None

my_solver.add_all_solver_constants()
project.add_solver(my_solver)

########################################################################################
# Tracer setting
########################################################################################
if args.tracer:
    if args.asserts:
        time_delta_between_two_snapsots = 1e-4
    else:
        time_delta_between_two_snapsots = 0.1
    my_solver.add_tracer(
        name="Tracer",
        coordinates=[[0, 0, 0]],#coordinates=[[-0.4251, 0, 0], [0, 0, 0], [0.4251, 0, 0]],
        project=project,
        number_of_entries_between_two_db_flushes = 100,
        data_delta_between_two_snapsots = 1e-8,
        time_delta_between_two_snapsots = time_delta_between_two_snapsots,
        clear_database_after_flush = False,
        tracer_unknowns = None
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

probe_point = [-12,-12,0.0]
project.add_plot_filter( probe_point,[24.0,24.0,0.01],1 )

project.set_Peano4_installation(args.peanodir, build_mode)

project.set_load_balancing(
    "toolbox::loadbalancing::strategies::SpreadOutHierarchically",
    "(new ::exahype2::LoadBalancingConfiguration(0.98,27,true))",
)

peano4_project = project.generate_Peano4_project(verbose=args.verbose)

if "sofd4" in args.solver:
    additional_mesh_traversal = peano4.solversteps.Step( name = "AdditionalMeshTraversal",
                                                           add_user_defined_actions=False,
                                                           )
    
    #
    # We misuse the projection. See @ref benchmarks_exahype2_ccz4_gauge_wave for details
    #
    project_patch_onto_faces  = exahype2.solvers.rkfd.actionsets.ProjectPatchOntoFaces(my_solver)
    project_patch_onto_faces.guards     = [ "false" for x in range(0,my_solver.number_of_Runge_Kutta_steps()+1) ]
    project_patch_onto_faces.guards[-1] = my_solver._store_cell_data_default_guard()

    roll_over_projected_faces = exahype2.solvers.rkfd.actionsets.RollOverUpdatedFace(my_solver, 
                                                                                    my_solver._store_face_data_default_guard(),
                                                                                    )

    additional_mesh_traversal.add_action_set( ComputeFirstDerivativesFD4RK(solver=my_solver,
                                                                            is_enclave_solver = args.enclave,
                                                                            ))
    additional_mesh_traversal.add_action_set( project_patch_onto_faces )
    additional_mesh_traversal.add_action_set( roll_over_projected_faces )
     
    project.init_new_user_defined_algorithmic_step( additional_mesh_traversal )
    
    # 
    # Consult remarks in README.dox on the additional step and how it is 
    # integrated into the main routine.
    #
    peano4_project.solversteps.add_step( additional_mesh_traversal )
    peano4_project.constants.define( "USE_ADDITIONAL_MESH_TRAVERSAL" )


my_solver.add_makefile_parameters(
    peano4_project, "../../../../applications/exahype2/ccz4"
)

peano4_project.generate(throw_away_data_after_generation=False)
peano4_project.build(make_clean_first=True, number_of_parallel_builds=args.j)
