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
    periodic_bc=[False, False, False],
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

  tarch::la::Vector<DIMENSIONS, double> centre0(5.0);
#if DIMENSIONS == 3
  constexpr double maxRadius = 7.5;
#else
  constexpr double maxRadius = 5.0;
#endif

  const double startRadius = ::runner::RuntimeParameters::getInstance().getMaxAdmissiblePatchSize(DomainSize);
  constexpr double growthRate  = 0.25; // radius = startRadius + growthRate*t
  constexpr double bandWidth   = 0.05;

  const double frontRadius = std::min(maxRadius, startRadius + growthRate * t);
  const double r = tarch::la::norm2(x - centre0);

  if (tarch::la::equals(t, 0.0)) {
    if (r < startRadius) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  } else {
    const double innerEraseRadius = std::max(0.0, frontRadius - bandWidth);
    if (r >= innerEraseRadius and r <= frontRadius) {
      result = ::exahype2::RefinementCommand::Refine;
    } else if (r < innerEraseRadius) {
      result = ::exahype2::RefinementCommand::Erase;
    }
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

project.generate_and_build()
