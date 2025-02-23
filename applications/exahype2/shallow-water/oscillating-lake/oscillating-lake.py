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
  const double omega = sqrt(0.2 * GRAV);

  using s = VariableShortcuts;

  Q[s::b] = 0.1 * (std::pow(x[0], 2) + std::pow(x[1], 2));
  Q[s::h] = std::fmax(0.0, 0.05 * (2 * x[0] * std::cos(omega * 0) + 2 * x[1] * std::sin(omega * 0)) + 0.075 - Q[s::b]);
  Q[s::hu] = 0.5 * omega * std::sin(omega * 0) * Q[s::h];
  Q[s::hv] = 0.5 * omega * std::cos(omega * 0) * Q[s::h];

  //Q[s::h] = 0.025 * (1.0 - std::sqrt((x(0) * x(0)) + (x(1) * x(1))));
  //Q[s::b] = -0.2 * (1.0 - std::sqrt((x(0) * x(0)) + (x(1) * x(1)))) - 0.1;
"""

boundary_conditions = """
  using s = VariableShortcuts;
  Qoutside[s::h]  = Qinside[s::h];
  Qoutside[s::hu] = -Qinside[s::hu];
  Qoutside[s::hv] = -Qinside[s::hv];
  Qoutside[s::b]  = Qinside[s::b];
"""

refinement_criterion = """
  return std::sqrt((x(0) * x(0)) + (x(1) * x(1))) <= 1.0
           ? ::exahype2::RefinementCommand::Refine
           : ::exahype2::RefinementCommand::Keep;
"""

parser = SWEArgumentParser()
parser.set_defaults(
    end_time=1.0,
    width=[4.0, 4.0],
    offset=[-2.0, -2.0],
)
args = parser.parse_args()

max_h = (1.1 * min(args.width) / (3.0**args.min_depth))
min_h = max_h * 3.0 ** (-args.amr_levels)
args.patch_size = args.dg_order * 2 + 1

unknowns={"h": 1, "hu": 1, "hv": 1}
auxiliary_variables={"b": 1}

riemann_solver = exahype2.solvers.fv.riemann.GlobalAdaptiveTimeStep(
    name="HLLEM" + str(args.patch_size),
    patch_size=args.patch_size,
    unknowns=unknowns,
    auxiliary_variables=auxiliary_variables,
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=args.time_step_relaxation,
    pde_terms_without_state=args.stateless,
    initial_conditions=re.sub(r'\bx\b', 'volumeCentre', initial_conditions),
    boundary_conditions=boundary_conditions,
    refinement_criterion=re.sub(r'\bx\b', 'volumeCentre', refinement_criterion),
    riemann_solver="return applications::exahype2::swe::hllem(QR, QL, FR, FL, LR, LL, xR, xL, h, t, dt, normal, APDQ, AMDQ);",
)
riemann_solver.plot_description = ", ".join(unknowns.keys()) + ", "
riemann_solver.plot_description += ", ".join(auxiliary_variables.keys())
riemann_solver.switch_storage_scheme(exahype2.solvers.Storage[args.storage], exahype2.solvers.Storage[args.storage])
riemann_solver.add_user_solver_includes(
    """
#include "../HLLEM.h"
"""
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "swe"],
    project_name="OscillatingLake",
    directory=".",
    executable=f"OscillatingLake.{args.build_mode}",
)
project.add_solver(riemann_solver)

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
project.set_timeout(3600)
project.set_Peano4_installation("../../../../", mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=True)
if args.fpe:
    project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")

project.output.makefile.add_CXX_flag("-DGRAV=9.81")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)

print(args)
print(project)
print(riemann_solver)
