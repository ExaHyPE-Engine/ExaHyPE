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
    parser.add_argument("-et", "--end-time", type=float, default=0.15)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=16)
    parser.add_argument("-l", "--grid-levels", type=int, default=2)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

initial_conditions = """
  const double squareOfX0 = tarch::la::productRoundedOnce(x(0), x(0));
  const double squareOfX1 = tarch::la::productRoundedOnce(x(1), x(1));
  const double radius     = std::sqrt(squareOfX0 + squareOfX1);
  const double bathymetry = 0.3 * std::exp(-radius * radius / 0.0625);
  const bool wet = radius > 0.35 and radius < 0.65;

  Q[Shortcuts::h] = wet ? 0.2 - bathymetry : 0.0;
  Q[Shortcuts::v + 0] = 0.0;
  Q[Shortcuts::v + 1] = 0.0;
  Q[Shortcuts::b] = bathymetry;
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]     = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::v + 0] = Qinside[Shortcuts::v + 0];
  Qoutside[Shortcuts::v + 1] = Qinside[Shortcuts::v + 1];
  Qoutside[Shortcuts::b]     = Qinside[Shortcuts::b];
"""

flux = """
  if (Q[Shortcuts::h] < DRY_TOLERANCE) {
    F[Shortcuts::h]     = 0.0;
    F[Shortcuts::v + 0] = 0.0;
    F[Shortcuts::v + 1] = 0.0;
  } else {
    const double inverseHeight = 1.0 / Q[Shortcuts::h];
    F[Shortcuts::h]     = Q[Shortcuts::v + normal];
    F[Shortcuts::v + 0] =
      Q[Shortcuts::v + normal] * Q[Shortcuts::v + 0] * inverseHeight;
    F[Shortcuts::v + 1] =
      Q[Shortcuts::v + normal] * Q[Shortcuts::v + 1] * inverseHeight;
  }
  F[Shortcuts::b] = 0.0;
"""

ncp = """
  BTimesDeltaQ[Shortcuts::h] = 0.0;
  switch (normal) {
    case 0:
      BTimesDeltaQ[Shortcuts::v + 0] =
        GRAVITY * Q[Shortcuts::h]
        * (deltaQ[Shortcuts::h] + deltaQ[Shortcuts::b]);
      BTimesDeltaQ[Shortcuts::v + 1] = 0.0;
      break;
    case 1:
      BTimesDeltaQ[Shortcuts::v + 0] = 0.0;
      BTimesDeltaQ[Shortcuts::v + 1] =
        GRAVITY * Q[Shortcuts::h]
        * (deltaQ[Shortcuts::h] + deltaQ[Shortcuts::b]);
      break;
  }
  BTimesDeltaQ[Shortcuts::b] = 0.0;
"""

max_eigenvalue = """
  if (Q[Shortcuts::h] < DRY_TOLERANCE) {
    return 0.0;
  }
  const double velocity = Q[Shortcuts::v + normal] / Q[Shortcuts::h];
  const double soundSpeed = std::sqrt(GRAVITY * Q[Shortcuts::h]);
  return std::fmax(
    std::fabs(velocity + soundSpeed),
    std::fabs(velocity - soundSpeed)
  );
"""

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="DryRing",
    dimensions=2,
    size=[2.0, 2.0, 2.0],
    offset=[-1.0, -1.0, -1.0],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

if project.dimensions != 2:
    raise ValueError("The dry ring test runs in 2D only.")

fv_solver = exahype2.fv.Solver(
    name="SWESolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"h": 1, "v": 2, "b": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.4),
    scheme=exahype2.fv.MusclHancock(limiter=exahype2.fv.Limiter.minmod),
)

fv_solver.set_pde_terms(
    initial_condition=initial_conditions,
    boundary_conditions=boundary_conditions,
    flux=flux,
    ncp=ncp,
    max_eigenvalue=max_eigenvalue,
)

project.add_solver(fv_solver)

project.output.makefile.add_compile_definition("GRAVITY=9.81")
project.output.makefile.add_compile_definition("DRY_TOLERANCE=1e-6")
project.generate_and_build()
