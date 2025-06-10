# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys

import peano4
import exahype2

sys.path.insert(0, os.path.abspath(".."))
from HLLEM import hllem

initial_conditions = """
  const double omega = std::sqrt(0.2 * GRAV);

  Q[s::b] = 0.1 * (std::pow(x[0], 2) + std::pow(x[1], 2));
  Q[s::h] = std::fmax(0.0, 0.05 * (2 * x[0] * std::cos(omega * 0) + 2 * x[1] * std::sin(omega * 0)) + 0.075 - Q[s::b]);
  Q[s::hu] = 0.5 * omega * std::sin(omega * 0) * Q[s::h];
  Q[s::hv] = 0.5 * omega * std::cos(omega * 0) * Q[s::h];

  //Q[s::h] = 0.025 * (1.0 - std::sqrt((x(0) * x(0)) + (x(1) * x(1))));
  //Q[s::b] = -0.2 * (1.0 - std::sqrt((x(0) * x(0)) + (x(1) * x(1)))) - 0.1;
"""

boundary_conditions = """
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

parser = exahype2.ArgumentParser()
parser.set_defaults(
    end_time=5.0,
)
args = parser.parse_args()

size = [4.0, 4.0]
max_h = (1.1 * min(size) / (3.0**args.min_depth))
min_h = max_h * 3.0 ** (-args.amr_levels)

riemann_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="HLLEMSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"b": 1},
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=args.time_step_relaxation,
)

riemann_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    riemann_solver=hllem,
)

riemann_solver.set_plotter(args.plotter)

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
    size=size,
    offset=[-2.0, -2.0],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
    ],
)

project.set_load_balancer(f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees_init}, {args.trees})")
project.set_Peano4_installation("../../../../", mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
project.set_fenv_handler(args.fpe)
project.output.makefile.add_CXX_flag("-DGRAV=9.81")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
