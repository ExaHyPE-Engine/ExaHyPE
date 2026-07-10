# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

DOMAIN_OFFSET = [-2.0, -2.0]
DOMAIN_SIZE = [4.0, 4.0]
END_TIME = 10.0
PERIODIC_BOUNDARY_CONDITIONS = [False, False]

initial_conditions = """
  constexpr double a     = 1.0;
  constexpr double hZero = 0.1;
  constexpr double eta   = 0.5;

  const double r     = std::sqrt(std::pow(x[0], 2) + std::pow(x[1], 2));
  const double omega = std::sqrt(2 * g * hZero) / a;

  Q[Shortcuts::b] = -hZero * (1.0 - std::pow(r, 2) / std::pow(a, 2));
  Q[Shortcuts::h] =
    ((eta * hZero) / std::pow(a, 2))
    * (2 * x[0] * std::cos(omega * 0.0) + 2 * x[1] * std::sin(omega * 0.0) - eta)
    - Q[Shortcuts::b];

  if (Q[Shortcuts::h] >= 0.0) {
    Q[Shortcuts::hu] = (-eta * omega * std::sin(omega * 0.0)) * Q[Shortcuts::h];
    Q[Shortcuts::hv] = (eta * omega * std::cos(omega * 0.0)) * Q[Shortcuts::h];
  } else {
    Q[Shortcuts::h]  = 0.0;
    Q[Shortcuts::hu] = 0.0;
    Q[Shortcuts::hv] = 0.0;
  }
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
  return std::sqrt(x(0) * x(0) + x(1) * x(1)) <= 1.0
    ? ::exahype2::RefinementCommand::Refine
    : ::exahype2::RefinementCommand::Keep;
"""

analytical_solution = """
  constexpr double a     = 1.0;
  constexpr double hZero = 0.1;
  constexpr double eta   = 0.5;

  const double omega = std::sqrt(2 * g * hZero) / a;
  const double currentHeight =
    ((eta * hZero) / std::pow(a, 2))
    * (2 * x[0] * std::cos(omega * t) + 2 * x[1] * std::sin(omega * t) - eta)
    - Q[Shortcuts::b];

  if (currentHeight > 0.0) {
    solution[0] = currentHeight;
    solution[1] = (-eta * omega * std::sin(omega * t)) * currentHeight;
    solution[2] = (eta * omega * std::cos(omega * t)) * currentHeight;
  } else {
    solution[0] = 0.0;
    solution[1] = 0.0;
    solution[2] = 0.0;
  }
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
    "hThreshold": [1e-5, "double"],
}

project = exahype2.Project(
    namespace=["applications", "exahype2", "exaskred"],
    project_name="OscillatingLake",
    dimensions=2,
    offset=DOMAIN_OFFSET,
    size=DOMAIN_SIZE,
    periodic_bc=PERIODIC_BOUNDARY_CONDITIONS,
    executable="ExaSkred-Tsunami",
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.min_grid_levels,
    max_grid_levels=args.max_grid_levels,
)
if project.dimensions != 2:
    raise ValueError("The oscillating-lake application is two-dimensional.")

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"b": 1},
    time_step_relaxation=0.5,
)
fv_solver.postprocess_updated_patch = """
  {
    int index = 0;
    for (int i = 0; i < {{NUMBER_OF_VOLUMES_PER_AXIS}} * {{NUMBER_OF_VOLUMES_PER_AXIS}}; i++) {
      double* const value = QOut + index;
      if (!std::isfinite(value[VariableShortcutsFVSolver::h]) or value[VariableShortcutsFVSolver::h] <= hThreshold) {
        value[VariableShortcutsFVSolver::h]  = 0.0;
        value[VariableShortcutsFVSolver::hu] = 0.0;
        value[VariableShortcutsFVSolver::hv] = 0.0;
      }
      index += {{NUMBER_OF_UNKNOWNS}} + {{NUMBER_OF_AUXILIARY_VARIABLES}};
    }
  }
"""

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
#include "../Flux.h"
#include "../NonconservativeProduct.h"
#include "../riemann/AugmentedState.h"
"""
)

exahype2.solvers.fv.ErrorMeasurement(
    fv_solver,
    error_measurement_implementation=analytical_solution,
    time_delta_between_snapshots=args.end_time / 100.0,
)

project.add_solver(fv_solver)

for const_name, (const_val, const_type) in constants.items():
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)

project.generate_and_build()
