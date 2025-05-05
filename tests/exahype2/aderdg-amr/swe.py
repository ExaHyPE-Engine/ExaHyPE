# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

order = 3
max_h = 1.0 / 9.0
min_h = 0.9 / 27.0  # 0.9 / 81.0
plot_interval = 0.0  # 0.01
end_time = 0.1

polynomials = exahype2.solvers.aderdg.Polynomials.Gauss_Legendre

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name="SWE",
    directory=".",
    executable="ADERDG-AMR"
)

solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=order,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.8,
    unknowns=4,
    auxiliary_variables=0,
    initial_conditions="""
  double r = std::sqrt((x[0] - 0.5) * (x[0] - 0.5) + (x[1] - 0.5) * (x[1] - 0.5));
  Q[0] = 1.;
  Q[1] = 0.;
  Q[2] = 0.;
  Q[3] = r < 0.1 ? 0.0 : 0.05;
""",
    boundary_conditions="""
  initialCondition(Qoutside, x, h, true);
""",
    refinement_criterion="""
  auto result = ::exahype2::RefinementCommand::Keep;
  if (x[0] < 0.3) {
    result = ::exahype2::RefinementCommand::Refine;
  }
  return result;
""",
    eigenvalues="""
  constexpr double grav = 9.81;
  const double u = Q[1 + normal] / Q[0];
  const double c = std::sqrt(grav * Q[0]);
  return std::max(std::abs(u + c), std::abs(u - c));
""",
    flux="""
  const double ih = 1.0 / Q[0];
  F[0] = Q[1 + normal];
  F[1] = Q[1 + normal] * Q[1] * ih;
  F[2] = Q[1 + normal] * Q[2] * ih;
  F[3] = 0.0;
""",
    ncp="""
  constexpr double grav = 9.81;
  BTimesDeltaQ[0]       = 0.0;
  switch (normal) {
  case 0:
    BTimesDeltaQ[1] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
    BTimesDeltaQ[2] = 0.0;
    break;
  case 1:
    BTimesDeltaQ[1] = 0.0;
    BTimesDeltaQ[2] = grav * Q[0] * (deltaQ[0] + deltaQ[3]);
    break;
  }
  BTimesDeltaQ[3] = 0.0;
""",
)

# solver._split_sweeps = True
project.add_solver(solver)
project.set_output_path("solutions")

project.set_global_simulation_parameters(
    dimensions=2,
    offset=[0.0, 0.0],
    size=[1.0, 1.0],
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=plot_interval,
    periodic_BC=[False, False],
)

project.set_load_balancing(
    "toolbox::loadbalancing::strategies::SpreadOutOnceGridStagnates",
    "new ::exahype2::LoadBalancingConfiguration(0.98)",
)
project.set_Peano4_installation("../../../", peano4.output.CompileMode.Release)
project = project.generate_Peano4_project(verbose=False)
#project.set_fenv_handler("FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW")

project.build(make_clean_first=True)
