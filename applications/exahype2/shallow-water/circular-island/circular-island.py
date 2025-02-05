# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import re
import sys

import peano4
import exahype2

sys.path.insert(0, os.path.abspath(".."))
from sweargs import SWEArgumentParser

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  static constexpr double h0    = 0.32;
  static constexpr double A     = 0.064;
  static constexpr double xc    = 12.5;
  static constexpr double yc    = 15.0;
  static constexpr double rc    = 3.6;
  const double gamma            = std::sqrt((3.0 * A) / (4.0 * h0));

  auto r = [](double x, double y) {
    return std::sqrt(std::pow(x - xc, 2) + std::pow(y - yc, 2));
  };

  using s = VariableShortcuts;
  Q[s::h] = h0 + (A / h0 * std::pow(1.0 / (std::cosh(gamma * (x(0) - 2.5))), 2));
  Q[s::b] = r(x(0), x(1)) <= rc ? 0.93 * (1.0 - r(x(0), x(1)) / rc) : 0.0;
"""

boundary_conditions = """
  using s = VariableShortcuts;
  Qoutside[s::h]  = Qinside[s::h];
  Qoutside[s::hu] = -Qinside[s::hu];
  Qoutside[s::hv] = -Qinside[s::hv];
  Qoutside[s::b]  = Qinside[s::b];
"""

refinement_criterion = """
  static constexpr double xc = 12.5;
  static constexpr double yc = 15.0;
  static constexpr double rc = 3.6;

  auto r = [](double x, double y) {
    return std::sqrt(std::pow(x - xc, 2) + std::pow(y - yc, 2));
  };

  return r(x(0), x(1)) <= rc
           ? ::exahype2::RefinementCommand::Refine
           : ::exahype2::RefinementCommand::Keep;
"""

parser = SWEArgumentParser()
parser.set_defaults(
    end_time=1.0,
    width=[30.0, 30.0],
    offset=[0.0, 0.0],
    min_depth=4,
    amr_levels=0,
    dg_order=5,
    number_of_snapshots=20,
    peano_dir="../../../../",
)
args = parser.parse_args()

max_h = (1.1 * min(args.width) / (3.0**args.min_depth))
min_h = max_h * 3.0 ** (-args.amr_levels)

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDG" + str(args.dg_order),
    order=args.dg_order,
    unknowns={"h": 1, "hu": 1, "hv": 1, "b": 1},
    auxiliary_variables=0,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=args.time_step_relaxation,
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux="applications::exahype2::swe::flux(Q, x, h, t, dt, normal, F);",
    ncp="applications::exahype2::swe::nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);",
    eigenvalues="return applications::exahype2::swe::maxEigenvalue(Q, x, h, t, dt, normal);",
)
aderdg_solver.plot_description = ", ".join({"h": 1, "hu": 1, "hv": 1, "b": 1}.keys())
aderdg_solver.switch_storage_scheme(exahype2.solvers.Storage[args.storage], exahype2.solvers.Storage[args.storage])
aderdg_solver.add_kernel_optimisations(is_linear=False, polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Legendre)
aderdg_solver.add_user_solver_includes(
    """
#include "../SWE.h"
"""
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "swe"],
    project_name="CircularIsland",
    directory=".",
    executable=f"CircularIsland.{args.build_mode}",
)
project.add_solver(aderdg_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=2,
    size=args.width,
    offset=args.offset,
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
    ],
)

if args.load_balancing_strategy != "None":
    strategy = f"toolbox::loadbalancing::strategies::{args.load_balancing_strategy}"
    assume_periodic_boundary_conditions = any([
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
    ])
    configuration = (
        f"new ::exahype2::LoadBalancingConfiguration("
        f"{args.load_balancing_quality},"
        f"0,"
        f"{'true' if assume_periodic_boundary_conditions else 'false'},"
        f"{args.trees},"
        f"{args.trees})"
    )
    project.set_load_balancing(strategy, configuration)

project.set_number_of_threads(args.threads)
project.set_timeout(args.timeout)
project.set_Peano4_installation(args.peano_dir, mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=True)
if args.fpe:
    project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")

project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)

print(args)
print(project)
print(aderdg_solver)
