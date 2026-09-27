# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse
from pathlib import Path

import exahype2

"""
This is a particle tracing convergence test for linear plume rise.
"""


def generate_particles_on_vertical_line(
    file_path, num_particles=100, x_fixed=0.5, y_min=0.1, y_max=0.9
):
    """
    Generates a .dat file with particles uniformly spaced along a vertical line at x = x_fixed.
    """
    if num_particles < 2:
        y_values = [0.5]
    else:
        step = (y_max - y_min) / (num_particles - 1)
        y_values = [round(y_min + i * step, 4) for i in range(num_particles)]

    with open(file_path, "w") as file:
        for y in y_values:
            file.write(f"{x_fixed:.4f} {y:.4f}\n")

    print(
        f"Generated {num_particles} particles along vertical line at x = {x_fixed} and saved to {file_path}"
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
        "-integ",
        "--integration",
        choices=integration_schemes.keys(),
        default="SemiEuler",
    )
    parser.add_argument("-et", "--end-time", type=float, default=1.0)
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
  Q[Shortcuts::rho]       = 1.0;
  Q[Shortcuts::rhoU + 0]  = 0.0;        // rho * u_x
  Q[Shortcuts::rhoU + 1]  = 0.5 * x(1); // rho * u_y
  Q[Shortcuts::rhoE]      = 1.0;        // Simple energy (rho*e)
"""

boundary_conditions = """
  // Reflective boundary conditions
  Qoutside[Shortcuts::rho]      = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::rhoU + 0] = -Qinside[Shortcuts::rhoU + 0];
  Qoutside[Shortcuts::rhoU + 1] = -Qinside[Shortcuts::rhoU + 1];
  Qoutside[Shortcuts::rhoE]     = Qinside[Shortcuts::rhoE];
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
  // Analytical solution for linear plume rise with exponential velocity growth in y-direction
  constexpr double VelocityGradient = 0.5;

  // Initial particle position (x0, y0)
  const double x0 = particle->getData(1); // x doesn't change
  const double y0 = particle->getData(2); // The analytical y(t) grows from y0

  // Time-dependent solution
  solution[0] = x0;                                                // x(t) = x0 (no x motion)
  solution[1] = y0 * exp(VelocityGradient * t);                    // y(t)

  // Fluid velocity field at particle position
  solution[3] = 0.0;                                               // fluid_u (x-direction)
  solution[4] = VelocityGradient * solution[1];                    // fluid_v

  // Particle velocity (matches fluid)
  solution[5] = 0.0;                                               // u = dx/dt
  solution[6] = VelocityGradient * solution[1];                    // v = dy/dt

  // Particle acceleration
  solution[7] = 0.0;                                               // a_x
  solution[8] = VelocityGradient * VelocityGradient * solution[1]; // a_y = d^2y/dt^2

  solution[2] = 1.0; // rho
  solution[9] = 1.0; // mass
"""

fv_solver = exahype2.fv.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": 2, "rhoE": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Fixed(0.01 / (args.degrees_of_freedom**0.5)),
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
        "init_y": 1,
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

generate_particles_on_vertical_line(
    "InitTracers.dat", num_particles=1000, x_fixed=1.5, y_min=0.1, y_max=0.7
)
tracer_particles.insert_from_file(
    "InitTracers.dat", scale_factor=1.0, save_initial_position=True
)

fv_solver.trace(
    tracer_particles,
    forces=exahype2.particles.Forces(
        integration_scheme=integration_schemes[args.integration],
        interpolation_scheme=interpolation_methods[args.interpolation],
    ),
    initialisation=False,
)

fv_solver.set_pde_terms(particle_analytical_solution=analytical_solution)
fv_solver.measure_particle_error(tracer_particles, file="Error_" + Path(__file__).name)

project.generate_and_build()
