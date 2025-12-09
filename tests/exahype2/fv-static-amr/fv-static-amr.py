# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

parser = exahype2.ArgumentParser("ExaHyPE 2 - Finite Volumes Static AMR Testing Script")
available_precisions = ["float", "double"]
parser.add_argument(
    "-pr", "--precision", default="double", help="|".join(available_precisions)
)
parser.set_defaults(
    min_depth=4,
    amr_levels=2,
    degrees_of_freedom=16,
    end_time=0.1,
)
args = parser.parse_args()

max_h = 1.1 / 3.0**args.min_depth
min_h = max_h * 3.0 ** (-args.amr_levels)

initial_conditions = """
  Q[Shortcuts::rho]       = 1.0;
  Q[Shortcuts::rhoU + 0]  = 0.0;
  Q[Shortcuts::rhoU + 1]  = 0.0;
#if DIMENSIONS == 2
  Q[Shortcuts::rhoE]      = ((std::sqrt(std::pow(0.5 - x(0), 2) + std::pow(0.5 - x(1), 2)) < 0.2) ? (1.0) : (1.01));
#else
  Q[Shortcuts::rhoU + 2]  = 0.0;
  Q[Shortcuts::rhoE]      = ((std::sqrt(std::pow(0.5 - x(0), 2) + std::pow(0.5 - x(1), 2) + std::pow(0.5 - x(2), 2)) < 0.2) ? (1.0) : (1.01));
#endif
"""

boundary_conditions = """
  // Reflective boundary conditions
  Qoutside[Shortcuts::rho]      = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::rhoU + 0] = -Qinside[Shortcuts::rhoU + 0];
  Qoutside[Shortcuts::rhoU + 1] = -Qinside[Shortcuts::rhoU + 1];
#if DIMENSIONS == 3
  Qoutside[Shortcuts::rhoU + 2] = -Qinside[Shortcuts::rhoU + 2];
#endif
  Qoutside[Shortcuts::rhoE]     = Qinside[Shortcuts::rhoE];
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;

#if DIMENSIONS == 3
  tarch::la::Vector<DIMENSIONS, double> circleCentre = {0.5, 0.5, 0.5};
#else
  tarch::la::Vector<DIMENSIONS, double> circleCentre = {0.5, 0.5};
#endif

  if (tarch::la::equals(t, 0.0)) {
    if (tarch::la::norm2(x - circleCentre) < 0.1) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  return result;
"""

compute_primitive_variables = r"""
  const auto irho = 1.0 / Q[Shortcuts::rho];
  const auto u0 = Q[Shortcuts::rhoU + 0] * irho;
  const auto u1 = Q[Shortcuts::rhoU + 1] * irho;
#if DIMENSIONS == 3
  const auto u2 = Q[Shortcuts::rhoU + 2] * irho;
#endif

#if DIMENSIONS == 3
  const auto uSq = u0 * u0 + u1 * u1 + u2 * u2;
  const auto u_n = (normal == 0) ? u0 : (normal == 1) ? u1 : u2;
#else
  const auto uSq = u0 * u0 + u1 * u1;
  const auto u_n = (normal == 0) ? u0 : u1;
#endif

  const auto internalE = Q[Shortcuts::rhoE] - 0.5 * Q[Shortcuts::rho] * uSq;
  const auto p = (GAMMA - 1.0) * internalE;
"""

max_eigenvalue = r"""
  {compute_primitive_variables}
  const auto speedOfSound = std::sqrt(GAMMA * p * irho);
  auto result = std::fmax(0.0, std::fabs(u_n - speedOfSound));
  result = std::fmax(result, std::fabs(u_n + speedOfSound));
  return result;
""".format(
    compute_primitive_variables=compute_primitive_variables
)

flux = r"""
  {compute_primitive_variables}

  F[Shortcuts::rho] = Q[Shortcuts::rhoU + normal];

  F[Shortcuts::rhoU + 0] = Q[Shortcuts::rhoU + 0] * u_n;
  F[Shortcuts::rhoU + 1] = Q[Shortcuts::rhoU + 1] * u_n;
#if DIMENSIONS == 3
  F[Shortcuts::rhoU + 2] = Q[Shortcuts::rhoU + 2] * u_n;
#endif

  F[Shortcuts::rhoU + normal] += p;

  F[Shortcuts::rhoE] = (Q[Shortcuts::rhoE] + p) * u_n;
""".format(
    compute_primitive_variables=compute_primitive_variables
)

fv_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": args.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.5,
    use_enclave_tasking=args.enclave_tasking,
    number_of_enclave_tasks=args.ntasks,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
)

fv_solver.set_solver_precisions(
    storage_precision=args.precision, compute_precision=args.precision
)

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name=".",
    directory=".",
    executable="ExaHyPE",
)
project.add_solver(fv_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=args.dimensions,
    size=[1.0, 1.0, 1.0][0 : args.dimensions],
    offset=[0.0, 0.0, 0.0][0 : args.dimensions],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    first_plot_time_stamp=0.0,
    time_in_between_plots=time_in_between_plots,
    periodic_BC=[
        args.periodic_boundary_conditions_x,
        args.periodic_boundary_conditions_y,
        args.periodic_boundary_conditions_z,
    ],
)

project.set_load_balancer("new ::exahype2::LoadBalancingConfiguration")
project.set_Peano4_installation(
    "../../../", mode=peano4.output.string_to_mode(args.build_mode)
)
project = project.generate_Peano4_project(verbose=False)
project.set_fenv_handler(args.fpe)
project.output.makefile.set_target_device(args.target_device)
project.output.makefile.add_CXX_flag("-DGAMMA=1.4")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
