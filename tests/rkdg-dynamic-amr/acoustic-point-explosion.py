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
    parser.add_argument("-et", "--end-time", type=float, default=2.0)
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
    namespace=["tests", "exahype2", "rkdg"],
    project_name="AcousticPointExplosion",
    dimensions=2,
    size=[2.0, 2.0, 2.0],
    offset=[-1.0, -1.0, -1.0],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)

solver = exahype2.solvers.rkdg.rusanov.GlobalAdaptiveTimeStep(
    name="RKDGSolver",
    rk_order=2,
    polynomials=exahype2.solvers.GaussLegendreBasis(args.degrees_of_freedom),
    time_step_relaxation=0.9,
    unknowns={"p": 1, "u": project.dimensions},
    auxiliary_variables=0,
)

# Placing the source on the domain diagonal preserves axis-permutation symmetry
# in the solution and refinement pattern. Unit offsets keep the detail
# indicators well scaled at the ambient state.
solver.set_implementation(
    initial_conditions="""
  const tarch::la::Vector<DIMENSIONS, double> sourceCentre(-0.7);
  const double distanceSquared = tarch::la::norm2Squared(x - sourceCentre);
  constexpr double Sigma = 0.1;
  Q[Shortcuts::p] = std::exp(-distanceSquared / (2.0 * Sigma * Sigma));
  for (int d = 0; d < DIMENSIONS; d++) {
    Q[Shortcuts::u + d] = 0.0;
  }
""",
    boundary_conditions="""
  Qoutside[Shortcuts::p] = Qinside[Shortcuts::p];
  for (int d = 0; d < DIMENSIONS; d++) {
    Qoutside[Shortcuts::u + d] = Qinside[Shortcuts::u + d];
  }
  Qoutside[Shortcuts::u + normal] = -Qinside[Shortcuts::u + normal];
""",
    refinement_criterion="""
  return ::exahype2::refineOnDetail(
    Q,
    DGOrder + 1,
    NumberOfUnknowns + NumberOfAuxiliaryVariables,
    1.0e-2,
    9.0,
    QuadraturePoints1D,
    [](const double* const values) { return 1.0 + values[Shortcuts::p]; },
    [](const double* const values) {
      double velocitySquared = 0.0;
      for (int d = 0; d < DIMENSIONS; d++) {
        velocitySquared += values[Shortcuts::u + d] * values[Shortcuts::u + d];
      }
      return 1.0 + RHO * WAVE_SPEED * std::sqrt(velocitySquared);
    }
  );
""",
    eigenvalues="return WAVE_SPEED;",
    flux="""
  static constexpr double K0 = RHO * WAVE_SPEED * WAVE_SPEED;

  F[Shortcuts::p] = K0 * Q[Shortcuts::u + normal];
  for (int d = 0; d < DIMENSIONS; d++) {
    F[Shortcuts::u + d] = 0.0;
  }
  F[Shortcuts::u + normal] = (1.0 / RHO) * Q[Shortcuts::p];
""",
)

project.add_solver(solver)

project.output.makefile.add_compile_definition("RHO=1.0")
project.output.makefile.add_compile_definition("WAVE_SPEED=1.0")
project.generate_and_build()
