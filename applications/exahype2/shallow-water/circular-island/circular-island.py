# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  constexpr double h0 = 0.32;
  constexpr double A  = 0.064;
  constexpr double xc = 12.5;
  constexpr double yc = 15.0;
  constexpr double rc = 3.6;
  const double gamma  = std::sqrt((3.0 * A) / (4.0 * h0));

  auto r = [](double x, double y) {
    return std::sqrt(std::pow(x - xc, 2) + std::pow(y - yc, 2));
  };

  Q[Shortcuts::h] = h0 + (A / h0 * std::pow(1.0 / (std::cosh(gamma * (x(0) - 2.5))), 2));
  Q[Shortcuts::z] = r(x(0), x(1)) <= rc ? 0.93 * (1.0 - r(x(0), x(1)) / rc) : 0.0;
"""

boundary_conditions = """
  Qoutside[Shortcuts::h]  = Qinside[Shortcuts::h];
  Qoutside[Shortcuts::hu] = -Qinside[Shortcuts::hu];
  Qoutside[Shortcuts::hv] = -Qinside[Shortcuts::hv];
  Qoutside[Shortcuts::z]  = Qinside[Shortcuts::z];
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

size = [30.0, 30.0]
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=args.degrees_of_freedom,
    unknowns={"h": 1, "hu": 1, "hv": 1, "z": 1},
    auxiliary_variables=0,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.5,
)

aderdg_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux=f"ShallowWater::flux<double, Shortcuts, {aderdg_solver.unknowns}>(Q, x, h, t, dt, normal, F);",
    max_eigenvalue="return ShallowWater::maxEigenvalue<double, Shortcuts>(Q, x, h, t, dt, normal);",
    ncp=f"ShallowWater::nonconservativeProduct<double, Shortcuts, {aderdg_solver.unknowns}>(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);",
)

aderdg_solver.add_kernel_optimisations(
    is_linear=False, polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Legendre
)
aderdg_solver.add_user_solver_includes(
    """
#include "../PDE.h"
"""
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "ShallowWater"],
    project_name="CircularIsland",
    directory=".",
    executable="ExaHyPE",
)
project.add_solver(aderdg_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=2,
    size=size,
    offset=[0.0, 0.0],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
    ],
)

project.set_load_balancer(
    f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees})"
)
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
for const_name, const_info in constants.items():
    const_val, const_type = const_info
    project.constants.export_constexpr_with_type(const_name, str(const_val), const_type)
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
