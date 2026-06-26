# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

"""
This is a particle tracing simulation for volcanic plume rise using the Euler equations.

In this one-way coupling setup, the fluid field (volcanic plume) affects the particles' motion,
but the particles do not influence the fluid dynamics. The simulation models the rise of hot,
buoyant volcanic material through cooler ambient air.
"""


def generate_particles_in_plume(
    file_path: str = "InitTracers.dat",
    num_x: int = 8,
    num_y: int = 8,
    x_min: float = 0.46,
    x_max: float = 0.54,
    y_min: float = 0.11,
    y_max: float = 0.29,
    round_decimals: int = 6,
):
    """
    Generates a .dat file with particles on a uniform grid inside a rectangular plume.

    Parameters:
        file_path (str): Path to save the .dat file.
        num_x (int): Number of points along x (>=1).
        num_y (int): Number of points along y (>=1).
        x_min (float): Minimum x of the plume.
        x_max (float): Maximum x of the plume.
        y_min (float): Minimum y of the plume.
        y_max (float): Maximum y of the plume.
        round_decimals (int): Decimal places for rounding/printing.
    """
    if num_x < 1 or num_y < 1:
        raise ValueError("num_x and num_y must be >= 1")
    if x_min > x_max:
        raise ValueError("x_min must be <= x_max")
    if y_min > y_max:
        raise ValueError("y_min must be <= y_max")

    if num_x == 1:
        x_values = [round((x_min + x_max) / 2.0, round_decimals)]
    else:
        x_step = (x_max - x_min) / (num_x - 1)
        x_values = [round(x_min + i * x_step, round_decimals) for i in range(num_x)]

    if num_y == 1:
        y_values = [round((y_min + y_max) / 2.0, round_decimals)]
    else:
        y_step = (y_max - y_min) / (num_y - 1)
        y_values = [round(y_min + j * y_step, round_decimals) for j in range(num_y)]

    with open(file_path, "w") as file:
        for x in x_values:
            for y in y_values:
                file.write(f"{x:.{round_decimals}f} {y:.{round_decimals}f}\n")

    total = num_x * num_y
    print(f"Generated {total} particles inside plume region and saved to {file_path}")


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
        "-integ", "--integration", choices=integration_schemes.keys(), default="RK4"
    )
    parser.add_argument("-et", "--end-time", type=float, default=3.0)
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

initial_conditions = """
  // Parameters for volcanic plume simulation using defined constants
  Q[4] = 0.0; // Initialise plume indicator

  const double plume_center_x = DomainSize[0] * 0.5;
  const double plume_width = DomainSize[0] * 0.1;
  const double plume_base_y = DomainSize[1] * 0.1;
  const double plume_top_y = DomainSize[1] * 0.3;

  // Identify if x is in the plume region
  bool in_plume_column =
      (x(0) >= plume_center_x - plume_width / 2.0) &&
      (x(0) <= plume_center_x + plume_width / 2.0);

  bool in_plume = in_plume_column &&
                  (x(1) >= plume_base_y) &&
                  (x(1) <= plume_top_y);

  // Compute piecewise hydrostatic pressure
  double p_local = 0.0;

  if (in_plume_column) {
    if (x(1) >= plume_top_y) {
    // Above plume -> full cold column
    p_local = P0 + GRAVITY * (DomainSize[1] - x(1)) * RHO_AMBIENT;
    } else if (x(1) < plume_base_y) {
      // Below plume -> lighter column above
      p_local = P0 +
              GRAVITY * (DomainSize[1] - plume_top_y) * RHO_AMBIENT + // Cold part above plume
              GRAVITY * (plume_top_y - plume_base_y) * RHO_HOT +      // Plume region
              GRAVITY * (plume_base_y - x(1)) * RHO_AMBIENT;          // Local segment below plume
    } else {
      // Inside plume -> partial light column
      p_local = P0 +
              GRAVITY * (DomainSize[1] - plume_top_y) * RHO_AMBIENT + // Cold part above plume
              GRAVITY * (plume_top_y - x(1)) * RHO_HOT;               // Current plume slice
      Q[4] = 1.0;
    }
  } else {
    // Outside plume column -> pure cold fluid
    p_local = P0 + GRAVITY * (DomainSize[1] - x(1)) * RHO_AMBIENT;
  }

  // Energy from pressure
  double e_local = p_local / (GAMMA - 1.0);

  Q[0] = in_plume ? RHO_HOT : RHO_AMBIENT;  // density
  Q[1] = 0.0;                               // rho*u
  Q[2] = 0.0;                               // rho*v
  Q[3] = e_local;                           // total energy
"""

boundary_conditions = """
  if (x(1) >= DomainSize[1] - 0.001) {
    // Top boundary (outflow - zero gradient)
    Qoutside[0] = RHO_AMBIENT;
    Qoutside[1] = 0.0;
    Qoutside[2] = 0.0;
    Qoutside[3] = P0 / (GAMMA - 1.0);
  } else if (x(1) <= DomainOffset[1] + 0.001) {
    // Bottom boundary (reflective)
    Qoutside[0] = Qinside[0];
    Qoutside[1] = Qinside[1];
    Qoutside[2] = -Qinside[2];
    Qoutside[3] = Qinside[3];
  } else if (x(0) <= DomainOffset[0] + 0.001 || x(0) >= DomainSize[0] - 0.001) {
    // Left and right boundaries (outflow - zero gradient)
    Qoutside[0] = Qinside[0];
    Qoutside[1] = Qinside[1];
    Qoutside[2] = Qinside[2];
    Qoutside[3] = Qinside[3];
  }
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

source_term = """
  S[0] = 0.0;
  S[1] = 0.0;
  S[2] = -GRAVITY * Q[0];
  S[3] = -GRAVITY * Q[2];
"""

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": 1, "rhoV": 1, "rhoE": 1},
    auxiliary_variables=0,
    time_step_relaxation=0.4,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
    source_term=source_term,
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

generate_particles_in_plume(
    file_path="InitTracers.dat",
    num_x=8,
    num_y=8,
    x_min=0.46,
    x_max=0.54,
    y_min=0.11,
    y_max=0.29,
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

project.output.makefile.add_compile_definition("GAMMA=1.4")
project.output.makefile.add_compile_definition("GRAVITY=2.0")
project.output.makefile.add_compile_definition("P0=1.0")
project.output.makefile.add_compile_definition("RHO_AMBIENT=1.0")
project.output.makefile.add_compile_definition("RHO_HOT=0.05")
project.generate_and_build()
