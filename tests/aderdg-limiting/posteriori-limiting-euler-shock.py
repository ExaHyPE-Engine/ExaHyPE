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
    parser.add_argument("-et", "--end-time", type=float, default=2.0)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=6)
    parser.add_argument("-l", "--grid-levels", type=int, default=3)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

use_aderdg = True
use_fv = True
use_limiting = use_aderdg and use_fv
with_limiting = 1  # 0 - all ADER, 1 - limit, 2 - all FV

dimensions = 2
order = args.degrees_of_freedom - 1
CFL = 0.5
unknowns = {"rho": 1, "rhoU": 1, "rhoV": 1, "rhoE": 1}
auxiliary = {}

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name="PosterioriLimitingEulerShock",
    dimensions=dimensions,
    size=[2.0, 2.0, 2.0],
    offset=[-1.0, -1.0, -1.0],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

if project.dimensions != dimensions:
    raise ValueError(
        f"Posteriori limiting Euler shock is a {dimensions}D testcase, but the "
        f"project was configured for {project.dimensions}D."
    )

Gamma = 1.4
OriginalShockPosition = 0.111
pZero = 1e-4

initial_conditions = f"""
  constexpr double OriginalShockPosition = {OriginalShockPosition};
  constexpr double Gamma = {Gamma};
  const double p = (x[0] < -OriginalShockPosition && x[1] < -OriginalShockPosition) ? 0.1 : 1.0;

  Q[0] = (x[0] < -OriginalShockPosition && x[1] < -OriginalShockPosition) ? 0.125 : 1.0;
  Q[1] = 0.0;
  Q[2] = 0.0;
  Q[3] = p / (Gamma - 1.0);
"""

boundary_conditions = f"""
  constexpr double OriginalShockPosition = {OriginalShockPosition};
  constexpr double Gamma = {Gamma};
  const double p = (x[0] < -OriginalShockPosition && x[1] < -OriginalShockPosition) ? 0.1 : 1.0;

  Qoutside[0] = (x[0] < -OriginalShockPosition && x[1] < -OriginalShockPosition) ? 0.125 : 1.0;
  Qoutside[1] = 0.0;
  Qoutside[2] = 0.0;
  Qoutside[3] = p / (Gamma - 1.0);
"""

flux = f"""
  constexpr double Gamma = {Gamma};
  constexpr double pZero = {pZero};
  const double irho = Q[0] > 0.0 ? 1.0 / Q[0] : 0.0;
  const double p = (Gamma - 1.0) * (Q[3] - 0.5 * irho * (Q[1]*Q[1]+Q[2]*Q[2]));

  if (tarch::la::greater(p, pZero)) {{
    F[0] = Q[normal + 1];
    F[1] = Q[normal + 1] * Q[1] * irho;
    F[2] = Q[normal + 1] * Q[2] * irho;
    F[3] = Q[normal + 1] * irho * (Q[3] + p);
    F[normal+1] += p;
  }} else {{
    F[0] = 0.0;
    F[1] = 0.0;
    F[2] = 0.0;
    F[3] = 0.0;
  }}
"""

max_eval = f"""
  constexpr double Gamma = {Gamma};
  constexpr double pZero = {pZero};
  const double irho = Q[0] > 0.0 ? 1.0 / Q[0] : 0.0;
  const double p = (Gamma - 1.0) * (Q[3] - 0.5 * irho*(Q[1]*Q[1]+Q[2]*Q[2]));

  if (tarch::la::greater(p, pZero)) {{
    const double c = std::sqrt(Gamma * p * irho);
    const double un = Q[normal + 1] * irho;
    return std::fmax(std::fabs(un - c), std::fabs(un + c));
  }}
  return 0.0;
"""

adjust_solution = f"""
  constexpr double pZero = {pZero};
  constexpr double Gamma = {Gamma};
  const double irho = Q[0] > 0.0 ? 1.0 / Q[0] : 0.0;
  const double p = (Gamma - 1.0) * (Q[3] - 0.5 * irho * (Q[1]*Q[1]+Q[2]*Q[2]));

  if (tarch::la::smaller(p, pZero)) {{
    Q[0] = 0.0;
    Q[1] = 0.0;
    Q[2] = 0.0;
    Q[3] = 0.0;
  }}
"""

isPhysicallyAdmissible = "return true;"
if with_limiting == 0:
    isPhysicallyAdmissible = "return true; //all ADERDG"
elif with_limiting == 1:
    isPhysicallyAdmissible = f"""
  constexpr double Gamma = {Gamma};
  constexpr double pZero = {pZero};
  const double irho = Q[0] > 0.0 ? 1.0 / Q[0] : 0.0;
  const double p = (Gamma - 1.0) * (Q[3] - 0.5 * irho * (Q[1]*Q[1]+Q[2]*Q[2]));

  // Pressure, Density and Energy should be positive
  if ((tarch::la::smaller(p, pZero))  // Pressure should be positive
    or (Q[0] < 0.0)                   // Density should be positive
    or (Q[3] < 0.0)                   // Energy should be positive
  ) {{
    return false;
  }}

  // Pressure, Density and Energy should be finite.
  // If non-physical oscillations occur, the wave speed might receive a negative value under the square root.
  // It results in eigenvalues becoming NaN, which are not marked as TROUBLED, if no isfinite check is performed.
  for (int i = 0; i < {len(unknowns)}; ++i) {{
    if (!std::isfinite(Q[i])) {{
      return false;
    }}
  }}

  return true;
"""
else:
    isPhysicallyAdmissible = "return false; //all FV"

if use_aderdg:
    aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
        name="ADERDGSolver",
        order=order,
        unknowns=unknowns,
        auxiliary_variables=auxiliary,
        time_step_relaxation=CFL,
    )
    aderdg_solver.set_implementation(
        flux=flux,
        max_eigenvalue=max_eval,
        initial_conditions=initial_conditions,
        boundary_conditions=boundary_conditions,
    )
    project.add_solver(aderdg_solver)

if use_fv:
    fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
        name="FVSolver",
        patch_size=2 * order + 1,
        unknowns=unknowns,
        auxiliary_variables=auxiliary,
        time_step_relaxation=CFL,
    )
    fv_solver.set_implementation(
        flux=flux,
        max_eigenvalue=max_eval,
        initial_conditions=initial_conditions,
        boundary_conditions=boundary_conditions,
        adjust_solution=adjust_solution,
    )
    project.add_solver(fv_solver)

if use_limiting:
    limiter = exahype2.solvers.limiting.PosterioriLimiting(
        name="LimitingSolver",
        regular_solver=aderdg_solver,
        limiting_solver=fv_solver,
        number_of_dmp_observables=len(unknowns),
        dmp_relaxation_parameter=0.01,  # 0.02 works for depth 2, 0.01 works for depth 3
        dmp_differences_scaling=0.03,  # 0.04 works for depth 2, 0.02 works for depth 3
        physical_admissibility_criterion=isPhysicallyAdmissible,
    )
    project.add_solver(limiter)

project.generate_and_build()
