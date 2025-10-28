# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

parser = exahype2.ArgumentParser("ExaHyPE 2 - ADER-DG Static AMR Testing Script")
parser.set_defaults(
    min_depth=3,
    amr_levels=1,
    degrees_of_freedom=4,
)
args = parser.parse_args()

size = [1.0, 1.0]
order = args.degrees_of_freedom - 1
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)
plot_interval = 0.01
end_time = 0.1

polynomials = exahype2.solvers.aderdg.Polynomials.Gauss_Legendre

project = exahype2.Project(
    namespace=["tests", "exahype2", "aderdg"],
    project_name=".",
    directory=".",
    executable="ExaHyPE",
)

solver = exahype2.solvers.aderdg.GlobalAdaptiveTimeStep(
    name="ADERDGSolver",
    order=order,
    min_cell_h=min_h,
    max_cell_h=max_h,
    time_step_relaxation=0.8,
    unknowns=4,
    auxiliary_variables=0,
)

solver.set_implementation(
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
    max_eigenvalue="""
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

project.add_solver(solver)
project.set_output_path("solutions")

project.set_global_simulation_parameters(
    dimensions=2,
    offset=[0.0, 0.0],
    size=size,
    min_end_time=end_time,
    max_end_time=end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=plot_interval,
    periodic_BC=[False, False],
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
project.set_Peano4_installation(
    "../../../", mode=peano4.output.string_to_mode(args.build_mode)
)
project = project.generate_Peano4_project(verbose=False)
project.set_fenv_handler(True)
project.build(make_clean_first=True)
