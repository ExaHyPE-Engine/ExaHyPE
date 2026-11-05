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
    project_name="FreeStream",
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
  Q[Shortcuts::p] = 1.0;
  for (int d = 0; d < DIMENSIONS; d++) {
    Q[Shortcuts::u + d] = 0.0;
  }
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = 0.0;
  }
  Qoutside[Shortcuts::p] = 1.0;
"""

refinement_criterion = """
  const tarch::la::Vector<DIMENSIONS, double> centre(5.0);
#if DIMENSIONS == 3
  constexpr double MaxRadius = 7.5;
#else
  constexpr double MaxRadius = 5.0;
#endif

  const double     startRadius = parameters.maxAdmissiblePatchSize;
  constexpr double GrowthRate  = 1.5;
  constexpr double BandWidth   = 0.05;

  const double frontRadius = std::min(MaxRadius, startRadius + GrowthRate * t);
  const double innerRadius = std::max(0.0, frontRadius - BandWidth);
  const double radius      = tarch::la::norm2(x - centre);

  const double cellReach = 0.5 * tarch::la::norm2(h);

  auto result = ::exahype2::RefinementCommand::Keep;
  if (radius - cellReach <= frontRadius and radius + cellReach >= innerRadius) {
    result = ::exahype2::RefinementCommand::Refine;
  } else if (Q != nullptr and radius + cellReach < innerRadius) {
    result = ::exahype2::RefinementCommand::Erase;
  }

  return result;
"""

flux = """
  F[Shortcuts::p] = Q[Shortcuts::u + normal];
  for (int d = 0; d < DIMENSIONS; d++) {
    F[Shortcuts::u + d] = 0.0;
  }
  F[Shortcuts::u + normal] = Q[Shortcuts::p];
"""

solver = exahype2.fv.singlestep.Solver(
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
    max_eigenvalue="return 1.0;",
    flux=flux,
    analytical_solution="""
  solution[Shortcuts::p] = 1.0;
  for (int d = 0; d < DIMENSIONS; d++) {
    solution[Shortcuts::u + d] = 0.0;
  }
""",
)

solver.measure_error(file="Error", every=args.end_time / 10.0)

project.add_solver(solver)
project.generate_and_build()
