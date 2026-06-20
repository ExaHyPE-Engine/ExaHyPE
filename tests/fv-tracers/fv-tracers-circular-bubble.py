# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

"""
This is a particle tracing simulation for circular bubble dynamics using the Euler equations.

In this one-way coupling setup, the fluid field (circular pressure bubble) affects the particles' motion,
but the particles do not influence the fluid dynamics. The simulation models particle transport
through a circular high-pressure region.
"""


def generate_random_particles(
    file_path, num_particles=10, domain_min=0.1, domain_max=0.9
):
    """
    Generates a .dat file with random x- and y-positions,
    rounding positions to two decimal places.

    Parameters:
    file_path (str): Path to save the .dat file.
    num_particles (int): Number of particles to generate.
    domain_min (float): Minimum bound of the domain.
    domain_max (float): Maximum bound of the domain.
    """
    particles = []
    import random

    for _ in range(num_particles):
        x = round(random.uniform(domain_min, domain_max), 2)
        y = round(random.uniform(domain_min, domain_max), 2)
        particles.append((x, y))

    particles.sort(key=lambda p: p[0])

    with open(file_path, "w") as file:
        for x, y in particles:
            file.write(f"{x:.2f} {y:.2f}\n")

    print(
        f"Generated {num_particles} particles in domain [{domain_min}, {domain_max}] and saved to {file_path}"
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
    periodic_BC=[False, False, False],
    executable="ExaHyPE",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

initial_conditions = """
  Q[Shortcuts::rho]  = 1.0;
  Q[Shortcuts::rhoU] = 0.0;
  Q[Shortcuts::rhoV] = 0.0;
  // Circular bubble centered at (0.5, 0.5) with radius 0.2
  Q[Shortcuts::rhoE] = ((std::sqrt(std::pow(0.5 - x(0), 2) + std::pow(0.5 - x(1), 2)) < 0.2) ? (1.1) : (1.0));
"""

boundary_conditions = """
  // Reflective boundary conditions
  Qoutside[Shortcuts::rho]  = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::rhoU] = -Qinside[Shortcuts::rhoU];
  Qoutside[Shortcuts::rhoV] = -Qinside[Shortcuts::rhoV];
  Qoutside[Shortcuts::rhoE] = Qinside[Shortcuts::rhoE];
"""

compute_primitive_variables = r"""
  const auto irho = 1.0 / Q[Shortcuts::rho];
  const auto u0 = Q[Shortcuts::rhoU] * irho;
  const auto u1 = Q[Shortcuts::rhoV] * irho;

  const auto uSq = u0 * u0 + u1 * u1;
  const auto u_n = (normal == 0) ? u0 : u1;

  const auto internalE = Q[Shortcuts::rhoE] - 0.5 * Q[Shortcuts::rho] * uSq;
  const auto p = (GAMMA - 1.0) * internalE;
"""

max_eigenvalue = r"""
  {compute_primitive_variables}
  const auto speedOfSound = std::sqrt(GAMMA * p * irho);
  auto result = std::fmax(0.0, std::fabs(u_n - speedOfSound));
  result = std::fmax(result, std::fabs(u_n + speedOfSound));
  return result;
""".format(
    compute_primitive_variables=compute_primitive_variables
)

flux = r"""
  {compute_primitive_variables}

  F[Shortcuts::rho] = Q[Shortcuts::rhoU + normal];

  F[Shortcuts::rhoU] = Q[Shortcuts::rhoU] * u_n;
  F[Shortcuts::rhoV] = Q[Shortcuts::rhoV] * u_n;

  F[Shortcuts::rhoU + normal] += p;

  F[Shortcuts::rhoE] = (Q[Shortcuts::rhoE] + p) * u_n;
""".format(
    compute_primitive_variables=compute_primitive_variables
)

fv_solver = exahype2.solvers.fv.godunov.GlobalFixedTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": 1, "rhoV": 1, "rhoE": 1},
    auxiliary_variables=0,
    normalised_time_step_size=0.00064 / (args.degrees_of_freedom**0.5),
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
        "d": 1,
    }
}

tracer_particles = fv_solver.add_tracer(
    name="Tracers", particle_attributes=tracer_attributes
)

generate_random_particles(
    "InitTracers.dat", num_particles=100, domain_min=0.1, domain_max=0.9
)
init_tracers = exahype2.tracer.InsertParticlesFromFile(
    particle_set=tracer_particles,
    filename="InitTracers.dat",
    scale_factor=1.0,
)

init_tracers.descend_invocation_order = 0
project.add_action_set_to_initialisation(init_tracers)

tracing_action_set = exahype2.tracer.FiniteVolumesTracingWithForce(
    tracer_particles,
    fv_solver,
    project,
    integration_scheme=integration_schemes[args.integration],
    interpolation_scheme=interpolation_methods[args.interpolation],
)

tracing_action_set.descend_invocation_order = (
    fv_solver._action_set_update_patch.descend_invocation_order + 1
)

project.add_action_set_to_timestepping(tracing_action_set)
project.add_action_set_to_initialisation(tracing_action_set)

project = project.generate_Peano4_project(verbose=False)
project.output.makefile.add_CXX_flag("-DGAMMA=1.4")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
