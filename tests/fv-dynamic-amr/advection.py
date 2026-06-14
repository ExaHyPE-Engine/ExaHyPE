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
    parser.add_argument("-et", "--end-time", type=float, default=1.5)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=4)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=2)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=6)
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
    project_name=".",
    dimensions=2,
    size=[2.0, 2.0, 2.0],
    offset=[0.0, 0.0, 0.0],
    periodic_BC=[True, False, False],
    executable="ExaHyPE",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  constexpr double radius = 0.1;

#if DIMENSIONS == 3
  tarch::la::Vector<DIMENSIONS, double> centre = {0.75, 1.0, 1.0};
#else
  tarch::la::Vector<DIMENSIONS, double> centre = {0.75, 1.0};
#endif

  if (tarch::la::norm2(x - centre) < radius) {
    Q[Shortcuts::rho] = 1.0;
  }
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
"""

refinement_criterion = """
  constexpr double radius = 0.1;

#if DIMENSIONS == 3
  const tarch::la::Vector<DIMENSIONS, double> centre = {0.75, 1.0, 1.0};
#else
  const tarch::la::Vector<DIMENSIONS, double> centre = {0.75, 1.0};
#endif

  if (tarch::la::equals(t, 0.0)) {
    return (tarch::la::smallerEquals(tarch::la::norm2(x - centre), radius))
      ? ::exahype2::RefinementCommand::Refine
      : ::exahype2::RefinementCommand::Keep;
  }

  const double rho = Q[Shortcuts::rho];

  return (tarch::la::greaterEquals(rho, 0.5) and tarch::la::smallerEquals(rho, 1.0))
    ? ::exahype2::RefinementCommand::Refine
    : ::exahype2::RefinementCommand::Erase;
"""

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": project.dimensions},
    auxiliary_variables=0,
    time_step_relaxation=0.5,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux="""
  for (int i = 0; i < NumberOfUnknowns; i++) {
    F[i] = 0.0;
  }
  F[normal] = Q[normal];
""",
    max_eigenvalue="return 1.0;",
)

project.add_solver(fv_solver)

project = project.generate_Peano4_project(verbose=False)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
