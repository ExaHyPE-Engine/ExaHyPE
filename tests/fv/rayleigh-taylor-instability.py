# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
"""Single-mode Rayleigh-Taylor instability with the WENO finite-volume solver.

A heavy fluid rests on a light one under gravity, and a cosine perturbation of
the interface seeds the instability. The pressure is hydrostatic, so the layers
stay at rest until the perturbation grows. The setup follows the ADER-DG case
in tests/aderdg-limiting/euler-rayleigh-taylor.py.
"""
import argparse

import exahype2

from PDE import max_eigenvalue, flux, source_term


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="Show this help message and continue with project help.",
    )
    parser.add_argument("-et", "--end-time", type=float, default=2.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=16)
    parser.add_argument("-l", "--grid-levels", type=int, default=3)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

initial_conditions = """
  constexpr double RhoLight = 1.0;
  constexpr double RhoHeavy = 2.0;
  constexpr double InterfacePressure = 5.0;
  constexpr double InterfaceHeight = 1.0;
  constexpr int Vertical = 1;

  const double horizontalShape = std::cos(2.0 * tarch::la::PI * (x(0) - 0.5));
  const double perturbedInterface = InterfaceHeight + 0.06 * horizontalShape;
  const double density = x(Vertical) < perturbedInterface ? RhoLight : RhoHeavy;
  const double distanceToInterface = x(Vertical) - InterfaceHeight;
  const double pressure =
    x(Vertical) < InterfaceHeight
      ? InterfacePressure - RhoLight * GRAVITY * distanceToInterface
      : InterfacePressure - RhoHeavy * GRAVITY * distanceToInterface;
  const double relativeInterfaceDistance = distanceToInterface / 2.0;
  const double verticalVelocity =
    0.03 * std::sqrt(2.0 * GRAVITY) * horizontalShape
    * std::exp(-100.0 * relativeInterfaceDistance * relativeInterfaceDistance);

  Q[Shortcuts::rho]      = density;
  Q[Shortcuts::rhoU + 0] = 0.0;
  Q[Shortcuts::rhoU + 1] = density * verticalVelocity;
  Q[Shortcuts::rhoE]     = pressure / (GAMMA - 1.0) + 0.5 * density * verticalVelocity * verticalVelocity;
"""

boundary_conditions = """
  // The lower and upper boundaries are walls; the horizontal direction is periodic.
  Qoutside[Shortcuts::rho]      = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::rhoU + 0] = Qinside[Shortcuts::rhoU + 0];
  Qoutside[Shortcuts::rhoU + 1] = -Qinside[Shortcuts::rhoU + 1];
  Qoutside[Shortcuts::rhoE]     = Qinside[Shortcuts::rhoE];
"""

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="RayleighTaylorInstability",
    dimensions=2,
    size=[1.0, 2.0, 1.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[True, False, True],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

fv_solver = exahype2.solvers.fv.weno.GlobalAdaptiveTimeStep(
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
    source_term=source_term,
)

project.add_solver(fv_solver)

project.output.makefile.add_compile_definition("GAMMA=1.4")
project.output.makefile.add_compile_definition("GRAVITY=1.0")
project.generate_and_build()
