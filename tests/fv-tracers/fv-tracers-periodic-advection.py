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
    parser.add_argument("-et", "--end-time", type=float, default=0.6)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=4)
    parser.add_argument("-l", "--grid-levels", type=int, default=3)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

# A constant velocity field carries every tracer through the periodic walls:
# The tracer at (0.9, 0.5) leaves through the right wall, the one at
# (0.5, 0.9) through the top wall and the one at (0.7, 0.8) through both.
project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name=".",
    dimensions=2,
    size=[1.0, 1.0, 1.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[True, True, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

initial_conditions = """
  Q[0] = 1.0;
  Q[1] = 0.5;
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
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

tracers = fv_solver.add_tracer(
    name="Tracers",
    particle_attributes={
        "unknowns": {
            "rho": 1,
            "fluid_u": 1,
            "fluid_v": 1,
            "u": 1,
            "v": 1,
            "a_x": 1,
            "a_y": 1,
            "d": 1,
        }
    },
)

init_tracers = exahype2.tracer.InsertParticlesByCoordinates(
    particle_set=tracers,
    coordinates=[[0.9, 0.5, 0.0], [0.5, 0.9, 0.0], [0.7, 0.8, 0.0]],
)
init_tracers.descend_invocation_order = 0
project.add_action_set_to_initialisation(init_tracers)

# The fluid velocity becomes the tracer velocity and an explicit Euler step
# moves the tracer with it; no force acts on the tracer.
tracing_action_set = exahype2.tracer.FiniteVolumesTracingWithForce(
    tracers,
    fv_solver,
    project,
    interpolation_scheme=2,
    integration_scheme=1,
    force_method=0,
    fields_to_interpolate="3,4",
)
tracing_action_set.descend_invocation_order = (
    fv_solver._action_set_update_patch.descend_invocation_order + 1
)
project.add_action_set_to_timestepping(tracing_action_set)
project.add_action_set_to_initialisation(tracing_action_set)

project.generate_and_build()
