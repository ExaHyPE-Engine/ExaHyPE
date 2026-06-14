# This file is part of the ExaHyPE2 project. For conditions of distribution and
# use, please see the copyright notice at www.peano-framework.org
import argparse

import exahype2

from PDE import max_eigenvalue, flux


def parse_arguments():
    parser = argparse.ArgumentParser(add_help=False, allow_abbrev=False)
    parser.add_argument(
        "-h",
        "--help",
        action="store_true",
        help="show this help message and continue with project help",
    )
    parser.add_argument("-et", "--end-time", type=float, default=0.01)
    parser.add_argument("-dofs", "--degrees-of-freedom", type=int, default=16)
    parser.add_argument("-l", "--grid-levels", type=int, default=6)
    args, _ = parser.parse_known_args()
    if args.help:
        parser.print_help()
        print()
    return args


args = parse_arguments()

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

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="SmoothConvergence",
    dimensions=2,
    size=[0.2, 0.2, 0.2],
    offset=[-0.1, -0.1, -0.1],
    periodic_BC=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

fv_solver = exahype2.solvers.fv.musclhancock.GlobalFixedTimeStep(
    name="MUSCLHancockSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": project.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    normalised_time_step_size=0.0001,
)

fv_solver.set_implementation(
    initial_conditions=initial_conditions,
    boundary_conditions=boundary_conditions,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
    limiter=exahype2.solvers.fv.musclhancock.Limiter.vanalbada,
)

project.add_solver(fv_solver)

project = project.generate_Peano4_project(verbose=False)
project.output.makefile.add_CXX_flag("-DGAMMA=1.4")
project.output.makefile.add_CXX_flag("-DPRESSURE=1.0")
project.build(make=True, make_clean_first=True, throw_away_data_after_build=True)
