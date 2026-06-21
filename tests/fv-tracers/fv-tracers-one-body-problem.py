# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

"""
This is a convergence test for time integration methods in particle tracing using orbital mechanics.

This simulation implements the classical one-body problem (particle orbiting a fixed central mass)
to systematically test the convergence properties and accuracy of different time integration schemes.
The orbital motion provides an ideal test case because:

- It has a known analytical solution for circular orbits
- It tests integration stability over long time periods
- Orbital energy and angular momentum should be conserved
- Integration errors accumulate and are easily measurable

The particle experiences central gravitational force F = GMm/r² and is initialised with exact
circular orbital conditions. By comparing numerical trajectories against the analytical solution,
this test quantifies:

- Order of accuracy for different integration schemes (Euler, RK, Verlet)
- Long-term stability and energy conservation
- Phase error accumulation in periodic motion

This convergence study validates the temporal discretisation accuracy of the particle
tracing framework for gravitational dynamics.
"""

interpolation_methods = {
    "None": 0,  # No interpolation (use raw values)
    "Constant": 1,  # Piecewise constant interpolation (nearest neighbor)
    "Linear": 2,  # Piecewise linear interpolation (more accurate)
}
integration_schemes = {
    "Static": 0,  # No integration (static particles)
    "Euler": 1,  # Explicit Euler method (simple but less accurate)
    "SemiEuler": 2,  # Semi-implicit Euler method (better stability)
    "Verlet": 3,  # Velocity Verlet algorithm (good for orbital mechanics)
    "Midpoint": 4,  # Midpoint method (2nd order Runge-Kutta)
    "RK3": 5,  # 3rd order Runge-Kutta method (high accuracy)
    "RK4": 6,  # 4th order Runge-Kutta method (higher accuracy)
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
        "-interp", "--interpolation", choices=interpolation_methods.keys(), default="None"
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
    periodic_BC=[False, False, False],
    executable="ExaHyPE",
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
  // System parameters
  constexpr double mu = 10.0; // Gravitational parameter (G*M)

  // Fixed central body position (Domain center)
  constexpr double x_center = 1.5;
  constexpr double y_center = 1.5;

  // Initial position of orbiting body
  constexpr double x0 = 2.0;
  constexpr double y0 = 2.0;

  // Initial velocity of orbiting body (circular orbit)
  const double r = sqrt(pow(x0 - x_center, 2) + pow(y0 - y_center, 2));
  const double omega = sqrt(mu / (r * r * r));

  // Compute velocity perpendicular to radius for circular orbit
  const double vx0 = -omega * (y0 - y_center); // Perpendicular direction (tangential)
  const double vy0 =  omega * (x0 - x_center);

  // Mass of orbiting body
  const double mass = 1.0;

  // Time-dependent angle
  const double angle = atan2(y0 - y_center, x0 - x_center) + omega * t;

  // Position
  solution[0] = x_center + r * cos(angle); // x
  solution[1] = y_center + r * sin(angle); // y

  // Velocity (perpendicular to radius)
  solution[5] = -r * omega * sin(angle); // vx
  solution[6] =  r * omega * cos(angle); // vy

  // Acceleration (central)
  solution[7] = -mu / (r * r * r) * (solution[0] - x_center); // ax
  solution[8] = -mu / (r * r * r) * (solution[1] - y_center); // ay

  // Other fields
  solution[2] = 0.0;  // rho (density)
  solution[3] = 0.0;  // fluid_u
  solution[4] = 0.0;  // fluid_v
  solution[9] = mass; // m (mass)
"""

fv_solver = exahype2.solvers.fv.godunov.GlobalFixedTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns=2,
    auxiliary_variables=0,
    normalised_time_step_size=0.0025 / (args.degrees_of_freedom**0.5),
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
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

tracer_particles = fv_solver.add_tracer(
    name="Tracers", particle_attributes=tracer_attributes
)

init_tracers = exahype2.tracer.InsertParticlesByCoordinatesAndData(
    particle_set=tracer_particles,
    unknowns_per_particle=len(tracer_attributes["unknowns"]),
    initial_data="""
  {2.0, 2.0, 0.0, 0.0, 0.0, -2.65914794847249380538e+00, 2.65914794847249424947e+00, -1.41421356237309474579e+01, -1.41421356237309474579e+01, 1.0}
""",
)

init_tracers.descend_invocation_order = 0
project.add_action_set_to_initialisation(init_tracers)

tracing_action_set = exahype2.tracer.FiniteVolumesTracingWithForce(
    tracer_particles,
    fv_solver,
    project,
    integration_scheme=integration_schemes[args.integration],
    force_method="1",  # Central gravitational force towards the center of the domain
    interpolation_scheme=interpolation_methods[args.interpolation],
)

tracing_action_set.descend_invocation_order = (
    fv_solver._action_set_update_patch.descend_invocation_order + 1
)

project.add_action_set_to_timestepping(tracing_action_set)
project.add_action_set_to_initialisation(tracing_action_set)

from pathlib import Path

error_measurement = exahype2.solvers.fv.ErrorMeasurementParticles(
    fv_solver,
    error_measurement_implementation=analytical_solution,
    output_file_name="Error_" + Path(__file__).name,
    particle_set=tracer_particles,
    descend_invocation_order=65536,  # After all other action sets
)

project = project.generate_Peano4_project()
project.generate_and_build()
