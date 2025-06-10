# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys

import peano4
import exahype2

sys.path.insert(0, os.path.abspath(".."))
from PDE import max_eigenvalue, flux

initial_conditions = """
  Q[s::rho]       = 1.0;
  Q[s::rhoU + 0]  = 0.0;
  Q[s::rhoU + 1]  = 0.0;
#if Dimensions == 2
  Q[s::rhoE]      = ((std::sqrt(std::pow(0.5 - x(0), 2) + std::pow(0.5 - x(1), 2)) < 0.2) ? (1.0) : (1.01));
#else
  Q[s::rhoU + 2]  = 0.0;
  Q[s::rhoE]      = ((std::sqrt(std::pow(0.5 - x(0), 2) + std::pow(0.5 - x(1), 2) + std::pow(0.5 - x(2), 2)) < 0.2) ? (1.0) : (1.01));
#endif
"""

boundary_conditions = """
  // Reflective boundary conditions
  Qoutside[s::rho]      = Qinside[s::rho];
  Qoutside[s::rhoU + 0] = -Qinside[s::rhoU + 0];
  Qoutside[s::rhoU + 1] = -Qinside[s::rhoU + 1];
#if Dimensions == 3
  Qoutside[s::rhoU + 2] = -Qinside[s::rhoU + 2];
#endif
  Qoutside[s::rhoE]     = Qinside[s::rhoE];
"""

refinement_criterion = """
  auto result = ::exahype2::RefinementCommand::Keep;

#if Dimensions == 3
  tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.5, 0.5};
#else
  tarch::la::Vector<Dimensions, double> circleCentre = {0.5, 0.5};
#endif

  if (tarch::la::equals(t, 0.0)) {
    if (tarch::la::norm2(x - circleCentre) < 0.1) {
      result = ::exahype2::RefinementCommand::Refine;
    }
  }

  return result;
"""

parser = exahype2.ArgumentParser("ExaHyPE 2 Navier-Stokes Point Explosion Argument Parser")
args = parser.parse_args()

size = [1.0, 1.0, 1.0]
max_h = (1.1 * min(size) / (3.0**args.min_depth))
min_h = max_h * 3.0 ** (-args.amr_levels)

riemann_solver = exahype2.solvers.fv.godunov.GlobalAdaptiveTimeStep(
    name="FVSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": args.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=args.time_step_relaxation,
    use_enclave_tasking=args.use_enclave_tasking,
    number_of_enclave_tasks=args.ntasks,
)

riemann_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    refinement_criterion=refinement_criterion,
    max_eigenvalue=max_eigenvalue,
    flux=flux,
)

riemann_solver.set_plotter(args.plotter)

project = exahype2.Project(
    namespace=["applications", "exahype2", "navierstokes"],
    project_name="PointExplosion",
    directory=".",
    executable=f"PointExplosion{args.dimensions}D.{args.build_mode}",
)
project.add_solver(riemann_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=args.dimensions,
    size=size[0:args.dimensions],
    offset=[0.0, 0.0, 0.0][0:args.dimensions],
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

project.set_load_balancer(f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees_init}, {args.trees})")
project.set_Peano4_installation("../../../../", mode=peano4.output.string_to_mode(args.build_mode))
project = project.generate_Peano4_project(verbose=False)
project.set_fenv_handler(args.fpe)
project.output.makefile.set_target_device(args.target_device)
project.output.makefile.add_CXX_flag("-DGAMMA=1.4")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
