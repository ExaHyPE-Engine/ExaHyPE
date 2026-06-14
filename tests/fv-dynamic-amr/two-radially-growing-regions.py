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
    parser.add_argument("-et", "--end-time", type=float, default=3.0)
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
    project_name=".",
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

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;

#if DIMENSIONS == 3
  tarch::la::Vector<DIMENSIONS, double> centre0 = {2.0, 5.0, 5.0};
  tarch::la::Vector<DIMENSIONS, double> centre1 = {8.0, 5.0, 5.0};
  constexpr double maxRadius = 7.5;
#else
  tarch::la::Vector<DIMENSIONS, double> centre0 = {2.0, 5.0};
  tarch::la::Vector<DIMENSIONS, double> centre1 = {8.0, 5.0};
  constexpr double maxRadius = 5.0;
#endif

  const double startRadius = ::exahype2::RuntimeParameters::getInstance().getMaxAdmissiblePatchSize(DomainSize);
  constexpr double growthRate  = 0.25; // radius = startRadius + growthRate*t
  constexpr double bandWidth   = 0.10;

  const double frontRadius = std::min(maxRadius, startRadius + growthRate * t);
  const double r0 = tarch::la::norm2(x - centre0);
  const double r1 = tarch::la::norm2(x - centre1);

  auto command0 = ::exahype2::RefinementCommand::Keep;
  auto command1 = ::exahype2::RefinementCommand::Keep;

  if (tarch::la::equals(t, 0.0)) {
    if (r0 < startRadius) command0 = ::exahype2::RefinementCommand::Refine;
    if (r1 < startRadius) command1 = ::exahype2::RefinementCommand::Refine;
  } else {
    const double innerEraseRadius = std::max(0.0, frontRadius - bandWidth);

    if (r0 >= innerEraseRadius and r0 <= frontRadius) {
      command0 = ::exahype2::RefinementCommand::Refine;
    } else if (r0 < innerEraseRadius) {
      command0 = ::exahype2::RefinementCommand::Erase;
    }

    if (r1 >= innerEraseRadius and r1 <= frontRadius) {
      command1 = ::exahype2::RefinementCommand::Refine;
    } else if (r1 < innerEraseRadius) {
      command1 = ::exahype2::RefinementCommand::Erase;
    }
  }

  if (command0 == ::exahype2::RefinementCommand::Refine or command1 == ::exahype2::RefinementCommand::Refine) {
    result = ::exahype2::RefinementCommand::Refine;
  } else if (command0 == ::exahype2::RefinementCommand::Erase or command1 == ::exahype2::RefinementCommand::Erase) {
    result = ::exahype2::RefinementCommand::Erase;
  } else {
    result = ::exahype2::RefinementCommand::Keep;
  }

  return result;
"""

fv_solver = exahype2.solvers.fv.godunov.GlobalFixedTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns=1,
    auxiliary_variables=0,
    normalised_time_step_size=0.01,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux="""
  for (int i = 0; i < NumberOfUnknowns; i++) {
    F[i] = 0.0;
  }
""",
    max_eigenvalue="return 1.0;",
)

project.add_solver(fv_solver)

project = project.generate_Peano4_project(verbose=False)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
