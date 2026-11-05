# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

"""
This is a particle tracing simulation for circular bubble dynamics using the Euler equations.
"""


def generate_random_particles(
    file_path, num_particles=10, domain_min=0.1, domain_max=0.9
):
    """
    Writes num_particles pseudo-random positions with two decimal places into file_path.
    A fixed seed makes every generation write the same positions.
    """
    particles = []
    import random

    generator = random.Random(0)
    for _ in range(num_particles):
        x = round(generator.uniform(domain_min, domain_max), 2)
        y = round(generator.uniform(domain_min, domain_max), 2)
        particles.append((x, y))

    particles.sort(key=lambda p: p[0])

    with open(file_path, "w") as file:
        for x, y in particles:
            file.write(f"{x:.2f} {y:.2f}\n")

    print(
        f"Generated {num_particles} particles in domain [{domain_min}, {domain_max}] and saved to {file_path}"
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
        default="Constant",
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
    size=[1.0, 1.0, 1.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

if project.dimensions != 2:
    raise ValueError("The circular bubble tracer test runs in 2D only.")

initial_conditions = """
  Q.rho  = 1.0;
  Q.rhoU = 0.0;
  Q.rhoE = ((std::sqrt(std::pow(0.5 - x(0), 2) + std::pow(0.5 - x(1), 2)) < 0.2) ? (1.1) : (1.0));
"""

boundary_conditions = """
  Qoutside = Qinside;
  for (int axis = 0; axis < 2; axis++) {
    Qoutside.rhoU(axis) = -Qinside.rhoU(axis);
  }
"""

compute_primitive_variables = r"""
  const auto irho = 1.0 / Q.rho;
  const auto u0 = Q.rhoU(0) * irho;
  const auto u1 = Q.rhoU(1) * irho;

  const auto squareOfU0 = tarch::la::productRoundedOnce(u0, u0);
  const auto squareOfU1 = tarch::la::productRoundedOnce(u1, u1);
  const auto uSq        = squareOfU0 + squareOfU1;
  const auto u_n = (normal == 0) ? u0 : u1;

  const auto internalE = Q.rhoE - 0.5 * Q.rho * uSq;
  const auto p = (GAMMA - 1.0) * internalE;
"""

max_eigenvalue = r"""
  {compute_primitive_variables}
  const auto speedOfSound = std::sqrt(GAMMA * p * irho);
  auto result = std::fmax(0.0, std::fabs(u_n - speedOfSound));
  result = std::fmax(result, std::fabs(u_n + speedOfSound));
  return result;
""".format(compute_primitive_variables=compute_primitive_variables)

flux = r"""
  {compute_primitive_variables}

  F.rho  = Q.rhoU(normal);
  F.rhoU = Q.rhoU * u_n;
  F.rhoU(normal) += p;
  F.rhoE = (Q.rhoE + p) * u_n;
""".format(compute_primitive_variables=compute_primitive_variables)

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
        "fluid_u": 1,
        "fluid_v": 1,
        "u": 1,
        "v": 1,
        "a_x": 1,
        "a_y": 1,
        "d": 1,
    }
}

tracer_particles = project.add_tracer(
    name="Tracers", particle_attributes=tracer_attributes
)

generate_random_particles(
    "InitTracers.dat", num_particles=100, domain_min=0.1, domain_max=0.9
)
tracer_particles.insert_from_file("InitTracers.dat", scale_factor=1.0)

fv_solver.trace(
    tracer_particles,
    forces=exahype2.particles.Forces(
        integration_scheme=integration_schemes[args.integration],
        interpolation_scheme=interpolation_methods[args.interpolation],
    ),
)

project.output.makefile.add_compile_definition("GAMMA=1.4")
project.generate_and_build()
