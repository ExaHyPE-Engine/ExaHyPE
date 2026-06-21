# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

"""
This is a particle settling simulation with buoyancy and drag forces.

This simulation models the dynamics of spherical particles settling under the combined
influence of gravity, buoyancy, and Stokes drag forces. The particles experience:

- Gravitational force: F_g = mg (downward)
- Buoyancy force: F_b = ρ_fluid * V * g (upward)
- Stokes drag: F_d = 6πμRv (opposing motion)

The analytical solution describes the exponential approach to terminal velocity:
v(t) = -v_s * (1 - e^(-t/τ)),
where v_s is terminal velocity and τ is the relaxation time.

This provides a validation test for:

- Multi-physics force integration (gravity + buoyancy + drag)
- Particle-fluid interaction through interpolated density fields
- Transient dynamics approaching steady-state terminal velocity

Particles with different masses and radii are initialised to test the scaling
of settling behavior with particle properties.
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
        "-interp",
        "--interpolation",
        choices=interpolation_methods.keys(),
        default="Linear",
    )
    parser.add_argument(
        "-integ",
        "--integration",
        choices=integration_schemes.keys(),
        default="SemiEuler",
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
  Q[0] = 1.0;
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
  constexpr double g  = 1.0; // Gravitational acceleration
  constexpr double mu = 1.0; // Dynamic viscosity

  // Pull particle properties
  const double mass     = particle->getData(7);
  const double radius   = particle->getData(8);

  // Compute volume & densities
  double volume             = (4.0/3.0) * M_PI * radius*radius*radius;
  double rho_particle       = mass / volume;
  double rho_fluid          = 1.0;
  double density_difference = rho_particle - rho_fluid;

  // Buoyant acceleration (effective g)
  const double a_eff = density_difference * g / rho_particle;
  // Stokes drag relaxation time
  const double tau = mass / (6.0 * M_PI * mu * radius);
  // Terminal speed (buoyancy + drag)
  const double vs = a_eff * tau;

  // Transient solution
  const double expTerm = std::exp(- t / tau);
  const double vy      = -vs * (1.0 - expTerm);
  const double y       = 2.5 + (
                          - vs * t    // Long-term drift
                          + vs * tau  // Offset so y(0)=2.5
                          * (1.0 - expTerm)
                        );

  solution[0] = 1.5;    // x(t)=const
  solution[1] = y;      // y(t)

  solution[2] = rho_fluid;
  solution[3] = 0.0;    // Fluid u
  solution[4] = 0.0;    // Fluid v

  solution[5] = 0.0;    // Particle u
  solution[6] = vy;     // Particle v(t)

  solution[7] = mass;   // Keep mass at index 7
  solution[8] = radius; // Keep radius at index 8
"""

fv_solver = exahype2.solvers.fv.godunov.GlobalFixedTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns=3,
    auxiliary_variables=0,
    normalised_time_step_size=0.001 / (args.degrees_of_freedom**0.5),
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
        "d": 1,
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
    2.0, 2.5, 0.0, 0.0, 0.0, 0, 0, 0, 0.0, 0.01, 0.1,
    1.5, 2.5, 0.0, 0.0, 0.0, 0, 0, 0, 0.0, 0.1,  0.1,
    1.0, 2.5, 0.0, 0.0, 0.0, 0, 0, 0, 0.0, 0.5,  0.1,
    0.5, 2.5, 0.0, 0.0, 0.0, 0, 0, 0, 0.0, 1.0,  0.1
  }
""",
)

init_tracers.descend_invocation_order = 0
project.add_action_set_to_initialisation(init_tracers)

tracing_action_set = exahype2.tracer.FiniteVolumesTracingWithForce(
    tracer_particles,
    fv_solver,
    project,
    integration_scheme=integration_schemes[args.integration],
    # Force method bitmask: 14 = 2 (Buoyancy) + 4 (Drag) + 8 (ConstantGravity)
    # This enables three forces acting on particles:
    # - Buoyancy: upward force based on fluid density
    # - Drag: resistance based on relative velocity
    # - Gravity: constant downward force
    force_method=14,
    interpolation_scheme=interpolation_methods[args.interpolation],
    # Field interpolation from fluid solver to particle data:
    # "0,1,2" maps solver fields to particle storage as follows:
    #   Solver Q[0] → particle.getData(0): fluid density (ρ)
    #     - Used for buoyancy force calculation: F_b = ρ*V*g
    #   Solver Q[1] → particle.getData(1): fluid velocity x-component (u_x)
    #     - Used for drag force calculation
    #   Solver Q[2] → particle.getData(2): fluid velocity y-component (u_y)
    #     - Used for drag force calculation
    # These values are interpolated at each particle position from the
    # surrounding fluid cells to enable accurate force calculations
    fields_to_interpolate="0,1,2",
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
