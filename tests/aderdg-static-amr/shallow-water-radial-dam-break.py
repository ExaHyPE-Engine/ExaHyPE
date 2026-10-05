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
    parser.add_argument("-et", "--end-time", type=float, default=0.1)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=5)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=3)
    parser.add_argument("-lmax", "--max-grid-levels", type=int, default=5)
    args, _ = parser.parse_known_args()
    if args.grid_levels is not None:
        args.min_grid_levels = args.grid_levels
        args.max_grid_levels = args.grid_levels
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

order = args.degrees_of_freedom - 1

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name="ShallowWaterRadialDamBreak",
    dimensions=2,
    size=[1.0, 1.0, 1.0],
    offset=[0.0, 0.0, 0.0],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)

if project.dimensions != 2:
    raise ValueError(
        f"ADER-DG static AMR is a 2D testcase, but the project was configured "
        f"for {project.dimensions}D."
    )

solver = exahype2.dg.ader.Solver(
    name="ADERDGSolver",
    order=order,
    unknowns=4,
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.8),
    basis=exahype2.dg.GaussLegendre,
)

solver.set_pde_terms(
    initial_condition="""
  const double firstSquare  = (x[0] - 0.5) * (x[0] - 0.5);
  const double secondSquare = (x[1] - 0.5) * (x[1] - 0.5);
  const double r            = std::sqrt(firstSquare + secondSquare);
  Q[0] = 1.;
  Q[1] = 0.;
  Q[2] = 0.;
  Q[3] = r < 0.1 ? 0.0 : 0.05;
""",
    boundary_conditions="""
  initialCondition(Qoutside, x, h);
""",
    refinement_criterion="""
  auto result = ::exahype2::RefinementCommand::Keep;
  const double firstSquare  = (x[0] - 0.5) * (x[0] - 0.5);
  const double secondSquare = (x[1] - 0.5) * (x[1] - 0.5);
  const double r            = std::sqrt(firstSquare + secondSquare);
  if (r < 0.2) {
    result = ::exahype2::RefinementCommand::Refine;
  }
  return result;
""",
    max_eigenvalue="""
  constexpr double Gravity = 9.81;
  const double u = Q[1 + normal] / Q[0];
  const double c = std::sqrt(Gravity * Q[0]);
  return std::max(std::abs(u + c), std::abs(u - c));
""",
    flux="""
  const double inverseHeight = 1.0 / Q[0];
  F[0] = Q[1 + normal];
  F[1] = Q[1 + normal] * Q[1] * inverseHeight;
  F[2] = Q[1 + normal] * Q[2] * inverseHeight;
  F[3] = 0.0;
""",
    ncp="""
  constexpr double Gravity = 9.81;
  BTimesDeltaQ[0]          = 0.0;
  BTimesDeltaQ[1 + normal] = Gravity * Q[0] * (deltaQ[0] + deltaQ[3]);
  BTimesDeltaQ[2 - normal] = 0.0;
  BTimesDeltaQ[3]          = 0.0;
""",
)

project.add_solver(solver)

project.generate_and_build()
