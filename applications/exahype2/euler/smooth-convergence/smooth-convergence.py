# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys

import peano4
import exahype2

sys.path.insert(0, os.path.abspath(".."))
from PDE import max_eigenvalue, flux

initial_conditions = """
  Q[Shortcuts::rho] = 1.0 + 0.2 * std::cos(2.0 * tarch::la::PI * x[0]);
  Q[Shortcuts::rhoU + 0] = 1.0;
  Q[Shortcuts::rhoU + 1] = 0.0;
#if DIMENSIONS == 3
  Q[Shortcuts::rhoU + 2] = 0.0;
#endif
  Q[Shortcuts::rhoE] = PRESSURE / (GAMMA - 1) + 0.5 / Q[Shortcuts::rho] * (Q[Shortcuts::rhoU] * Q[Shortcuts::rhoU]);
"""

boundary_conditions = """
  // Outflow boundary conditions
  Qoutside[Shortcuts::rho]      = Qinside[Shortcuts::rho];
  Qoutside[Shortcuts::rhoU + 0] = Qinside[Shortcuts::rhoU + 0];
  Qoutside[Shortcuts::rhoU + 1] = Qinside[Shortcuts::rhoU + 1];
#if DIMENSIONS == 3
  Qoutside[Shortcuts::rhoU + 2] = Qinside[Shortcuts::rhoU + 2];
#endif
  Qoutside[Shortcuts::rhoE]     = Qinside[Shortcuts::rhoE];
"""

parser = exahype2.ArgumentParser("ExaHyPE 2 Euler Smooth Convergence Argument Parser")
parser.set_defaults(
    min_depth=6,
    end_time=0.01,
    time_step_size=0.0001,  # Suitable up to roughly min_depth = 6 based on speed of sound and max eigenvalue estimation
    degrees_of_freedom=16,
)
args = parser.parse_args()

size = [0.2, 0.2, 0.2]
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

riemann_solver = exahype2.solvers.fv.musclhancock.GlobalFixedTimeStep(
    name="MUSCLHancockSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": args.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    min_volume_h=min_h,
    max_volume_h=max_h,
    normalised_time_step_size=args.time_step_size,
)

riemann_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
    limiter=exahype2.solvers.fv.musclhancock.Limiter.vanalbada,
)

riemann_solver.set_plotter(args.plotter)

project = exahype2.Project(
    namespace=["applications", "exahype2", "euler"],
    project_name="SmoothConvergence",
    directory=".",
    executable="Euler",
)
project.add_solver(riemann_solver)

if args.number_of_snapshots <= 0:
    time_in_between_plots = 0.0
else:
    time_in_between_plots = args.end_time / args.number_of_snapshots
    project.set_output_path(args.output)

project.set_global_simulation_parameters(
    dimensions=args.dimensions,
    size=size[0 : args.dimensions],
    offset=[-0.1, -0.1, -0.1][0 : args.dimensions],
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
project.output.makefile.add_CXX_flag("-DPRESSURE=1.0")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
