# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
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

parser = exahype2.ArgumentParser(
    "ExaHyPE 2 - Finite Volumes Particle Tracing Testing Script"
)

interpolation_methods = {
    "None": 0,  # No interpolation (use raw values)
    "Constant": 1,  # Piecewise constant interpolation (nearest neighbor)
    "Linear": 2,  # Piecewise linear interpolation (more accurate)
}
parser.add_argument(
    "-interp", "--interpolation", default="None", help="|".join(interpolation_methods)
)

integration_schemes = {
    "Static": 0,  # No integration (static particles)
    "Euler": 1,  # Explicit Euler method (simple but less accurate)
    "SemiEuler": 2,  # Semi-implicit Euler method (better stability)
    "Verlet": 3,  # Velocity Verlet algorithm (good for orbital mechanics)
    "Midpoint": 4,  # Midpoint method (2nd order Runge-Kutta)
    "RK3": 5,  # 3rd order Runge-Kutta method (high accuracy)
    "RK4": 6,  # 4th order Runge-Kutta method (higher accuracy)
}
parser.add_argument(
    "-integ", "--integration", default="Euler", help="|".join(integration_schemes)
)

parser.set_defaults(
    min_depth=3,
    degrees_of_freedom=64,
    end_time=2.0,
)

args = parser.parse_args()

size = [3.0, 3.0]
offset = [0.0, 0.0]
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name=".",
    directory=".",
    executable="ExaHyPE",
)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=2,
    size=size,
    offset=offset,
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
    ],
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
    min_volume_h=min_h,
    max_volume_h=max_h,
    normalised_time_step_size=0.0025,
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
    output_file_name=args.output + "/Error_" + Path(__file__).name,
    particle_set=tracer_particles,
    descend_invocation_order=65536,  # After all other action sets
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
