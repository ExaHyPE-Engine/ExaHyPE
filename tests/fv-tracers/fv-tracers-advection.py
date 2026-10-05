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
    args, _ = parser.parse_known_args()
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
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
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

fv_solver = exahype2.fv.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"v": project.dimensions},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    scheme=exahype2.fv.Godunov(),
    precision=args.precision,
)

fv_solver.set_pde_terms(
    initial_condition=initial_conditions,
    boundary_conditions=boundary_conditions,
    flux="""
  for (int i = 0; i < NumberOfUnknowns; i++) {
    F[i] = 0.0;
  }
  F[normal] = Q[normal];
""",
    max_eigenvalue="return 1.0;",
)

project.add_solver(fv_solver)

tracers = project.add_tracer(
    name="Tracers",
    particle_attributes={
        "unknowns": (
            {
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
        )
    },
)

tracers.insert_by_coordinates([[0.25, 0.5, 0.5], [0.5, 0.5, 0.5], [0.75, 0.5, 0.5]])

# The tracers read the velocity components once after the initialisation and stay in place.
fv_solver.trace(
    tracers,
    forces=exahype2.particles.Forces(
        interpolation_scheme=2,
        integration_scheme=0,
        force_method=0,
        fields_to_interpolate=",".join(
            str(1 + axis) for axis in range(project.dimensions)
        ),
    ),
    time_steps=False,
)

project.generate_and_build()
