# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
from pathlib import Path

import exahype2

"""
This is a convergence test for time integration methods in particle tracing using orbital mechanics.
"""

interpolation_methods = {
    "None": 0,
    "Constant": 1,
    "Linear": 2,
}
integration_schemes = {
    "Static": 0,
    "Euler": 1,
    "SemiEuler": 2,
    "Verlet": 3,
    "Midpoint": 4,
    "RK3": 5,
    "RK4": 6,
}


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument(
        "-interp",
        "--interpolation",
        choices=interpolation_methods.keys(),
        default="None",
    )
    parser.add_argument(
        "-integ", "--integration", choices=integration_schemes.keys(), default="Euler"
    )
    parser.add_argument("-et", "--end-time", type=float, default=2.0)
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
    size=[3.0, 3.0, 3.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
"""

flux = """
  for (int i = 0; i < NumberOfUnknowns; i++) {
    F[i] = 0.0;
  }
"""

max_eigenvalue = """
  return 1.0;
"""

analytical_solution = """
  // Analytical solution for one-body problem with fixed central mass (circular orbit)
  constexpr double GravitationalParameter = 10.0; // G*M

  constexpr double CentreX = 1.5;
  constexpr double CentreY = 1.5;

  constexpr double StartX = 2.0;
  constexpr double StartY = 2.0;

  const double r = sqrt(pow(StartX - CentreX, 2) + pow(StartY - CentreY, 2));
  const double omega = sqrt(GravitationalParameter / (r * r * r));

  const double mass = 1.0;

  const double angle = atan2(StartY - CentreY, StartX - CentreX) + omega * t;

  // Position
  solution[0] = CentreX + r * cos(angle); // x
  solution[1] = CentreY + r * sin(angle); // y

  // Velocity (perpendicular to radius)
  solution[5] = -r * omega * sin(angle); // vx
  solution[6] =  r * omega * cos(angle); // vy

  // Acceleration (central)
  solution[7] = -GravitationalParameter / (r * r * r) * (solution[0] - CentreX); // ax
  solution[8] = -GravitationalParameter / (r * r * r) * (solution[1] - CentreY); // ay

  // Other fields
  solution[2] = 0.0;  // rho (density)
  solution[3] = 0.0;  // fluid_u
  solution[4] = 0.0;  // fluid_v
  solution[9] = mass; // m (mass)
"""

fv_solver = exahype2.fv.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns=2,
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Fixed(0.0025 / (args.degrees_of_freedom**0.5)),
    scheme=exahype2.fv.Godunov(),
)

fv_solver.set_pde_terms(
    initial_condition=initial_conditions,
    boundary_conditions=boundary_conditions,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
)

project.add_solver(fv_solver)

tracer_attributes = {
    "unknowns": {
        "rho": 1,
        "fluid_u": 1,
        "fluid_v": 1,
        "u": 1,
        "v": 1,
        "a_x": 1,
        "a_y": 1,
        "m": 1,
    }
}

tracer_particles = project.add_tracer(
    name="Tracers", particle_attributes=tracer_attributes, solver=fv_solver
)

init_tracers = exahype2.particles.InsertParticlesByCoordinatesAndData(
    particle_set=tracer_particles,
    unknowns_per_particle=len(tracer_attributes["unknowns"]),
    initial_data="""
  {2.0, 2.0, 0.0, 0.0, 0.0, -2.65914794847249380538e+00, 2.65914794847249424947e+00, -1.41421356237309474579e+01, -1.41421356237309474579e+01, 1.0}
""",
)

init_tracers.descend_invocation_order = 0
project.add_action_set_to_initialisation(init_tracers)

fv_solver.trace(
    tracer_particles,
    forces=exahype2.particles.Forces(
        integration_scheme=integration_schemes[args.integration],
        force_method="1",  # Central gravitational force towards the centre of the domain
        interpolation_scheme=interpolation_methods[args.interpolation],
    ),
)

fv_solver.set_pde_terms(particle_analytical_solution=analytical_solution)
fv_solver.measure_particle_error(
    tracer_particles,
    file="Error_" + Path(__file__).name,
    descend_invocation_order=65536,  # After all other action sets
)

project.generate_and_build()
