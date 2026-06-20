# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

"""
This is a particle tracing convergence test for radial jet flow.

This simulation implements a radial jet velocity field v = r/r² emanating from a central point,
designed to test the convergence properties of different interpolation schemes. In this one-way
coupling setup, particles are advected through the analytical velocity field, allowing for
precise comparison between numerical and analytical solutions.

The radial jet provides an ideal test case because:

- It has a known analytical solution for particle trajectories
- It tests interpolation accuracy across varying field gradients
- It validates integration scheme performance in divergent flow fields

Particles are initialised radially around the jet center and their trajectories are compared
against the analytical solution to measure interpolation and integration errors.
"""


def generate_radial_particles(
    file_path,
    num_rays=20,
    particles_per_ray=50,
    r_min=0.05,
    r_max=1.0,
    center_x=1.5,
    center_y=1.5,
):
    """
    Generates a .dat file with particles arranged radially from the domain center.

    Parameters:
        file_path (str): Path to save the .dat file.
        num_rays (int): Number of angular directions (rays).
        particles_per_ray (int): Number of particles along each ray.
        r_min (float): Minimum distance from center (to avoid singularity).
        r_max (float): Maximum distance from center.
        center_x (float): X-coordinate of the radial center.
        center_y (float): Y-coordinate of the radial center.
    """
    import math

    with open(file_path, "w") as file:
        for i in range(num_rays):
            theta = 2 * math.pi * i / num_rays  # Equally spaced angles
            for j in range(particles_per_ray):
                r = r_min + j * (r_max - r_min) / (particles_per_ray - 1)
                x = center_x + r * math.cos(theta)
                y = center_y + r * math.sin(theta)
                file.write(f"{x:.6f} {y:.6f}\n")

    total = num_rays * particles_per_ray
    print(
        f"Generated {total} radial particles in a 3x3 domain centered at ({center_x},{center_y}) and saved to {file_path}"
    )


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
        "-integ", "--integration", choices=integration_schemes.keys(), default="RK4"
    )
    parser.add_argument("-et", "--end-time", type=float, default=0.8)
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
  constexpr double Epsilon = 1e-3;
  const double x0 = 1.5; // Jet center x
  const double y0 = 1.5; // Jet center y
  const double dx = x(0) - x0;
  const double dy = x(1) - y0;
  const double r2 = dx * dx + dy * dy + Epsilon * Epsilon;

  Q[0] = 1.0;        // rho
  Q[1] = dx / r2;    // rho * u
  Q[2] = dy / r2;    // rho * v
  Q[3] = 1.0;        // rho * e (just constant baseline)
"""

boundary_conditions = """
  // Reflective boundary conditions
  Qoutside[0] = Qinside[0];
  Qoutside[1] = -Qinside[1];
  Qoutside[2] = -Qinside[2];
  Qoutside[3] = Qinside[3];
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
  // Analytical solution for radial jet with velocity field v = (x/r^2, y/r^2)
  constexpr double Epsilon = 1e-3;

  // Jet center
  const double x_c = 1.5;
  const double y_c = 1.5;

  // Initial position relative to center
  const double dx0 = particle->getData(1) - x_c;
  const double dy0 = particle->getData(2) - y_c;

  // Initial squared distance
  const double r0_2 = dx0 * dx0 + dy0 * dy0 + Epsilon * Epsilon;

  // Current distance scaling
  const double r_t_2 = r0_2 + 2.0 * t;      // r(t)^2 = r0^2 + 2t
  const double scale = sqrt(r_t_2 / r0_2);  // r(t)/r0

  // Updated position
  solution[0] = x_c + dx0 * scale; // x(t)
  solution[1] = y_c + dy0 * scale; // y(t)

  // Fluid velocity at particle position at time t
  const double inv_rt2 = 1.0 / r_t_2;
  solution[3] = (solution[0] - x_c) * inv_rt2; // u(x(t), y(t))
  solution[4] = (solution[1] - y_c) * inv_rt2; // v(x(t), y(t))

  // Particle velocity (same as fluid)
  solution[5] = dx0 / r_t_2;  // u(t) = dx0 / (r0^2 + 2t)
  solution[6] = dy0 / r_t_2;  // v(t) = dy0 / (r0^2 + 2t)

  // Acceleration
  solution[7] = 0.0;
  solution[8] = 0.0;

  solution[2] = 1.0; // rho
  solution[9] = 1.0; // mass
"""

fv_solver = exahype2.solvers.fv.godunov.GlobalFixedTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "u": 1, "v": 1, "e": 1},
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
        "init_x": 1,
        "init_y": 1,  # Interpolated fluid velocity
        "u": 1,
        "v": 1,  # Particle's own velocity
        "a_x": 1,
        "a_y": 1,
        "d": 1,
    }
}

tracer_particles = fv_solver.add_tracer(
    name="Tracers", particle_attributes=tracer_attributes
)

generate_radial_particles(
    "InitTracers.dat", num_rays=36, particles_per_ray=5, r_min=0.1, r_max=0.5
)
init_action_set = exahype2.tracer.InsertParticlesFromFile(
    particle_set=tracer_particles,
    filename="InitTracers.dat",
    scale_factor=1.0,
    save_initialpos=True,
)

init_action_set.descend_invocation_order = 0
project.add_action_set_to_initialisation(init_action_set)

tracing_action_set = exahype2.tracer.FiniteVolumesTracingWithForce(
    tracer_particles,
    fv_solver,
    project,
    integration_scheme=integration_schemes[args.integration],
    interpolation_scheme=interpolation_methods[args.interpolation],
    force_method=0,
    fields_to_interpolate="0,3,4",
)

tracing_action_set.descend_invocation_order = (
    fv_solver._action_set_update_patch.descend_invocation_order + 1
)

project.add_action_set_to_timestepping(tracing_action_set)

from pathlib import Path

error_measurement = exahype2.solvers.fv.ErrorMeasurementParticles(
    fv_solver,
    error_measurement_implementation=analytical_solution,
    output_file_name="Error_" + Path(__file__).name,
    particle_set=tracer_particles,
    descend_invocation_order=65536,  # After all other action sets
)

project = project.generate_Peano4_project(verbose=False)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
