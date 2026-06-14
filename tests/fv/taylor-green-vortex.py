# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

from PDE import max_eigenvalue, flux


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=0.5)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=16)
    parser.add_argument("-l", "--grid-levels", type=int, default=6)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

initial_conditions = """
  constexpr double U0 = 1.0; // Initial amplitude of velocity
  constexpr double Pressure0 = 100 / GAMMA; // Reference pressure

  Q[Shortcuts::rho]      = 1.0;
  Q[Shortcuts::rhoU + 0] = U0 * std::sin(x(0)) * std::cos(x(1));
  Q[Shortcuts::rhoU + 1] = -U0 * std::cos(x(0)) * std::sin(x(1));
#if DIMENSIONS == 3
  Q[Shortcuts::rhoU + 2] = 0.0;
#endif

  const double pressure = Pressure0 + (Q[Shortcuts::rho] * U0) * (1.0 / 4.0) * (std::cos(2.0 * x(0)) + std::cos(2.0 * x(1)));
  // Total energy: internal energy + kinetic energy
  Q[Shortcuts::rhoE] = pressure / (GAMMA - 1.0) + 0.5  * (Q[Shortcuts::rhoU + 0] * Q[Shortcuts::rhoU + 0] + Q[Shortcuts::rhoU + 1] * Q[Shortcuts::rhoU + 1]);
"""

boundary_conditions = """
"""

analytical_solution = """
  constexpr double Viscosity = 1.0;
  const double C = 100.0 / GAMMA;

  // Time decay factors for velocity and pressure
  const double expVelocityFactor = std::exp(-2.0 * Viscosity * t); // Time decay factor for velocity
  const double expPressureFactor = std::exp(-4.0 * Viscosity * t); // Time decay factor for pressure

  // Set the initial conditions for the Taylor-Green vortex
  solution[Shortcuts::rho] = 1.0; // Constant density

  // Velocity components with time decay
  solution[Shortcuts::rhoU + 0] = expVelocityFactor * std::sin(x(0)) * std::cos(x(1));
  solution[Shortcuts::rhoU + 1] = -expVelocityFactor * std::cos(x(0)) * std::sin(x(1));
#if DIMENSIONS == 3
  solution[Shortcuts::rhoU + 2] = 0.0;
#endif

  // Pressure with time decay and the given formula
  double pressure = expPressureFactor * (std::cos(2.0 * x(0)) + std::cos(2.0 * x(1)))*(1.0 / 4.0) + C;

  // Total energy: internal energy + kinetic energy
  solution[Shortcuts::rhoE] = pressure / (GAMMA - 1.0) + 0.5 * (Q[Shortcuts::rhoU + 0] * Q[Shortcuts::rhoU + 0] + Q[Shortcuts::rhoU + 1] * Q[Shortcuts::rhoU + 1]);
"""

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="TaylorGreenVortex",
    dimensions=2,
    size=[2 * 3.14159265359, 2 * 3.14159265359, 2 * 3.14159265359],
    offset=[0.0, 0.0, 0.0],
    periodic_BC=[True, True, True],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

fv_solver = exahype2.solvers.fv.musclhancock.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": project.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    time_step_relaxation=0.5,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
    limiter=exahype2.solvers.fv.musclhancock.Limiter.vanalbada,
)

exahype2.solvers.fv.ErrorMeasurement(
    fv_solver,
    error_measurement_implementation=analytical_solution,
    output_file_name="Error",
)

project.add_solver(fv_solver)

project = project.generate_Peano4_project(verbose=False)
project.output.makefile.add_CXX_flag("-DGAMMA=1.4")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
