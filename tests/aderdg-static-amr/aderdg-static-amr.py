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

polynomials = exahype2.solvers.aderdg.Polynomials.Gauss_Legendre

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name="ADERDGStaticAMR",
    dimensions=2,
    size=[1.0, 1.0, 1.0],
    offset=[0.0, 0.0, 0.0],
    periodic_BC=[False, False, False],
    executable="ExaHyPE",
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

solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=order,
    time_step_relaxation=0.8,
    unknowns=4,
    auxiliary_variables=0,
)

solver.set_implementation(
    initial_conditions="""
  double r = std::sqrt((x[0] - 0.5) * (x[0] - 0.5) + (x[1] - 0.5) * (x[1] - 0.5));
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
  double r = std::sqrt((x[0] - 0.5) * (x[0] - 0.5) + (x[1] - 0.5) * (x[1] - 0.5));
  if (r < 0.2) {
    result = ::exahype2::RefinementCommand::Refine;
  }
  return result;
""",
    max_eigenvalue="""
  constexpr double grav = 9.81;
  const double u = Q[1 + normal] / Q[0];
  const double c = std::sqrt(grav * Q[0]);
  return std::max(std::abs(u + c), std::abs(u - c));
""",
    flux="""
  const double ih = 1.0 / Q[0];
  F[0] = Q[1 + normal];
  F[1] = Q[1 + normal] * Q[1] * ih;
  F[2] = Q[1 + normal] * Q[2] * ih;
  F[3] = 0.0;
""",
    ncp="""
  constexpr double grav = 9.81;
  BTimesDeltaQ[0]       = 0.0;
  switch (normal) {
  case 0:
    BTimesDeltaQ[1] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
    BTimesDeltaQ[2] = 0.0;
    break;
  case 1:
    BTimesDeltaQ[1] = 0.0;
    BTimesDeltaQ[2] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
    break;
  }
  BTimesDeltaQ[3] = 0.0;
""",
)

project.add_solver(solver)

project = project.generate_Peano4_project()
project.generate_and_build()
