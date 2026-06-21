# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

DOMAIN_OFFSET = [-5.0, 0.0]
DOMAIN_SIZE = [15.0, 15.0]
END_TIME = 5.0
PERIODIC_BOUNDARY_CONDITIONS = [False, False]

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  constexpr double depth           = 1.0;
  constexpr double beachSlopeAngle = 30.0 * std::numbers::pi / 180.0;
  constexpr double waveAmplitude   = 0.5;

  const double gamma    = std::sqrt((3.0 * waveAmplitude) / (4.0 * depth));
  const double wavePos  = std::acosh(std::sqrt(20.0)) / gamma;
  const double shorelineX = depth / std::tan(beachSlopeAngle);

  if (x[0] < 0.0) {
    Q[Shortcuts::b] = std::fabs(x[0]) * std::tan(beachSlopeAngle);
  } else if (x[0] < shorelineX) {
    Q[Shortcuts::b] = -(x[0] * std::tan(beachSlopeAngle));
  } else {
    Q[Shortcuts::b] = -depth;
  }

  const double innerTerm   = gamma * ((x[0] - (shorelineX + wavePos)) / depth);
  const double waveProfile = waveAmplitude / std::pow(std::cosh(innerTerm), 2);
  if (x[0] >= shorelineX) {
    Q[Shortcuts::h] = depth + waveProfile;
  } else if (x[0] >= 0.0) {
    Q[Shortcuts::h] = std::fabs(Q[Shortcuts::b]);
  } else if (x[0] >= (shorelineX + 2.0 * wavePos)) {
    Q[Shortcuts::h] = depth;
  }

  const double waveCelerity = std::sqrt(g * (waveAmplitude + depth));
  Q[Shortcuts::hu] = -((waveCelerity * waveProfile) / (depth + waveProfile)) * Q[Shortcuts::h];
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu] = -Qinside[Shortcuts::hu];
  Qoutside[Shortcuts::hv] = -Qinside[Shortcuts::hv];
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
  return ::exahype2::RefinementCommand::Keep;
"""

riemann_solver = """
  return ::applications::exahype2::exaskred::riemann::llf<Shortcuts>(
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
    "hThreshold": [1e-5, "double"],
}

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="SolitaryWave",
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
    raise ValueError("The solitary-wave application is two-dimensional.")

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
#include <cmath>
#include <numbers>
#include "../Flux.h"
#include "../NonconservativeProduct.h"
#include "../riemann/LLF.h"
"""
)

project.add_solver(fv_solver)
peano_project = project.generate_Peano4_project()

for const_name, (const_val, const_type) in constants.items():
    peano_project.constants.export_constexpr_with_type(
        const_name, str(const_val), const_type
    )

peano_project.generate_and_build()
