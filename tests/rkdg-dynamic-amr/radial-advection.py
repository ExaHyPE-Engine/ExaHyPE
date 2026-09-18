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
    parser.add_argument("-et", "--end-time", type=float, default=0.4)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=5)
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
    namespace=["tests", "exahype2", "rkdg"],
    project_name="RadialAdvection",
    dimensions=2,
    size=[2.0, 2.0, 2.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)

initial_conditions = """
  const tarch::la::Vector<DIMENSIONS, double> centre(1.0);
  constexpr double BumpRadius = 0.3;
  const auto scaledRadiusSquared =
    tarch::la::norm2Squared(x - centre) / (BumpRadius * BumpRadius);
  Q[Shortcuts::rho] = 0.0;
  if (scaledRadiusSquared < 1.0) {
    const auto exponent = 1.0 - 1.0 / (1.0 - scaledRadiusSquared);
    const auto minimumNormalExponent = std::log(std::numeric_limits<double>::min());
    if (exponent >= minimumNormalExponent) {
      Q[Shortcuts::rho] = std::exp(exponent);
    }
  }
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
"""

flux = """
  const tarch::la::Vector<DIMENSIONS, double> centre(1.0);
  const tarch::la::Vector<DIMENSIONS, double> radialOffset = x - centre;
  const double radius = tarch::la::norm2(radialOffset);
  F[Shortcuts::rho] = (radius > 0.0 ? radialOffset(normal) / radius : 0.0) * Q[Shortcuts::rho];
"""

# The unit offset keeps the relative detail indicator well scaled where density
# vanishes.
refinement_criterion = """
  return ::exahype2::refineOnDetail(
    Q,
    DGOrder + 1,
    NumberOfUnknowns + NumberOfAuxiliaryVariables,
    1.0e-2,
    ::exahype2::DefaultCoarsenSafetyFactor,
    kernels::RKDGSolver::Quadrature::QuadraturePoints1D,
    [](const double* const values) { return 1.0 + values[Shortcuts::rho]; }
  );
"""

solver = exahype2.solvers.rkdg.GlobalAdaptiveTimeStep(
    name="RKDGSolver",
    rk_order=2,
    dg_order=args.degrees_of_freedom - 1,
    time_step_relaxation=0.9,
    unknowns={"rho": 1},
    auxiliary_variables=0,
)

solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    max_eigenvalue="return 1.0;",
    flux=flux,
)

project.add_solver(solver)
project.generate_and_build()
