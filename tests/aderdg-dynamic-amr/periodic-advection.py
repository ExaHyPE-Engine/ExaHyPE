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
    parser.add_argument("-et", "--end-time", type=float, default=1.0)
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
    namespace=["tests", "exahype2", "aderdg"],
    project_name="PeriodicAdvection",
    dimensions=2,
    size=[1.0, 1.0, 1.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[True, True, True],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)

initial_conditions = """
  const tarch::la::Vector<DIMENSIONS, double> centre(0.5);
  constexpr double BumpRadius = 0.3;
  const auto scaledRadiusSquared =
    tarch::la::norm2Squared(x - centre) / (BumpRadius * BumpRadius);
  Q.q = 0.0;
  if (scaledRadiusSquared < 1.0) {
    const auto exponent = 1.0 - 1.0 / (1.0 - scaledRadiusSquared);
    const auto minimumNormalExponent = std::log(std::numeric_limits<double>::min());
    if (exponent >= minimumNormalExponent) {
      Q.q = std::exp(exponent);
    }
  }
"""

# The detail indicator follows the bump across periodic boundaries and coarsens
# its wake.
refinement_criterion = """
  return ::exahype2::refineOnDetail(
    Q,
    Quadrature<double>::getQuadraturePoints1D(),
    1.0e-2,
    ::exahype2::DefaultCoarsenSafetyFactor,
    [](const auto& state) { return 1.0 + state.q; }
  );
"""

flux = """
  F.q = Q.q;
"""

solver = exahype2.dg.ader.Solver(
    name="ADERDGSolver",
    order=(args.degrees_of_freedom - 1),
    unknowns={"q": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.9),
    linear=True,
)

solver.set_pde_terms(
    initial_condition=initial_conditions,
    refinement_criterion=refinement_criterion,
    max_eigenvalue="return 1.0;",
    flux=flux,
)

project.add_solver(solver)
project.generate_and_build()
