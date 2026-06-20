# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2


def parse_arguments():
    available_precisions = ["float", "double"]
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument(
        "-pr", "--precision", choices=available_precisions, default="double"
    )
    parser.add_argument("-et", "--end-time", type=float, default=0.5)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=16)
    parser.add_argument("-l", "--grid-levels", type=int, default=3)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=1)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=1)
    args, _ = parser.parse_known_args()
    if args.grid_levels is not None:
        args.min_grid_levels = args.grid_levels
        args.max_grid_levels = args.grid_levels
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name=".",
    dimensions=2,
    size=[1.0, 1.0, 1.0],
    offset=[0.0, 0.0, 0.0],
    periodic_BC=[False, False, False],
    executable="ExaHyPE",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = std::sin(x(0) * tarch::la::PI) * std::sin(x(1) * tarch::la::PI)
#if DIMENSIONS == 3
      * std::sin(x(2) * tarch::la::PI)
#endif
    ;
  }
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;

  if (x(0) > 0.5) {
    result = ::exahype2::RefinementCommand::Refine;
  } else {
    result = ::exahype2::RefinementCommand::Erase;
  }

  return result;
"""

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"v": project.dimensions},
    auxiliary_variables=0,
    time_step_relaxation=0.5,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux="""
  for (int i = 0; i < NumberOfUnknowns; i++) {
    F[i] = 0.0;
  }
  F[normal] = Q[normal];
""",
    max_eigenvalue="return 1.0;",
)

fv_solver.set_solver_precisions(
    storage_precision=args.precision, compute_precision=args.precision
)

project.add_solver(fv_solver)

tracers = project.add_tracer(
    name="Tracers",
    particle_attributes={
        "unknowns": {
            "rho": 1,
            "fluid_u": 1,
            "fluid_v": 1,
            "fluid_w": 1,  # Interpolated fluid velocity
            "u": 1,
            "v": 1,
            "w": 1,  # Particle's own velocity
            "a_x": 1,
            "a_y": 1,
            "a_z": 1,
            "d": 1,
        }
        if project.dimensions == 3
        else {
            "rho": 1,
            "fluid_u": 1,
            "fluid_v": 1,  # Interpolated fluid velocity
            "u": 1,
            "v": 1,  # Particle's own velocity
            "a_x": 1,
            "a_y": 1,
            "d": 1,
        }
    },
)

init_tracers = exahype2.tracer.InsertParticlesByCoordinates(
    particle_set=tracers,
    coordinates=[[0.25, 0.5, 0.5], [0.5, 0.5, 0.5], [0.75, 0.5, 0.5]],
)

init_tracers.descend_invocation_order = 0  # TODO: Why?
project.add_action_set_to_initialisation(
    init_tracers
)  # TODO: Is there not an easier way to expose the API to the user?

# TODO: Why do we need this? Why not just fv_solver.addTracers(...)
# TODO: Don't make the API too hard to understand for the user!
tracing_action_set = exahype2.tracer.FiniteVolumesTracing(
    tracers,
    fv_solver,
    project_on_tracer_properties_kernel="::exahype2::fv::updateParticleState<0, 1, 2>",
    projection_kernel_arguments=""" // TODO: Also way too complicated!
  *p,
  fineGridCell{{SOLVER_NAME}}CellLabel.getTimeStepSize(),
  marker,
  {{PATCH_SIZE}},
  {{NUMBER_OF_UNKNOWNS}}+{{NUMBER_OF_AUXILIARY_VARIABLES}},
  fineGridCell{{CURRENT_UNKNOWN_IDENTIFIER}}.data(),
  {1,1,1},
  {0,0,0},
  {1,0,0}
""",
)
tracing_action_set.descend_invocation_order = (
    fv_solver._action_set_update_patch.descend_invocation_order
    + 1  # TODO: Why does the user need to do this advanced setting?
)
# TODO: This is all too complicated!
project.add_action_set_to_initialisation(tracing_action_set)
# project.add_action_set_to_timestepping(tracing_action_set)

project = project.generate_Peano4_project(verbose=False)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
