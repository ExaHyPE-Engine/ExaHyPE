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
  Q.rho     = 1.0 + 0.2 * std::cos(2.0 * tarch::la::PI * x[0]);
  Q.rhoU    = 0.0;
  Q.rhoU(0) = 1.0;
  Q.rhoE    = PRESSURE / (GAMMA - 1) + 0.5 / Q.rho * (Q.rhoU * Q.rhoU);
"""

boundary_conditions = """
  // Outflow boundary conditions
  Qoutside = Qinside;
"""

project = exahype2.Project(
    namespace=["tests", "exahype2", "fv"],
    project_name="SmoothConvergence",
    dimensions=2,
    size=[0.2, 0.2, 0.2],
    offset=[-0.1, -0.1, -0.1],
    periodic_bc=[False, False, False],
    min_end_time=args.end_time,
    max_end_time=args.end_time,
    min_grid_levels=args.grid_levels,
    max_grid_levels=args.grid_levels,
)

fv_solver = exahype2.fv.Solver(
    name="MUSCLHancockSolver",
    patch_size=args.degrees_of_freedom,
    unknowns={"rho": 1, "rhoU": project.dimensions, "rhoE": 1},
    auxiliary_variables=0,
    time_stepping=exahype2.TimeStepping.Adaptive(0.5),
    scheme=exahype2.fv.MusclHancock(limiter=exahype2.fv.Limiter.vanalbada),
)

fv_solver.set_pde_terms(
    initial_condition=initial_conditions,
    boundary_conditions=boundary_conditions,
    flux=flux,
    max_eigenvalue=max_eigenvalue,
)

project.add_solver(fv_solver)

project.output.makefile.add_compile_definition("GAMMA=1.4")
project.output.makefile.add_compile_definition("PRESSURE=1.0")
project.generate_and_build()
