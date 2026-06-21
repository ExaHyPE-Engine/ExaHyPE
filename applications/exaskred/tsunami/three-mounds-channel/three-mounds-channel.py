# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

DOMAIN_OFFSET = [0.0, 0.0]
DOMAIN_SIZE = [30.0, 30.0]
END_TIME = 20.0
PERIODIC_BOUNDARY_CONDITIONS = [False, False]

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  constexpr double initialWaterHeightDam = 0.9;

  const double moundOne =
    1.0 - 0.10 * std::sqrt((x(0) - 15.0) * (x(0) - 15.0) + (x(1) - 22.5) * (x(1) - 22.5));
  const double moundTwo =
    1.0 - 0.10 * std::sqrt((x(0) - 15.0) * (x(0) - 15.0) + (x(1) - 7.50) * (x(1) - 7.50));
  const double moundThree =
    1.0 - 0.28 * std::sqrt((x(0) - 28.0) * (x(0) - 28.0) + (x(1) - 15.0) * (x(1) - 15.0));

  Q[Shortcuts::h] = x[0] <= 5.0 ? initialWaterHeightDam : 0.0;
  Q[Shortcuts::b] = std::max({0.0, moundOne, moundTwo, moundThree});
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu] = 0.0;
  Qoutside[Shortcuts::hv] = 0.0;
  Qoutside[Shortcuts::b]  = Qinside[Shortcuts::b];
"""

flux = """
  ::applications::exahype2::exaskred::tsunami::flux<Shortcuts, NumberOfUnknowns>(
    Q, x, h, t, dt, normal, F
  );
"""

max_eigenvalue = """
  return ::applications::exahype2::exaskred::tsunami::maxEigenvalue<Shortcuts>(
    Q, x, h, t, dt, normal
  );
"""

nonconservative_product = """
  ::applications::exahype2::exaskred::tsunami::nonconservativeProduct<Shortcuts, NumberOfUnknowns>(
    Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ
  );
"""

refinement_criterion = """
  const double moundOne =
    1.0 - 0.10 * std::sqrt((x(0) - 15.0) * (x(0) - 15.0) + (x(1) - 22.5) * (x(1) - 22.5));
  const double moundTwo =
    1.0 - 0.10 * std::sqrt((x(0) - 15.0) * (x(0) - 15.0) + (x(1) - 7.50) * (x(1) - 7.50));
  const double moundThree =
    1.0 - 0.28 * std::sqrt((x(0) - 28.0) * (x(0) - 28.0) + (x(1) - 15.0) * (x(1) - 15.0));

  return moundOne > 0.0 || moundTwo > 0.0 || moundThree > 0.0
    ? ::exahype2::RefinementCommand::Refine
    : ::exahype2::RefinementCommand::Keep;
"""

riemann_solver = """
  return ::applications::exahype2::exaskred::riemann::augmentedState<Shortcuts>(
    QL,
    QR,
    x,
    h,
    t,
    dt,
    normal,
    FL,
    FR
  );
"""


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=END_TIME)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=16)
    parser.add_argument("-l", "--grid-levels", type=int, default=None)
    parser.add_argument("-lmin", "--min-grid-levels", type=int, default=5)
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

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-3, "double"],
}

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="ThreeMoundsChannel",
    dimensions=2,
    offset=DOMAIN_OFFSET,
    size=DOMAIN_SIZE,
    periodic_BC=PERIODIC_BOUNDARY_CONDITIONS,
    directory=".",
    executable="ExaSkred-Tsunami",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The three-mounds-channel application is two-dimensional.")

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"b": 1},
    time_step_relaxation=0.5,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
    ncp=nonconservative_product,
    riemann_solver=riemann_solver,
)
fv_solver.add_user_solver_includes(
    """
#include <algorithm>
#include <cmath>
#include "../Flux.h"
#include "../NonconservativeProduct.h"
#include "../riemann/AugmentedState.h"
"""
)

project.add_solver(fv_solver)
peano_project = project.generate_Peano4_project()

for const_name, (const_val, const_type) in constants.items():
    peano_project.constants.export_constexpr_with_type(
        const_name, str(const_val), const_type
    )

peano_project.generate_and_build()
