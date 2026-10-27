# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="Show this help message and continue with project help.",
    )
    parser.add_argument("-et", "--end-time", type=float, default=0.5)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=4)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=2)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=4)
    args, _ = parser.parse_known_args()
    if args.grid_levels is not None:
        args.min_grid_levels = args.grid_levels
        args.max_grid_levels = args.grid_levels
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="EulerPointExplosion",
    dimensions=2,
    size=[2.0, 2.0, 2.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)

primitives = """
  const double density        = std::max(Q[Shortcuts::rho], 1.0e-12);
  const double inverseDensity = 1.0 / density;
  const double normalVelocity = Q[Shortcuts::rhoU + normal] * inverseDensity;
  const double momentumSquared = tarch::la::norm2Squared(tarch::la::Vector<DIMENSIONS, double>(Q + Shortcuts::rhoU));
  const double pressure =
    std::max((GAMMA - 1.0) * (Q[Shortcuts::rhoE] - 0.5 * momentumSquared * inverseDensity), 1.0e-12);
"""

initial_conditions = """
  Q[Shortcuts::rho] = 1.0;
  for (int d = 0; d < DIMENSIONS; d++) {
    Q[Shortcuts::rhoU + d] = 0.0;
  }
  const tarch::la::Vector<DIMENSIONS, double> centre(1.0);
  constexpr double Radius = 0.15;
  const double pressure = tarch::la::smallerEquals(tarch::la::norm2(x - centre), Radius) ? 5.0 : 1.0;
  Q[Shortcuts::rhoE] = pressure / (GAMMA - 1.0);
"""

boundary_conditions = """
  Qoutside[Shortcuts::rho] = Qinside[Shortcuts::rho];
  for (int d = 0; d < DIMENSIONS; d++) {
    Qoutside[Shortcuts::rhoU + d] = -Qinside[Shortcuts::rhoU + d];
  }
  Qoutside[Shortcuts::rhoE] = Qinside[Shortcuts::rhoE];
"""

flux = """
  {primitives}
  F[Shortcuts::rho] = Q[Shortcuts::rhoU + normal];
  for (int d = 0; d < DIMENSIONS; d++) {{
    F[Shortcuts::rhoU + d] = Q[Shortcuts::rhoU + d] * normalVelocity;
  }}
  F[Shortcuts::rhoU + normal] += pressure;
  F[Shortcuts::rhoE] = (Q[Shortcuts::rhoE] + pressure) * normalVelocity;
""".format(primitives=primitives)

max_eigenvalue = """
  {primitives}
  const double soundSpeed = std::sqrt(GAMMA * pressure * inverseDensity);
  return std::max(std::abs(normalVelocity - soundSpeed), std::abs(normalVelocity + soundSpeed));
""".format(primitives=primitives)

# Density and pressure have the symmetries of the blast and show its fronts.
refinement_criterion = """
  return ::exahype2::refineOnDetail(
    Q,
    ::exahype2::getVolumeCentresInUnitInterval<PatchSize>(),
    NumberOfUnknowns + NumberOfAuxiliaryVariables,
    2.0e-2,
    ::exahype2::DefaultCoarsenSafetyFactor,
    [](const double* const values) { return values[Shortcuts::rho]; },
    [](const double* const values) {
      const double momentumSquared =
        tarch::la::norm2Squared(tarch::la::Vector<DIMENSIONS, double>(values + Shortcuts::rhoU));
      const double density = std::max(values[Shortcuts::rho], 1.0e-12);
      return std::max(
        (GAMMA - 1.0) * (values[Shortcuts::rhoE] - 0.5 * momentumSquared / density),
        1.0e-12
      );
    }
  );
"""

fv_solver = exahype2.fv.singlestep.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": project.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.8),
    scheme=exahype2.fv.Godunov(),
)

fv_solver.set_pde_terms(
    initial_condition=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
)

project.add_solver(fv_solver)

project.output.makefile.add_compile_definition("GAMMA=1.4")
project.generate_and_build()
