# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

end_time=10.0
width=[10000, 10000]  # [m]
offset=[-5000, -5000]  # [m]
min_depth=4
amr_levels=0
dg_order=5
number_of_snapshots=0
build_mode="Release"
storage="CallStack"
output="solutions"
load_balancing_strategy="SpreadOutOnceGridStagnates"
load_balancing_quality=0.99
trees=-2

initial_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Q[i] = 0.0;
  }

  constexpr double INITIAL_WATER_HEIGHT = 100.0;
  constexpr double INITIAL_BATHYMETRY_BEFORE_EARTHQUAKE = -100.0;

  using s = VariableShortcuts;
  Q[s::h] = INITIAL_WATER_HEIGHT;

  auto displacementX = [](double x) { return std::sin((x / 500.0 + 1.0) * std::numbers::pi); };
  auto displacementY = [](double y) { return -std::pow(y / 500.0, 2) + 1.0; };
  auto displacement = [&](double x, double y) { return 5.0 * displacementX(x) * displacementY(y); };

  const double bathymetryAfterEarthquake = INITIAL_BATHYMETRY_BEFORE_EARTHQUAKE + displacement(x(0), x(1));

  if (std::abs(x(0)) <= 500 and std::abs(x(1)) <= 500) { // Center of domain is [0, 0]
    Q[s::b] = bathymetryAfterEarthquake;
  } else {
    Q[s::b] = INITIAL_BATHYMETRY_BEFORE_EARTHQUAKE;
  }
"""

boundary_conditions = """
  using s = VariableShortcuts;
  Qoutside[s::h]  = Qinside[s::h];
  Qoutside[s::hu] = -Qinside[s::hu];
  Qoutside[s::hv] = -Qinside[s::hv];
  Qoutside[s::b]  = Qinside[s::b];
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;
  if (std::abs(x(0)) <= 500 and std::abs(x(1)) <= 500) { // Center of domain is [0, 0]
    result = ::exahype2::RefinementCommand::Refine;
  }
  return result;
"""

max_h = (1.1 * min(width) / (3.0**min_depth))
min_h = max_h * 3.0 ** (-amr_levels)

aderdg_solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDG" + str(dg_order),
    order=dg_order,
    unknowns={"h": 1, "hu": 1, "hv": 1, "b": 1},
    auxiliary_variables=0,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.5,
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    flux="applications::exahype2::swe::flux(Q, x, h, t, dt, normal, F);",
    ncp="applications::exahype2::swe::nonconservativeProduct(Q, deltaQ, x, h, t, dt, normal, BTimesDeltaQ);",
    eigenvalues="return applications::exahype2::swe::maxEigenvalue(Q, x, h, t, dt, normal);",
)
aderdg_solver.plot_description = ", ".join({"h": 1, "hu": 1, "hv": 1, "b": 1}.keys())
aderdg_solver.switch_storage_scheme(exahype2.solvers.Storage[storage], exahype2.solvers.Storage[storage])
aderdg_solver.add_kernel_optimisations(is_linear=False, polynomials=exahype2.solvers.aderdg.Polynomials.Gauss_Legendre)
aderdg_solver.add_user_solver_includes(
    """
#include "../../../../applications/exahype2/shallow-water/SWE.h"
"""
)

project = exahype2.Project(
    namespace=["benchmarks", "exahype2", "swe"],
    project_name="ArtificialTsunami",
    directory=".",
    executable=f"ArtificialTsunami.{build_mode}",
)
project.add_solver(aderdg_solver)

if number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = end_time / number_of_snapshots
    project.set_output_path(output)

project.set_global_simulation_parameters(
    dimensions=2,
    size=width,
    offset=offset,
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[False, False]
)

if load_balancing_strategy != "None":
    strategy = f"toolbox::loadbalancing::strategies::{load_balancing_strategy}"
    configuration = (
        f"new ::exahype2::LoadBalancingConfiguration("
        f"{load_balancing_quality},"
        f"0,"
        f"{'false'},"
        f"{trees},"
        f"{trees})"
    )
    project.set_load_balancing(strategy, configuration)

project.set_Peano4_installation("../../../../", mode=peano4.output.string_to_mode(build_mode))
project = project.generate_Peano4_project(verbose=True)
project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")

project.output.makefile.add_CXX_flag("-DGRAV=9.81")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)

print(project)
print(aderdg_solver)
