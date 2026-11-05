# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""
A convergence test of the particle tracing in a linearly rising plume.
"""

import argparse
from pathlib import Path

import exahype2


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
        f"Generated {num_particles} particles along a vertical line at x = {x_fixed} and saved to {file_path}"
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
        help="Show this help message and continue with project help.",
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

if project.dimensions != 2:
    raise ValueError("The vertical plume tracer test runs in 2D only.")

initial_conditions = """
  Q.rho     = 1.0;
  Q.rhoU(0) = 0.0;
  Q.rhoU(1) = 0.5 * x(1);
  Q.rhoE    = 1.0;
"""

boundary_conditions = """
  Qoutside = Qinside;
  for (int axis = 0; axis < 2; axis++) {
    Qoutside.rhoU(axis) = -Qinside.rhoU(axis);
  }
"""

flux = """
  F = {};
"""

max_eigenvalue = """
  return 1.0;
"""

analytical_solution = """
  // The tracers move in the fluid velocity field (0, VelocityGradient * y). The solution sets the
  // mass and the acceleration of a tracer to zero.
  constexpr double VelocityGradient = 0.5;

  const double initialX = particle.getData(1);
  const double initialY = particle.getData(2);

  solution.position = {initialX, initialY * std::exp(VelocityGradient * t)};
  solution.rho      = 1.0;
  solution.init_x   = initialX;
  solution.init_y   = initialY;
  solution.u        = 0.0;
  solution.v        = VelocityGradient * solution.position(1);
  solution.a_x      = 0.0;
  solution.a_y      = 0.0;
  solution.mass     = 0.0;
"""

fv_solver = exahype2.fv.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": 2, "rhoE": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
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
        "v": 1,
        "a_x": 1,
        "a_y": 1,
        "mass": 1,
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
