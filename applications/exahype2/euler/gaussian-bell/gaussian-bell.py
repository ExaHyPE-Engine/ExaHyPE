# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import os
import sys

import peano4
import exahype2

sys.path.insert(0, os.path.abspath(".."))
from PDE import max_eigenvalue, flux

initial_conditions = """
  Q[Shortcuts::rho] = 0.02 * (1.0 + std::exp(-0.5 * tarch::la::norm2(x) * tarch::la::norm2(x) / 0.01));

  Q[Shortcuts::rhoU + 0] = 1.0 * Q[Shortcuts::rho];
  Q[Shortcuts::rhoU + 1] = 1.0 * Q[Shortcuts::rho];
#if DIMENSIONS == 3
  Q[Shortcuts::rhoU + 2] = 1.0 * Q[Shortcuts::rho];
#endif

  const tarch::la::Vector<DIMENSIONS, double> momenta(1.0 * Q[Shortcuts::rho]);

  // Should lead to a constant pressure over the domain given the initial conditions
  Q[Shortcuts::rhoE] = PRESSURE / (GAMMA - 1.0) + 0.5 * (momenta * momenta) / Q[Shortcuts::rho];
"""

analytical_solution = """
  // The initial condition is transported without deformation in a periodic
  // domain [-.5, +.5], i.e. the value at a given point is the value at the
  // position x - v*t but accounting for periodicity.
  // Therefore we find the point x - v*t, and then shift this by instances
  // of 1.0 until we find the point within the domain.
#if DIMENSIONS == 2
  tarch::la::Vector<DIMENSIONS, double> pos = { (x[0] - t) + (int)(t + .5 - x[0]),
                                                (x[1] - t) + (int)(t + .5 - x[1]) };
#else
  tarch::la::Vector<DIMENSIONS, double> pos = { (x[0] - t) + (int)(t + .5 - x[0]),
                                                (x[1] - t) + (int)(t + .5 - x[1]),
                                                (x[2] - t) + (int)(t + .5 - x[2]) };
#endif

  solution[Shortcuts::rho] = 0.02 * (1.0 + std::exp(-0.5 * tarch::la::norm2(pos) * tarch::la::norm2(pos) / 0.01));
  solution[Shortcuts::rhoU + 0] = 1.0 * solution[Shortcuts::rho];
  solution[Shortcuts::rhoU + 1] = 1.0 * solution[Shortcuts::rho];
#if DIMENSIONS == 3
  solution[Shortcuts::rhoU + 2] = 1.0 * solution[Shortcuts::rho];
#endif

  const tarch::la::Vector<DIMENSIONS, double> momenta(1.0 * solution[Shortcuts::rho]);

  // Should lead to a constant p over the domain given the initial conditions
  solution[Shortcuts::rhoE] = PRESSURE / (GAMMA - 1.0) + 0.5 * (momenta * momenta) / solution[Shortcuts::rho];
"""

parser = exahype2.ArgumentParser("ExaHyPE 2 - Euler Gaussian Bell Argument Parser")
parser.set_defaults(
    min_depth=6,
    # end_time=2.0,
    degrees_of_freedom=16,
    periodic_boundary_conditions_x=True,
    periodic_boundary_conditions_y=True,
    periodic_boundary_conditions_z=True,
)
args = parser.parse_args()

size = [1.0, 1.0, 1.0]
max_h = 1.1 * min(size) / (3.0**args.min_depth)
min_h = max_h * 3.0 ** (-args.amr_levels)

riemann_solver = exahype2.solvers.fv.musclhancock.GlobalAdaptiveTimeStep(
    name="MUSCLHancockSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": args.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    min_volume_h=min_h,
    max_volume_h=max_h,
    time_step_relaxation=0.5,
)

riemann_solver.set_implementation(
    initial_conditions=initial_conditions,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
    limiter=exahype2.solvers.fv.musclhancock.Limiter.minmod,
)

riemann_solver.set_plotter(args.plotter)

exahype2.solvers.fv.ErrorMeasurement(
    riemann_solver,
    error_measurement_implementation=analytical_solution,
    output_file_name="Error",
)

project = exahype2.Project(
    namespace=["applications", "exahype2", "euler"],
    project_name="GaussianBell",
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
    offset=[-0.5, -0.5, -0.5][0 : args.dimensions],
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
    f"new ::exahype2::LoadBalancingConfiguration({args.load_balancing_quality}, 1, {args.trees}, {args.trees})"
)
project.set_Peano4_installation(
    "../../../../", mode=peano4.output.string_to_mode(args.build_mode)
)
project = project.generate_Peano4_project(verbose=False)
project.set_fenv_handler(args.fpe)
project.output.makefile.set_target_device(args.target_device)
project.output.makefile.add_CXX_flag("-DGAMMA=1.4")
project.output.makefile.add_CXX_flag("-DPRESSURE=1.0")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
