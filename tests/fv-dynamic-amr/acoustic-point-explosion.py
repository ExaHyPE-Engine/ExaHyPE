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
    namespace=["tests", "exahype2", "fv"],
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

# The pulse lies on the diagonal and every wall reflects, so the solution stays symmetric under axis
# permutations.
initial_conditions = """
  const tarch::la::Vector<DIMENSIONS, double> sourceCentre(-0.7);
  const double distanceSquared = tarch::la::norm2Squared(x - sourceCentre);
  constexpr double Sigma = 0.1;
  Q[Shortcuts::p] = std::exp(-distanceSquared / (2.0 * Sigma * Sigma));
  for (int d = 0; d < DIMENSIONS; d++) {
    Q[Shortcuts::u + d] = 0.0;
  }
"""

boundary_conditions = """
  Qoutside[Shortcuts::p] = Qinside[Shortcuts::p];
  for (int d = 0; d < DIMENSIONS; d++) {
    Qoutside[Shortcuts::u + d] = Qinside[Shortcuts::u + d];
  }
  Qoutside[Shortcuts::u + normal] = -Qinside[Shortcuts::u + normal];
"""

flux = """
  static constexpr double K0 = RHO * WAVE_SPEED * WAVE_SPEED;

  F[Shortcuts::p] = K0 * Q[Shortcuts::u + normal];
  for (int d = 0; d < DIMENSIONS; d++) {
    F[Shortcuts::u + d] = 0.0;
  }
  F[Shortcuts::u + normal] = (1.0 / RHO) * Q[Shortcuts::p];
"""

# Pressure and velocity magnitude are symmetric under axis permutations. The unit offsets bound the
# relative detail near the zero background.
refinement_criterion = """
  return ::exahype2::refineOnDetail(
    Q,
    ::exahype2::getVolumeCentresInUnitInterval<PatchSize>(),
    NumberOfUnknowns + NumberOfAuxiliaryVariables,
    5.0e-3,
    ::exahype2::DefaultCoarsenSafetyFactor,
    [](const double* const values) { return 1.0 + values[Shortcuts::p]; },
    [](const double* const values) {
      double velocitySquared = 0.0;
      for (int d = 0; d < DIMENSIONS; d++) {
        const double velocity = values[Shortcuts::u + d];
        const double square   = velocity * velocity;
        velocitySquared += square;
      }
      return 1.0 + RHO * WAVE_SPEED * std::sqrt(velocitySquared);
    }
  );
"""

solver = exahype2.fv.Solver(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"p": 1, "u": project.dimensions},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.8),
    scheme=exahype2.fv.Godunov(),
)

solver.set_pde_terms(
    initial_condition=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=flux,
    max_eigenvalue="return WAVE_SPEED;",
)

project.add_solver(solver)

project.output.makefile.add_compile_definition("RHO=1.0")
project.output.makefile.add_compile_definition("WAVE_SPEED=1.0")
project.generate_and_build()
