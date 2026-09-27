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
  // Total energy: Internal energy plus kinetic energy.
  Q[Shortcuts::rhoE] = pressure / (GAMMA - 1.0) + 0.5  * (Q[Shortcuts::rhoU + 0] * Q[Shortcuts::rhoU + 0] + Q[Shortcuts::rhoU + 1] * Q[Shortcuts::rhoU + 1]);
"""

analytical_solution = """
  constexpr double Viscosity = 1.0;
  const double C = 100.0 / GAMMA;

  // Time decay factors for velocity and pressure
  const double expVelocityFactor = std::exp(-2.0 * Viscosity * t);
  const double expPressureFactor = std::exp(-4.0 * Viscosity * t);

  solution[Shortcuts::rho] = 1.0;

  solution[Shortcuts::rhoU + 0] = expVelocityFactor * std::sin(x(0)) * std::cos(x(1));
  solution[Shortcuts::rhoU + 1] = -expVelocityFactor * std::cos(x(0)) * std::sin(x(1));
#if DIMENSIONS == 3
  solution[Shortcuts::rhoU + 2] = 0.0;
#endif

  double pressure = expPressureFactor * (std::cos(2.0 * x(0)) + std::cos(2.0 * x(1)))*(1.0 / 4.0) + C;

  // Total energy: Internal energy plus kinetic energy.
  solution[Shortcuts::rhoE] = pressure / (GAMMA - 1.0) + 0.5 * (solution[Shortcuts::rhoU + 0] * solution[Shortcuts::rhoU + 0] + solution[Shortcuts::rhoU + 1] * solution[Shortcuts::rhoU + 1]);
"""

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="TaylorGreenVortex",
    dimensions=2,
    size=[2 * 3.14159265359, 2 * 3.14159265359, 2 * 3.14159265359],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[True, True, True],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

fv_solver = exahype2.fv.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": project.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    scheme=exahype2.fv.MusclHancock(limiter=exahype2.fv.Limiter.vanalbada),
)

fv_solver.set_pde_terms(
    initial_condition=initial_conditions,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
    analytical_solution=analytical_solution,
)

fv_solver.measure_error(file="Error", every=args.end_time / 100.0)

project.add_solver(fv_solver)

project.output.makefile.add_compile_definition("GAMMA=1.4")
project.generate_and_build()
