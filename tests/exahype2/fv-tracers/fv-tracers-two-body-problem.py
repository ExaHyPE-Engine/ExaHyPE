# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

"""
This is a convergence test for N-body gravitational interactions using the classical two-body problem.

This simulation models two particles orbiting around their common center of mass under mutual
gravitational attraction, designed to validate N-body interaction algorithms and test the
convergence properties of integration schemes for multi-particle systems. The two-body problem
provides an ideal test case because:

- It has exact analytical solutions for circular orbits
- Both particles follow predictable elliptical/circular trajectories
- The center of mass remains stationary (momentum conservation)
- Total energy and angular momentum are conserved quantities

The particles experience mutual gravitational forces F₁₂ = -F₂₁ = Gm₁m₂r̂/r², and are initialised
with masses and velocities calculated for exact circular motion about their barycenter.
This test quantifies:

- Accuracy of particle-particle force calculations
- Conservation of physical quantities over long time periods
- Integration stability for coupled multi-body dynamics
- Convergence order of different temporal integration schemes

This validation case ensures the N-body interaction framework correctly handles gravitational
dynamics between the two particles.
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

gravity_models = {
    "None": 0,  # No gravity (straight-line motion)
    "Central": 1,  # Central gravitational force (1/r²) from a fixed point
    "Custom": 2,  # User-defined gravitational field
}
parser.add_argument(
    "-grav", "--gravity", default="Central", help="|".join(gravity_models)
)

parser.set_defaults(
    min_depth=3,
    degrees_of_freedom=64,
    end_time=2.0,
)

args = parser.parse_args()

size = [2.0, 2.0]
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
  constexpr double G = 0.1; // Gravitational constant

  // Initial positions of particles
  constexpr double x1_0 = 1.0; // Initial x-position of particle 1
  constexpr double y1_0 = 1.0; // Initial y-position of particle 1
  constexpr double x2_0 = 1.4; // Initial x-position of particle 2
  constexpr double y2_0 = 1.4; // Initial y-position of particle 2

  // Particle masses
  constexpr double m1 = 10.0; // Mass of particle 1
  constexpr double m2 = 1.0; // Mass of particle 2

  // Distance between initial positions of particles
  constexpr double dx0 = x2_0 - x1_0;
  constexpr double dy0 = y2_0 - y1_0;
  const double R = sqrt(dx0*dx0 + dy0*dy0);

  // Center of mass position
  const double cm_x = (m1*x1_0 + m2*x2_0) / (m1 + m2);
  const double cm_y = (m1*y1_0 + m2*y2_0) / (m1 + m2);

  // Calculate orbit radii (distances from center of mass)
  const double r1 = m2 / (m1 + m2) * R; // Radius for particle 1
  const double r2 = m1 / (m1 + m2) * R; // Radius for particle 2

  // Calculate initial phase angles
  const double phi1 = atan2(y1_0 - cm_y, x1_0 - cm_x);
  const double phi2 = atan2(y2_0 - cm_y, x2_0 - cm_x);

  // Calculate omega (angular velocities)
  double omega = sqrt(G * (m1 + m2) / (R * R * R ));

  // Use particle mass to determine which particle it is
  double mass = particle->getData(7);

  // Get phit and ri based on mass
  double phit = omega*t + (mass == m1 ? phi1 : phi2);
  double ri   =           (mass == m1 ? r1   : r2  );

  // Calculate positions for both particles at time t
  double x = cm_x + ri * cos(phit);
  double y = cm_y + ri * sin(phit);

  // Calculate velocities for both particles at time t
  double vx = -omega * ri * sin(phit);
  double vy =  omega * ri * cos(phit);

  // Calculate accelerations for both particles at time t
  double ax = -omega * omega * ri * cos(phit);
  double ay = -omega * omega * ri * sin(phit);

  // Fill the solution vector
  solution[0] = x;
  solution[1] = y;
  solution[2] = 0.0;
  solution[3] = 0.0;
  solution[4] = 0.0;
  solution[5] = vx;
  solution[6] = vy;
  solution[7] = ax;
  solution[8] = ay;
  // Set the mass of the particle
  solution[9] = mass;
"""

fv_solver = exahype2.solvers.fv.godunov.GlobalFixedTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns=3,
    auxiliary_variables=0,
    min_volume_h=min_h,
    max_volume_h=max_h,
    normalised_time_step_size=0.01,
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
  {
    1.4, 1.4, 0.0, 0.0, 0.0, -0.8963985905647842, 0.8963985905647845,   -2.2070312500000000, -2.2070312500000000, 1.0,
    1.0, 1.0, 0.0, 0.0, 0.0, 0.08963985905647845, -0.08963985905647842, 0.2207031250000000,  0.2207031250000000,  10.0
  }
""",
)

init_tracers.descend_invocation_order = 0
project.add_action_set_to_initialisation(init_tracers)

tracing_action_set = exahype2.tracer.NBodyInteraction(
    tracer_particles,
    fv_solver,
    project,
    integration_scheme=integration_schemes[args.integration],
    interpolation_scheme=interpolation_methods[args.interpolation],
    gravity_model=gravity_models[args.gravity],
)

tracing_action_set.descend_invocation_order = (
    fv_solver._action_set_update_patch.descend_invocation_order + 1
)

project.add_action_set_to_timestepping(tracing_action_set)

from pathlib import Path

error_measurement = exahype2.solvers.fv.ErrorMeasurementParticles(
    fv_solver,
    error_measurement_implementation=analytical_solution,
    output_file_name=args.output + "/Error_" + Path(__file__).name,
    particle_set=tracer_particles,
    descend_invocation_order=65536,  # After all other action sets
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
project.set_Peano4_installation(
    "../../../", mode=peano4.output.string_to_mode(args.build_mode)
)
project = project.generate_Peano4_project(verbose=False)
project.set_fenv_handler(args.fpe)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
