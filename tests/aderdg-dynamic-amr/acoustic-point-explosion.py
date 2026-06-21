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
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=0.1)
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

polynomials = exahype2.solvers.aderdg.Polynomials.Gauss_Legendre

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name="AcousticPointExplosion",
    dimensions=2,
    size=[10.0, 10.0, 10.0],
    offset=[0.0, 0.0, 0.0],
    periodic_BC=[False, False, False],
    executable="ExaHyPE",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)

solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=(args.degrees_of_freedom - 1),
    time_step_relaxation=0.5,
    unknowns={"p": 1, "u": project.dimensions},
    auxiliary_variables=0,
)

solver.set_implementation(
    initial_conditions="""
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }
""",
    boundary_conditions="""
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = 0.0;
  }
""",
    refinement_criterion="""
  auto result = ::exahype2::RefinementCommand::Keep;

  static constexpr double sourceRadius = 2.2;
  const tarch::la::Vector<DIMENSIONS, double> sourceCentre(1.0);

  static constexpr double t0 = 0.8;
  if (tarch::la::greaterEquals(t, 0.0) and tarch::la::smallerEquals(t, t0)) {
    if (tarch::la::smallerEquals(tarch::la::norm2(x - sourceCentre), sourceRadius)) {
      result = ::exahype2::RefinementCommand::Refine;
    }
    return result;
  }

  const double pAbs = std::abs(Q[Shortcuts::p]);

  double u2 = 0.0;
  for (int i = 0; i < DIMENSIONS; i++) {
    const double ui = Q[Shortcuts::u + i];
    u2 += ui * ui;
  }
  const double uMag = std::sqrt(u2);

  const double indicator = std::max(pAbs, (RHO * WAVE_SPEED) * uMag);

  static constexpr double refineThreshold = 0.10;
  static constexpr double eraseThreshold  = 0.05;

  if (tarch::la::greaterEquals(indicator, refineThreshold)) {
    result = ::exahype2::RefinementCommand::Refine;
  } else if (tarch::la::smallerEquals(indicator, eraseThreshold)) {
    result = ::exahype2::RefinementCommand::Erase;
  }

  return result;
""",
    max_eigenvalue="""return WAVE_SPEED;
""",
    flux="""
  static constexpr double K0 = RHO * WAVE_SPEED * WAVE_SPEED;

  F[Shortcuts::p]     = K0 * Q[normal + 1];
  F[Shortcuts::u + 0] = 0.0;
  F[Shortcuts::u + 1] = 0.0;
#if DIMENSIONS == 3
  F[Shortcuts::u + 2] = 0.0;
#endif
  F[normal + 1]       = (1.0 / RHO) * Q[Shortcuts::p];
""",
    source_term="""
  S[Shortcuts::p]     = 0.0;
  S[Shortcuts::u + 0] = 0.0;
  S[Shortcuts::u + 1] = 0.0;
#if DIMENSIONS == 3
  S[Shortcuts::u + 2] = 0.0;
#endif
  constexpr double sourceRadius = 0.10;
  const bool sourceLocation = tarch::la::norm2(x - tarch::la::Vector<DIMENSIONS, double>(1.0)) <= sourceRadius;

  if (sourceLocation) {
    static constexpr double t0 = 0.7;
    static constexpr double M0 = 1000.0;
    const double force = M0 * std::exp(-((t - t0) * (t - t0)) / (2.0 * SIGMA * SIGMA));
    S[Shortcuts::p] = force;
  }
""",
)

project.add_solver(solver)

project = project.generate_Peano4_project()
project.output.makefile.add_compile_definition("RHO=2.7")
project.output.makefile.add_compile_definition("WAVE_SPEED=6.0")
project.output.makefile.add_compile_definition("SIGMA=0.1149")
project.generate_and_build()
