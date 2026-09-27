# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
from pathlib import Path

import exahype2

"""
This is a particle tracing convergence test for radial jet flow.
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
    Generates a .dat file with particles arranged radially from the domain centre.
    """
    import math

    with open(file_path, "w") as file:
        for i in range(num_rays):
            theta = 2 * math.pi * i / num_rays
            for j in range(particles_per_ray):
                r = r_min + j * (r_max - r_min) / (particles_per_ray - 1)
                x = center_x + r * math.cos(theta)
                y = center_y + r * math.sin(theta)
                file.write(f"{x:.6f} {y:.6f}\n")

    total = num_rays * particles_per_ray
    print(
        f"Generated {total} radial particles in a 3x3 domain centred at ({center_x},{center_y}) and saved to {file_path}"
    )


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
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

initial_conditions = """
  constexpr double Epsilon = 1e-3;
  const double x0 = 1.5; // Jet centre x
  const double y0 = 1.5; // Jet centre y
  const double dx = x(0) - x0;
  const double dy = x(1) - y0;
  const double r2 = dx * dx + dy * dy + Epsilon * Epsilon;

  Q[0] = 1.0;        // rho
  Q[1] = dx / r2;    // rho * u
  Q[2] = dy / r2;    // rho * v
  Q[3] = 1.0;        // rho * e (constant baseline)
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

  // Jet centre
  const double x_c = 1.5;
  const double y_c = 1.5;

  // Initial position relative to centre
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

fv_solver = exahype2.fv.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "u": 1, "v": 1, "e": 1},
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
        "init_x": 1,
        "init_y": 1,  # Interpolated fluid velocity
        "u": 1,
        "v": 1,  # Particle's own velocity
        "a_x": 1,
        "a_y": 1,
        "d": 1,
    }
}

tracer_particles = project.add_tracer(
    name="Tracers", particle_attributes=tracer_attributes
)

generate_radial_particles(
    "InitTracers.dat", num_rays=36, particles_per_ray=5, r_min=0.1, r_max=0.5
)
tracer_particles.insert_from_file(
    "InitTracers.dat", scale_factor=1.0, save_initial_position=True
)

fv_solver.trace(
    tracer_particles,
    forces=exahype2.particles.Forces(
        integration_scheme=integration_schemes[args.integration],
        interpolation_scheme=interpolation_methods[args.interpolation],
        force_method=0,
        fields_to_interpolate="0,3,4",
    ),
    initialisation=False,
)

fv_solver.set_pde_terms(particle_analytical_solution=analytical_solution)
fv_solver.measure_particle_error(tracer_particles, file="Error_" + Path(__file__).name)

project.generate_and_build()
