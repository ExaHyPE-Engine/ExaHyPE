# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import peano4
import exahype2

from PDE import max_eigenvalue, flux

initial_conditions = """
  Q[Shortcuts::rho]      = 1.0;
  Q[Shortcuts::rhoU + 0] = 0.0; // Flow in x-direction
  Q[Shortcuts::rhoU + 1] = 0.0; // No vertical velocity
#if DIMENSIONS == 3
  Q[Shortcuts::rhoU + 2] = 0.0;
#endif
  Q[Shortcuts::rhoE] = 1.0 / (GAMMA - 1.0) + 0.5 * (Q[Shortcuts::rhoU + 0] * Q[Shortcuts::rhoU + 0] + Q[Shortcuts::rhoU + 1] * Q[Shortcuts::rhoU + 1]) / Q[Shortcuts::rho];
"""

boundary_conditions = """
  for (int i = 0; i < NumberOfUnknowns + NumberOfAuxiliaryVariables; i++) {
    Qoutside[i] = Qinside[i];
  }

  // Inlet boundary condition (x[0] = 0.0)
  if (tarch::la::smallerEquals(x[0], MaxAdmissibleCellH)) {
    Qoutside[Shortcuts::rhoU + 0] = 1.0;
  }

  // Outlet boundary condition
  if (tarch::la::greaterEquals(x[0], DomainSize[0] - MaxAdmissibleCellH)) {
    Qoutside[Shortcuts::rhoU + 0] = Qinside[Shortcuts::rhoU + 0];
  }
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;
  return result;
"""

parser = exahype2.ArgumentParser("ExaHyPE 2 - Finite Volumes Testing Script")
parser.set_defaults(min_depth=6, degrees_of_freedom=16, end_time=2.0)
args = parser.parse_args()

size = [1.0, 1.0, 1.0]
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

fv_solver = exahype2.solvers.fv.musclhancock.GlobalAdaptiveTimeStep(
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
    limiter=exahype2.solvers.fv.musclhancock.Limiter.minmod,
)

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="ChannelFlow",
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
    size=size[0 : args.dimensions],
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

project.set_load_balancer(
    f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees})"
)
project.set_build_mode(mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
project.output.makefile.set_target_device(args.target_device)
project.output.makefile.add_CXX_flag("-DGAMMA=1.4")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
