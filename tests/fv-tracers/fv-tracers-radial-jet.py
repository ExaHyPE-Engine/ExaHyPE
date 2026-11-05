# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""
A convergence test of the particle tracing in a radial jet flow.
"""

import argparse
import math
from pathlib import Path

import exahype2


def generate_radial_particles(
    file_path,
    num_rays=20,
    particles_per_ray=50,
    r_min=0.05,
    r_max=1.0,
    centre_x=0.0,
    centre_y=0.0,
):
    """
    Generates a .dat file with particle positions on rays around (centre_x, centre_y).
    Every ray lies between two axes. A tracer on an axis of a Hilbert grid reads only the
    cell below its face.
    """
    with open(file_path, "w") as file:
        for i in range(num_rays):
            theta = 2 * math.pi * (i + 0.5) / num_rays
            for j in range(particles_per_ray):
                r = r_min + j * (r_max - r_min) / (particles_per_ray - 1)
                x = centre_x + r * math.cos(theta)
                y = centre_y + r * math.sin(theta)
                file.write(f"{x:.6f} {y:.6f}\n")

    total = num_rays * particles_per_ray
    print(
        f"Generated {total} radial particles in a 3x3 domain centred at ({centre_x},{centre_y}) and saved to {file_path}"
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
    offset=[-1.5, -1.5, -1.5],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

if project.dimensions != 2:
    raise ValueError("The radial jet tracer test runs in 2D only.")

initial_conditions = """
  constexpr double Epsilon = 1e-3;
  constexpr double CentreX = 0.0;
  constexpr double CentreY = 0.0;

  const double offsetX       = x(0) - CentreX;
  const double offsetY       = x(1) - CentreY;
  const double radiusSquared = tarch::la::productRoundedOnce(offsetX, offsetX)
                             + tarch::la::productRoundedOnce(offsetY, offsetY) + Epsilon * Epsilon;

  Q[Shortcuts::rho] = 1.0;
  Q[Shortcuts::u]   = offsetX / radiusSquared;
  Q[Shortcuts::v]   = offsetY / radiusSquared;
  Q[Shortcuts::e]   = 1.0;
"""

boundary_conditions = """
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
  // A tracer in the velocity field (x - CentreX, y - CentreY) / r^2 moves along its ray with
  // r(t)^2 = r(0)^2 + 2t. The entries of solution are the position and the tracer data of
  // tracer_attributes, with zero mass and acceleration for a tracer.
  constexpr double Epsilon = 1e-3;
  constexpr double CentreX = 0.0;
  constexpr double CentreY = 0.0;

  const double initialX             = particle->getData(1);
  const double initialY             = particle->getData(2);
  const double initialOffsetX       = initialX - CentreX;
  const double initialOffsetY       = initialY - CentreY;
  const double initialRadiusSquared = initialOffsetX * initialOffsetX + initialOffsetY * initialOffsetY + Epsilon * Epsilon;
  const double radiusSquared        = initialRadiusSquared + 2.0 * t;
  const double scale                = std::sqrt(radiusSquared / initialRadiusSquared);

  solution[0] = CentreX + initialOffsetX * scale;
  solution[1] = CentreY + initialOffsetY * scale;
  solution[2] = 1.0;
  solution[3] = initialX;
  solution[4] = initialY;
  solution[5] = initialOffsetX * scale / radiusSquared;
  solution[6] = initialOffsetY * scale / radiusSquared;
  solution[7] = 0.0;
  solution[8] = 0.0;
  solution[9] = 0.0;
"""

fv_solver = exahype2.fv.singlestep.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "u": 1, "v": 1, "e": 1},
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
        # The interpolation overwrites rho, u and v of each tracer with the fluid state.
        fields_to_interpolate="0,3,4",
    ),
    initialisation=False,
)

fv_solver.set_pde_terms(particle_analytical_solution=analytical_solution)
fv_solver.measure_particle_error(tracer_particles, file="Error_" + Path(__file__).name)

project.generate_and_build()
