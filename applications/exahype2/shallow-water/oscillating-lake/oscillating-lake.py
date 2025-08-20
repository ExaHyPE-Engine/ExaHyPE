# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys

import peano4
import exahype2

sys.path.insert(0, os.path.abspath(".."))
from HLLEM import hllem

initial_conditions = """
  const double omega = std::sqrt(0.2 * g);

  Q[Shortcuts::z] = 0.1 * (std::pow(x[0], 2) + std::pow(x[1], 2));
  Q[Shortcuts::h] = std::fmax(0.0, 0.05 * (2 * x[0] * std::cos(omega * 0) + 2 * x[1] * std::sin(omega * 0)) + 0.075 - Q[Shortcuts::z]);
  Q[Shortcuts::hu] = 0.5 * omega * std::sin(omega * 0) * Q[Shortcuts::h];
  Q[Shortcuts::hv] = 0.5 * omega * std::cos(omega * 0) * Q[Shortcuts::h];

  //Q[Shortcuts::h] = 0.025 * (1.0 - std::sqrt((x(0) * x(0)) + (x(1) * x(1))));
  //Q[Shortcuts::z] = -0.2 * (1.0 - std::sqrt((x(0) * x(0)) + (x(1) * x(1)))) - 0.1;
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu] = -Qinside[Shortcuts::hu];
  Qoutside[Shortcuts::hv] = -Qinside[Shortcuts::hv];
  Qoutside[Shortcuts::z]  = Qinside[Shortcuts::z];
"""

refinement_criterion = """
  return std::sqrt((x(0) * x(0)) + (x(1) * x(1))) <= 1.0
           ? ::exahype2::RefinementCommand::Refine
           : ::exahype2::RefinementCommand::Keep;
"""

parser = exahype2.ArgumentParser()
parser.set_defaults(
    min_depth=4,
    end_time=1.0,
)
args = parser.parse_args()

constants = {
    "g": [9.81, "double"],
    "hThreshold": [1e-5, "double"],
}

size = [4.0, 4.0]
max_h = (1.1 * min(size) / (3.0**args.min_depth))
min_h = max_h * 3.0 ** (-args.amr_levels)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 1, "hv": 1},
    auxiliary_variables={"z": 1},
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=args.time_step_relaxation,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    riemann_solver=hllem,
)

fv_solver.set_plotter(args.plotter)

project = exahype2.Project(
    namespace=["applications", "exahype2", "swe"],
    project_name="OscillatingLake",
    directory=".",
    executable="ExaHyPE-ShallowWater",
)
project.add_solver(fv_solver)

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

project.set_load_balancer(f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees}, {args.trees})")
project.set_Peano4_installation("../../../../", mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
for const_name, const_info in constants.items():
    const_val, const_type = const_info
    project.constants.export_constexpr_with_type(
        const_name, str(const_val), const_type
    )
project.set_fenv_handler(args.fpe)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
